#ifndef __EXECUTOR_HPP__
#define __EXECUTOR_HPP__

#include <string>
#include <thread>

class DataBase;
class Query;
class CreateTableQuery;
class InsertQuery;
class SelectQuery;
class CreateIndexQuery;

/**
 * @struct ExecutionContext
 * @brief Represents the execution context for a session or transaction.
 *
 * Tracks whether a transaction is active, the transaction ID,
 * and the thread/session ID executing the queries.
 */
struct ExecutionContext {
	bool in_transaction = false;
	int	 transaction_id = -1;

	std::thread::id session_id;
	ExecutionContext() : session_id(std::this_thread::get_id()) {}
};

/**
 * @class Executor
 * @brief Executes database queries within a given execution context.
 *
 * Interfaces with the DataBase instance to run queries such as
 * create table, insert, and select.
 *
 * Provides a single execute() method that dispatches query types
 * to the appropriate private execution methods.
 */
class Executor {
  private:
	DataBase&		  db;
	ExecutionContext& ctx;

  public:
	Executor(DataBase& db, ExecutionContext& ctx);
	std::string execute(const Query& q);

  private:
	std::string exec_create(const CreateTableQuery& q);
	std::string exec_insert(const InsertQuery& q);
	std::string exec_select(const SelectQuery& q);
	std::string exec_create_index(const CreateIndexQuery& q);
};

#endif
