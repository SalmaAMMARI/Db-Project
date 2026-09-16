#include "minidb/executor.hpp"
#include "minidb/database.hpp"

#include "minidb/buffer_manager/buffer_manager.hpp"
#include "minidb/catalog.hpp"
#include "minidb/config.hpp"
#include "minidb/index/b_plus_tree.hpp"
#include "minidb/query.hpp"
#include "minidb/query_result.hpp"
#include "minidb/semantic_verifier.hpp"
#include "minidb/table_manager/table_manager.hpp"

#include <algorithm>
#include <cstring>
#include <sstream>

Executor::Executor(DataBase& db_, ExecutionContext& ctx_) : db(db_), ctx(ctx_) {
}

std::string Executor::execute(const Query& query) {
	if (auto* q = dynamic_cast<const CreateTableQuery*>(&query)) {
		return exec_create(*q);
	} else if (auto* q = dynamic_cast<const InsertQuery*>(&query)) {
		return exec_insert(*q);
	} else if (auto* q = dynamic_cast<const SelectQuery*>(&query)) {
		return exec_select(*q);
	} else if (auto* q = dynamic_cast<const CreateIndexQuery*>(&query)) {
		return exec_create_index(*q);
	} else {
		return ("UNKNOWN QUERY");
	}
}

std::string Executor::exec_create(const CreateTableQuery& query) {
	// update db tablemanager
	// register table in catalog

	std::string path = Config::get_instance()->datadir + "/" + query.table_name() + ".tbl";

	int id = db.get_table_manager().create_table(query.table_name(), path);
	if (id == -1) {
		return QueryResult(false, "TABLE " + query.table_name() + " ALREADY EXIST")();
	}

	db.getCatalog().createTable({query.table_name(), query.columns()});
	return QueryResult(true, "TABLE " + query.table_name() + " CREATED")();
}

std::string Executor::exec_insert(const InsertQuery& query) {
	// schema validation
	try {
		SemanticVerifier verifier(db.getCatalog());
		verifier.verify_insert_query(query);
	} catch (const std::exception& e) {
		return QueryResult(false, e.what())();
	}

	int table_id = db.get_table_manager().get_table_id(query.table_name());

	auto table_info_opt = db.getCatalog().getTable(query.table_name());
	auto schema			= table_info_opt->columns;

	std::sort(
		schema.begin(), schema.end(),
		[](const ColumnDefinition& a, const ColumnDefinition& b) { return (a.name < b.name); });

	// serialization;
	auto				 value_map = query.columns_to_values();
	std::vector<uint8_t> record;
	for (const auto& col : schema) {
		auto			   it	   = value_map.find(col.name);
		const std::string& val_str = it->second;

		switch (col.type_) {
		case DataType::INT: {
			int v = std::stoi(val_str);
			record.insert(record.end(), reinterpret_cast<uint8_t*>(&v),
						  reinterpret_cast<uint8_t*>(&v) + sizeof(int));
			break;
		}
		case DataType::FLOAT: {
			float v = std::stof(val_str);
			record.insert(record.end(), reinterpret_cast<uint8_t*>(&v),
						  reinterpret_cast<uint8_t*>(&v) + sizeof(float));
			break;
		}
		case DataType::BOOL: {
			bool v = (val_str == "true");
			record.push_back(static_cast<uint8_t>(v));
			break;
		}
		case DataType::STRING: {
			uint16_t len = val_str.size();
			record.insert(record.end(), reinterpret_cast<uint8_t*>(&len),
						  reinterpret_cast<uint8_t*>(&len) + sizeof(uint16_t));
			record.insert(record.end(), val_str.begin(), val_str.end());
			break;
		}
		}
	}

	// inserting into table
	try {
		RecordId rid = db.get_table_manager().insert(table_id, record);

		const auto& indices = table_info_opt->indexes;
		if (!indices.empty()) {
			size_t offset = 0;
			for (const auto& col : schema) {
				// If there's an index on this column
				auto idx_it =
					std::find_if(indices.begin(), indices.end(),
								 [&](const IndexInfo& idx) { return idx.column == col.name; });
				if (idx_it != indices.end()) {
					// Extract value for this column
					switch (col.type_) {
					case DataType::INT: {
						int v;
						std::memcpy(&v, &record[offset], sizeof(int));
						db.get_index_manager().insert(query.table_name(), col.name, v, rid);
						break;
					}
					case DataType::FLOAT: {
						float v;
						std::memcpy(&v, &record[offset], sizeof(float));
						db.get_index_manager().insert(query.table_name(), col.name, v, rid);
						break;
					}
					case DataType::BOOL: {
						bool v = record[offset];
						db.get_index_manager().insert(query.table_name(), col.name, v, rid);
						break;
					}
					case DataType::STRING: {
						uint16_t len;
						std::memcpy(&len, &record[offset], sizeof(uint16_t));
						offset += sizeof(uint16_t);
						std::string v(reinterpret_cast<const char*>(&record[offset]), len);

						db.get_index_manager().insert<std::string>(query.table_name(), col.name, v,
																   rid);
						offset += len; // string case has extra shift
						continue;	   // skip generic offset update below
					}
					}
				}

				// Move offset even if column is not indexed
				switch (col.type_) {
				case DataType::INT:
					offset += sizeof(int);
					break;
				case DataType::FLOAT:
					offset += sizeof(float);
					break;
				case DataType::BOOL:
					offset += sizeof(uint8_t);
					break;
				case DataType::STRING: {
					uint16_t len;
					std::memcpy(&len, &record[offset], sizeof(uint16_t));
					offset += sizeof(uint16_t) + len;
					break;
				}
				}
			}
		}
		return QueryResult(1)();
	} catch (const std::exception& e) {
		return QueryResult(false, e.what())();
	}
}

std::string Executor::exec_select(const SelectQuery& query) {
	try {
		SemanticVerifier verifier(db.getCatalog());
		verifier.verify_select_query(query);
	} catch (const std::exception& e) {
		return QueryResult(false, e.what())();
	}

	auto table_manager = db.get_table_manager();
	// linear scan of the table
	auto table_id = table_manager.get_table_id(query.from());

	auto table_info_opt = db.getCatalog().getTable(query.from());
	auto schema			= table_info_opt->columns;

	std::sort(schema.begin(), schema.end(),
			  [](const ColumnDefinition& a, const ColumnDefinition& b) { return a.name < b.name; });

	const auto& selected_cols = query.columns();

	std::vector<RecordId> record_ids = table_manager.scan(table_id);

	QueryResult results(selected_cols);

	for (const auto& rid : record_ids) {
		auto   record = table_manager.get(table_id, rid);
		size_t offset = 0;

		std::unordered_map<std::string, std::string> col_values;
		for (const auto& col : schema) {
			if (offset >= record.size())
				break;

			std::string val;

			switch (col.type_) {
			case DataType::INT: {
				int v;
				std::memcpy(&v, &record[offset], sizeof(int));
				val = std::to_string(v);
				offset += sizeof(int);
				break;
			}
			case DataType::FLOAT: {
				float v;
				std::memcpy(&v, &record[offset], sizeof(int));
				val = std::to_string(v);
				offset += sizeof(float);
				break;
			}
			case DataType::BOOL: {
				bool v = record[offset];
				val	   = v ? "true" : "false";
				offset += sizeof(uint8_t);
				break;
			}
			case DataType::STRING: {
				uint16_t len;
				std::memcpy(&len, &record[offset], sizeof(uint16_t));
				offset += sizeof(uint16_t);

				val = std::string(reinterpret_cast<const char*>(&record[offset]), len);
				offset += len;
				break;
			}
			}
			col_values[col.name] = val;
		}

		std::vector<std::string> row;
		for (const auto& col : selected_cols) {
			row.push_back(col_values[col]);
		}
		results.add_row(row);
	}
	return (results());
}

std::string Executor::exec_create_index(const CreateIndexQuery& q) {
	try {
		SemanticVerifier verifier(db.getCatalog());
		verifier.verify_create_index(q);
	} catch (const std::exception& e) {
		return QueryResult(false, e.what())();
	}
	this->db.get_index_manager().create_index(q.table_name(), q.column_name());

	//1-  we need to insert the keys into the index

	int	  table_id		 = db.get_table_manager().get_table_id(q.table_name());
	auto  table_info_opt = db.getCatalog().getTable(q.table_name());
	auto& schema		 = table_info_opt->columns;

	// let's order the schema first
	std::sort(
		schema.begin(), schema.end(),
		[](const ColumnDefinition& a, const ColumnDefinition& b) { return (a.name < b.name); });

	// 2- Find the column definition for the indexed column
	auto it = std::find_if(schema.begin(), schema.end(), [&](const ColumnDefinition& col) {
		return col.name == q.column_name();
	});
	ColumnDefinition indexed_col = *it;

	// 3. Scan all record IDs
	std::vector<RecordId> record_ids = db.get_table_manager().scan(table_id);

	// 4. For each record, extract the key column and insert into index
	for (const auto& rid : record_ids) {
		auto record = db.get_table_manager().get(table_id, rid);

		// Deserialize record fields
		size_t offset = 0;
		for (const auto& col : schema) {
			if (offset >= record.size())
				break;

			if (col.name == indexed_col.name) {
				// Indexed column → extract its value
				switch (col.type_) {
				case DataType::INT: {
					int v;
					std::memcpy(&v, &record[offset], sizeof(int));
					db.get_index_manager().insert<int>(q.table_name(), q.column_name(), v, rid);
					break;
				}
				case DataType::FLOAT: {
					float v;
					std::memcpy(&v, &record[offset], sizeof(float));
					db.get_index_manager().insert<float>(q.table_name(), q.column_name(), v, rid);
					break;
				}
				case DataType::BOOL: {
					bool v = record[offset];
					db.get_index_manager().insert<bool>(q.table_name(), q.column_name(), v, rid);
					break;
				}
				case DataType::STRING: {
					uint16_t len;
					std::memcpy(&len, &record[offset], sizeof(uint16_t));
					offset += sizeof(uint16_t);

					std::string v(reinterpret_cast<const char*>(&record[offset]), len);
					db.get_index_manager().insert<std::string>(q.table_name(), q.column_name(), v,
															   rid);
					break;
				}
				}
			}

			// Advance offset regardless
			switch (col.type_) {
			case DataType::INT:
				offset += sizeof(int);
				break;
			case DataType::FLOAT:
				offset += sizeof(float);
				break;
			case DataType::BOOL:
				offset += sizeof(uint8_t);
				break;
			case DataType::STRING: {
				uint16_t len;
				std::memcpy(&len, &record[offset], sizeof(uint16_t));
				offset += sizeof(uint16_t) + len;
				break;
			}
			}
		}
	}

	return QueryResult(true, "INDEX CREATED SUCCESSFULLY")();
}
