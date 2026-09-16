#include "minidb/semantic_verifier.hpp"

#include "minidb/catalog.hpp"
#include "minidb/query.hpp"
#include "minidb/table.hpp"

#include <algorithm>
#include <stdexcept>

SemanticVerifier::SemanticVerifier(const CatalogManager& cat) : catalog(cat) {
}

void SemanticVerifier::verify_insert_query(const InsertQuery& q) const {
	// 1- TABLE existance check
	auto tinfo_opt = catalog.getTable(q.table_name());
	if (!tinfo_opt)
		throw std::runtime_error("UNKNOWN TABLE");

	const auto&							 tinfo		   = tinfo_opt.value();
	const std::vector<ColumnDefinition>& table_columns = tinfo.columns;

	// 2- Columns and values count match
	if (q.columns().size() != q.values().size()) {
		throw std::runtime_error("COLUMNS AND VALUES UNMATCH");
	}

	// 3- Columns count unmatch schema columns count
	if (q.columns().size() != table_columns.size()) {
		throw std::runtime_error("SCHEMA UNMATCH");
	}

	// 4- Column existance check
	// 5. Type Check
	for (size_t i = 0; i < q.columns().size(); i++) {
		const std::string& col_name = q.columns()[i];
		const std::string& raw_val	= std::get<std::string>(q.values()[i]);

		auto it = std::find_if(table_columns.begin(), table_columns.end(),
							   [&](const ColumnDefinition& col) { return col.name == col_name; });
		if (it == table_columns.end()) {
			throw std::runtime_error("UNKNOWN COLUMN: " + col_name);
		}

		const ColumnDefinition& col_def = *it;

		auto converted = convert_string_to_type(raw_val, col_def.type_);
		if (!converted.has_value()) {
			throw std::runtime_error("TYPE MISMATCH on COLUMN: " + col_name);
		}
	}
}

void SemanticVerifier::verify_select_query(const SelectQuery& q) const {
	// 1- TABLE existance check
	auto table_info_opt = catalog.getTable(q.from());
	if (!table_info_opt)
		throw std::runtime_error("UNKNOWN TABLE");

	const auto&							 tinfo		   = table_info_opt.value();
	const std::vector<ColumnDefinition>& table_columns = tinfo.columns;

	if (q.columns().size() > table_columns.size()) {
		throw std::runtime_error("UNKNOWN COLUMNS IN SELECT");
	}

	// columns check
	for (size_t i = 0; i < q.columns().size(); i++) {
		const std::string& col_name = q.columns()[i];

		auto it = std::find_if(table_columns.begin(), table_columns.end(),
							   [&](const ColumnDefinition& col) { return col.name == col_name; });
		if (it == table_columns.end()) {
			throw std::runtime_error("UNKNOWN COLUMN: " + col_name);
		}
	}
}

void SemanticVerifier::verify_create_index(const CreateIndexQuery& q) const {
	auto table_info_opt = catalog.getTable(q.table_name());
	if (!table_info_opt) {
		throw std::runtime_error("UNKNOWN TABLE: " + q.table_name());
	}
	const auto& tinfo = table_info_opt.value();

	// 1. Check if column exists
	bool column_found = false;
	for (const auto& col : tinfo.columns) {
		if (col.name == q.column_name()) {
			column_found = true;
			break;
		}
	}
	if (!column_found) {
		throw std::runtime_error("UNKNOWN COLUMN: " + q.column_name() + " in table " +
								 q.table_name());
	}

	// 2. Check if index already exists on this column
	for (const auto& idx : tinfo.indexes) {
		if (idx.column == q.column_name()) {
			throw std::runtime_error("INDEX ALREADY EXISTS on column " + q.column_name() +
									 " of table " + q.table_name());
		}
	}
}
