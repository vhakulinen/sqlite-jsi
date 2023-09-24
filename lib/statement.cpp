#include "jsi/jsi.h"
#include "sqlite3.h"

#include "sqlite-jsi/connection.h"
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

Column parseColumn(sqlite3_stmt *stmt, int i) {
  auto name = std::string(sqlite3_column_name(stmt, i));
  auto type = sqlite3_column_type(stmt, i);

  switch (type) {
  case SQLITE_INTEGER: {
    auto v = Column{name, Value(sqlite3_column_int(stmt, i))};
    return Column{name, Value(sqlite3_column_int(stmt, i))};
  } break;
  case SQLITE_FLOAT: {
    return Column{name, Value(sqlite3_column_double(stmt, i))};
  } break;
  case SQLITE_TEXT: {
    std::string text = std::string(
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, i)));
    return Column{name, Value(text)};
  } break;
  case SQLITE_BLOB: {
    auto blob = static_cast<const char *>(sqlite3_column_blob(stmt, i));
    auto bytes = sqlite3_column_bytes(stmt, i);
    return Column{name, Value(std::vector<char>(blob, blob + bytes))};
  } break;
  case SQLITE_NULL: {
    return Column{name, Value()};
  } break;
  }

  __builtin_unreachable();
}

Row parseRow(sqlite3_stmt *stmt) {
  auto count = sqlite3_data_count(stmt);
  auto row = Row();

  for (int i = 0; i < count; i++) {
    row.push_back(parseColumn(stmt, i));
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
      return (*m_invoker)([reject, bind, msg](jsi::Runtime &rt) {              \
        auto err = sqliteError(rt, bind, *msg);                                \
        reject->asObject(rt).asFunction(rt).call(rt, err);                     \
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
          auto queryParams = std::make_shared<std::vector<Value>>(
              Value::fromJsiArgs(rt, args, count));

          return rt.global()
              .getPropertyAsFunction(rt, "Promise")
              .callAsConstructor(
                  rt, jsi::Function::createFromHostFunction(
                          rt, jsi::PropNameID::forUtf8(rt, "Promise"), 2,
                          createExec(queryParams)));
        });
  }

  if (prop == "select") {
    return jsi::Function::createFromHostFunction(
        rt, name, 1,
        [&](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *args,
            size_t count) {
          auto queryParams = std::make_shared<std::vector<Value>>(
              Value::fromJsiArgs(rt, args, count));

          return rt.global()
              .getPropertyAsFunction(rt, "Promise")
              .callAsConstructor(
                  rt, jsi::Function::createFromHostFunction(
                          rt, jsi::PropNameID::forUtf8(rt, "Promise"), 2,
                          createSelect(queryParams)));
        });
  }

  if (prop == "get") {
    return jsi::Function::createFromHostFunction(
        rt, name, 1,
        [&](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *args,
            size_t count) {
          auto queryParams = std::make_shared<std::vector<Value>>(
              Value::fromJsiArgs(rt, args, count));

          return rt.global()
              .getPropertyAsFunction(rt, "Promise")
              .callAsConstructor(
                  rt, jsi::Function::createFromHostFunction(
                          rt, jsi::PropNameID::forUtf8(rt, "Promise"), 2,
                          createGet(queryParams)));
        });
  }

  return jsi::Value::undefined();
}

jsi::HostFunctionType
Statement::createGet(std::shared_ptr<std::vector<Value>> params) {
  return [&](jsi::Runtime &rt, const jsi::Value &thisVal,
             const jsi::Value *args, size_t count) {
    if (count < 2) {
      throw jsi::JSError(rt, "Invalid promise constructor");
    }

    auto resolve = std::make_shared<jsi::Value>(rt, args[0]);
    auto reject = std::make_shared<jsi::Value>(rt, args[1]);

    m_executor->queue([=]() {
      ConnectionGuard conn(*m_conn);

      STATEMENT_BIND_OR_RETURN((*params));

      int res = sqlite3_step(m_stmt);

      // If we're already done, reject with "no rows" error.
      if (res == SQLITE_DONE) {
        STATEMENT_RESET_AND_RETURN(([reject](jsi::Runtime &rt) {
          auto err = sqliteJSINoRowsError(rt);
          reject->asObject(rt).asFunction(rt).call(rt, err);
        }));
      }

      // Handle any errors.
      if (res != SQLITE_ROW) {
        auto msg = std::make_shared<std::string>(sqlite3_errmsg(&conn));

        STATEMENT_RESET_AND_RETURN(([reject, res, msg](jsi::Runtime &rt) {
          auto err = sqliteError(rt, res, *msg);
          reject->asObject(rt).asFunction(rt).call(rt, err);
        }));
      }

      // Parse the row.
      auto row = std::make_shared<Row>(parseRow(m_stmt));

      if ((res = sqlite3_reset(m_stmt)) != SQLITE_OK) {
        /**
         * NOTE(ville): Unlike the other API calls, in `get` we call
         * sqlite3_step only once and get what ever it returns. According to
         * sqlite3 docs, in such cases the reset might fail even tho' the step
         * call indicated no problems.
         *
         * I wasn't able to test such cases, so this code path is untested...
         *
         * https://www.sqlite.org/c3ref/reset.html
         */

        auto msg = std::make_shared<std::string>(sqlite3_errmsg(&conn));

        sqlite3_clear_bindings(m_stmt);

        return (*m_invoker)([reject, res, msg](jsi::Runtime &rt) {
          auto err = sqliteError(rt, res, *msg);
          reject->asObject(rt).asFunction(rt).call(rt, err);
        });
      }

      STATEMENT_RESET_AND_RETURN(([resolve, row](jsi::Runtime &rt) {
        jsi::Object obj = jsi::Object(rt);

        for (auto column : *row) {
          obj.setProperty(rt, column.name.c_str(), column.toValue(rt));
        }

        resolve->asObject(rt).asFunction(rt).call(rt, obj);
      }));
    });

    return jsi::Value::undefined();
  };
}

jsi::HostFunctionType
Statement::createSelect(std::shared_ptr<std::vector<Value>> &params) {
  return [&](jsi::Runtime &rt, const jsi::Value &thisVal,
             const jsi::Value *args, size_t count) {
    if (count < 2) {
      throw jsi::JSError(rt, "Invalid promise constructor");
    }

    auto resolve = std::make_shared<jsi::Value>(rt, args[0]);
    auto reject = std::make_shared<jsi::Value>(rt, args[1]);

    // Enter executor.
    m_executor->queue([=]() {
      ConnectionGuard conn(*m_conn);

      STATEMENT_BIND_OR_RETURN((*params))

      auto rows = std::make_shared<std::vector<Row>>();

      int stepRes;
      // While it would be a unusual use case, a statment's exec might return
      // rows. If it does, ignore them.
      while (stepRes = sqlite3_step(m_stmt), stepRes != SQLITE_DONE) {
        if (stepRes == SQLITE_ROW) {
          rows->push_back(parseRow(m_stmt));
          continue;
        }

        auto msg = std::make_shared<std::string>(sqlite3_errmsg(&conn));

        STATEMENT_RESET_AND_RETURN(([reject, stepRes, msg](jsi::Runtime &rt) {
          auto err = sqliteError(rt, stepRes, *msg);
          reject->asObject(rt).asFunction(rt).call(rt, err);
        }));
      }

      STATEMENT_RESET_AND_RETURN(([resolve, rows](jsi::Runtime &rt) {
        auto rowObjs = jsi::Array(rt, rows->size());
        for (size_t i = 0; i < rowObjs.length(rt); i++) {
          jsi::Object obj = jsi::Object(rt);

          for (auto column : (*rows)[i]) {
            obj.setProperty(rt, column.name.c_str(), column.toValue(rt));
          }

          rowObjs.setValueAtIndex(rt, i, obj);
        }

        resolve->asObject(rt).asFunction(rt).call(rt, rowObjs);
      }));
    });

    return jsi::Value::undefined();
  };
}

jsi::HostFunctionType
Statement::createExec(std::shared_ptr<std::vector<Value>> &params) {
  return [&](jsi::Runtime &rt, const jsi::Value &thisVal,
             const jsi::Value *args, size_t count) {
    if (count < 2) {
      throw jsi::JSError(rt, "Invalid promise constructor");
    }

    auto resolve = std::make_shared<jsi::Value>(rt, args[0]);
    auto reject = std::make_shared<jsi::Value>(rt, args[1]);

    // Enter executor.
    m_executor->queue([=]() {
      ConnectionGuard conn(*m_conn);

      STATEMENT_BIND_OR_RETURN((*params));

      int stepRes;
      // While it would be a unusual use case, a statment's exec might as well
      // return rows. If it does, ignore them.
      while (stepRes = sqlite3_step(m_stmt), stepRes != SQLITE_DONE) {
        if (stepRes == SQLITE_ROW) {
          // In case the statement gives us rows, ignore them.
          continue;
        }

        auto msg = std::make_shared<std::string>(sqlite3_errmsg(&conn));

        STATEMENT_RESET_AND_RETURN(([reject, stepRes, msg](jsi::Runtime &rt) {
          auto err = sqliteError(rt, stepRes, *msg);
          reject->asObject(rt).asFunction(rt).call(rt, err);
        }));
      }

      STATEMENT_RESET_AND_RETURN(([resolve](jsi::Runtime &rt) {
        resolve->asObject(rt).asFunction(rt).call(rt);
      }));
    });

    return jsi::Value::undefined();
  };
}

} // namespace sqlitejsi
