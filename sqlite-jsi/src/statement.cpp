#include "jsi/jsi.h"
#include "sqlite-jsi/transaction.h"
#include "sqlite3.h"

#include "sqlite-jsi/connection.h"
#include "sqlite-jsi/promise.h"
#include "sqlite-jsi/statement.h"
#include "sqlite-jsi/utils.h"
#include "sqlite-jsi/value.h"

namespace sqlitejsi {
using namespace facebook;
using namespace sqlitejsi;

struct Column {
  std::string name;
  Value val;

  jsi::Value toValue(jsi::Runtime &rt);
};

typedef std::vector<Column> Row;

jsi::Value Column::toValue(jsi::Runtime &rt) { return this->val.toJsi(rt); }

Row parseRow(sqlite3_stmt *stmt) {
  auto count = sqlite3_data_count(stmt);
  auto row = Row();

  for (int i = 0; i < count; i++) {
    auto name = std::string(sqlite3_column_name(stmt, i));

    row.push_back(Column{name, Value::fromSqlite(stmt, i)});
  }

  return row;
}

#define STATEMENT_RESET_AND_RETURN(lambda)                                     \
  /* Reset the statement. */                                                   \
  sqlite3_reset(m_stmt);                                                       \
  /* Clear any bindings. */                                                    \
  sqlite3_clear_bindings(m_stmt);                                              \
  return (*m_invoker)lambda;

#define STATEMENT_BIND_OR_RETURN(params)                                       \
  for (size_t i = 0; i < params.size(); i++) {                                 \
    int bind = params[i].bind(m_stmt, i + 1);                                  \
    if (bind != SQLITE_OK) {                                                   \
      /* Clear the bindings. */                                                \
      sqlite3_clear_bindings(m_stmt);                                          \
      auto msg = std::make_shared<std::string>(sqlite3_errmsg(&conn));         \
      return (*m_invoker)([promise, bind, msg](jsi::Runtime &rt) {             \
        promise->reject(rt, sqliteError(rt, bind, *msg));                      \
      });                                                                      \
    }                                                                          \
  }

jsi::Value Statement::get(jsi::Runtime &rt, const jsi::PropNameID &name) {
  auto prop = name.utf8(rt);

  if (prop == "toString") {
    return createToString(rt, name, "Statement");
  }

  // type QueryParam = string | number | null;

  if (prop == "exec") {
    // (query: string, ...params: QueryParam[]): Promise<????>
    return jsi::Function::createFromHostFunction(
        rt, name, 1,
        [&](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *args,
            size_t count) {
          return this->sqlExec(rt, m_executor, args, count);
        });
  }

  if (prop == "select") {
    return jsi::Function::createFromHostFunction(
        rt, name, 1,
        [&](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *args,
            size_t count) {
          return this->sqlSelect(rt, m_executor, args, count);
        });
  }

  if (prop == "get") {
    return jsi::Function::createFromHostFunction(
        rt, name, 1,
        [&](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *args,
            size_t count) {
          return this->sqlGet(rt, m_executor, args, count);
        });
  }

  return jsi::Value::undefined();
}

template <typename T>
jsi::Value Statement::sqlExec(jsi::Runtime &rt, std::shared_ptr<T> executor,
                              const jsi::Value *args, size_t count) {
  auto params = std::make_shared<Params>(Value::fromJsiArgs(rt, args, count));

  return Promise::createPromise(rt, [=](auto &rt, auto promise) {
    // Enter executor.
    executor->queue(rt, [=]() {
      ConnectionGuard conn(*m_conn);

      STATEMENT_BIND_OR_RETURN((*params));

      int stepRes;
      // While it would be a unusual use case, a statment's exec might
      // as well return rows. If it does, ignore them.
      while (stepRes = sqlite3_step(m_stmt), stepRes != SQLITE_DONE) {
        if (stepRes == SQLITE_ROW) {
          // In case the statement gives us rows, ignore them.
          continue;
        }

        auto msg = std::make_shared<std::string>(sqlite3_errmsg(&conn));

        STATEMENT_RESET_AND_RETURN(([promise, stepRes, msg](jsi::Runtime &rt) {
          promise->reject(rt, sqliteError(rt, stepRes, *msg));
        }));
      }

      STATEMENT_RESET_AND_RETURN(([promise](jsi::Runtime &rt) {
        promise->resolve(rt, jsi::Value::undefined());
      }));
    });
  });
}

template <typename T>
jsi::Value Statement::sqlSelect(jsi::Runtime &rt, std::shared_ptr<T> executor,
                                const jsi::Value *args, size_t count) {
  auto params = std::make_shared<Params>(Value::fromJsiArgs(rt, args, count));

  return Promise::createPromise(rt, [=](auto &rt, auto promise) {
    // Enter executor.
    executor->queue(rt, [=]() {
      ConnectionGuard conn(*m_conn);

      STATEMENT_BIND_OR_RETURN((*params))

      auto rows = std::make_shared<std::vector<Row>>();

      int stepRes;
      // While it would be a unusual use case, a statment's exec might
      // return rows. If it does, ignore them.
      while (stepRes = sqlite3_step(m_stmt), stepRes != SQLITE_DONE) {
        if (stepRes == SQLITE_ROW) {
          rows->push_back(parseRow(m_stmt));
          continue;
        }

        auto msg = std::make_shared<std::string>(sqlite3_errmsg(&conn));

        STATEMENT_RESET_AND_RETURN(([promise, stepRes, msg](jsi::Runtime &rt) {
          promise->reject(rt, sqliteError(rt, stepRes, *msg));
        }));
      }

      STATEMENT_RESET_AND_RETURN(([promise, rows](jsi::Runtime &rt) {
        auto rowObjs = jsi::Array(rt, rows->size());
        for (size_t i = 0; i < rowObjs.length(rt); i++) {
          jsi::Object obj = jsi::Object(rt);

          for (auto &column : (*rows)[i]) {
            obj.setProperty(rt, column.name.c_str(), column.toValue(rt));
          }

          rowObjs.setValueAtIndex(rt, i, obj);
        }

        promise->resolve(rt, std::move(rowObjs));
      }));
    });
  });
}

template <typename T>
jsi::Value Statement::sqlGet(jsi::Runtime &rt, std::shared_ptr<T> executor,
                             const jsi::Value *args, size_t count) {
  auto params = std::make_shared<Params>(Value::fromJsiArgs(rt, args, count));

  return Promise::createPromise(rt, [=](auto &rt, auto promise) {
    executor->queue(rt, [=]() {
      ConnectionGuard conn(*m_conn);

      STATEMENT_BIND_OR_RETURN((*params));

      int res = sqlite3_step(m_stmt);

      // If we're already done, reject with "no rows" error.
      if (res == SQLITE_DONE) {
        STATEMENT_RESET_AND_RETURN(([promise](jsi::Runtime &rt) {
          promise->reject(rt, sqliteJSINoRowsError(rt));
        }));
      }

      // Handle any errors.
      if (res != SQLITE_ROW) {
        auto msg = std::make_shared<std::string>(sqlite3_errmsg(&conn));

        STATEMENT_RESET_AND_RETURN(([promise, res, msg](jsi::Runtime &rt) {
          promise->reject(rt, sqliteError(rt, res, *msg));
        }));
      }

      // Parse the row.
      auto row = std::make_shared<Row>(parseRow(m_stmt));

      if ((res = sqlite3_reset(m_stmt)) != SQLITE_OK) {
        /**
         * NOTE(ville): Unlike the other API calls, in `get` we call
         * sqlite3_step only once and get what ever it returns.
         * According to sqlite3 docs, in such cases the reset might fail
         * even tho' the step call indicated no problems.
         *
         * I wasn't able to test such cases, so this code path is
         * untested...
         *
         * https://www.sqlite.org/c3ref/reset.html
         */

        auto msg = std::make_shared<std::string>(sqlite3_errmsg(&conn));

        sqlite3_clear_bindings(m_stmt);

        return (*m_invoker)([promise, res, msg](jsi::Runtime &rt) {
          promise->reject(rt, sqliteError(rt, res, *msg));
        });
      }

      STATEMENT_RESET_AND_RETURN(([promise, row](jsi::Runtime &rt) {
        jsi::Object obj = jsi::Object(rt);

        for (auto &column : *row) {
          obj.setProperty(rt, column.name.c_str(), column.toValue(rt));
        }

        promise->resolve(rt, std::move(obj));
      }));
    });
  });
}

#define EXECUTOR_TEMPLATE(exec)                                                \
  template jsi::Value Statement::sqlExec<exec>(                                \
      jsi::Runtime & rt, std::shared_ptr<exec> executor,                       \
      const jsi::Value *args, size_t count);                                   \
  template jsi::Value Statement::sqlGet<exec>(                                 \
      jsi::Runtime & rt, std::shared_ptr<exec> executor,                       \
      const jsi::Value *args, size_t count);                                   \
  template jsi::Value Statement::sqlSelect<exec>(                              \
      jsi::Runtime & rt, std::shared_ptr<exec> executor,                       \
      const jsi::Value *args, size_t count);

EXECUTOR_TEMPLATE(Executor)
EXECUTOR_TEMPLATE(TransactionExecutor)

} // namespace sqlitejsi
