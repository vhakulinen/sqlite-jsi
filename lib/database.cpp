#include "jsi/jsi.h"

#include "sqlite-jsi/connection.h"
#include "sqlite-jsi/database.h"
#include "sqlite-jsi/sqlite-jsi.h"
#include "sqlite-jsi/statement.h"
#include "sqlite-jsi/utils.h"
#include "sqlite-jsi/value.h"

namespace sqlitejsi {
using namespace sqlitejsi;

Database::Database(jsi::Runtime &rt, std::string name, int flags,
                   std::shared_ptr<RtInvoker> invoker)
    : m_invoker(invoker), m_executor(std::make_shared<Executor>()) {

  sqlite3 *db;
  int res = sqlite3_open_v2(name.c_str(), &db, flags, NULL);
  if (res != SQLITE_OK) {
    const char *err = sqlite3_errmsg(db);
    sqlite3_close_v2(db);
    throw jsi::JSError(rt, err);
  }

  m_conn = std::make_shared<Connection>(db);
}

Database::~Database() {
  ConnectionGuard conn(*m_conn);
  sqlite3_close_v2(&conn);
}

bool Database::isBusy() { return m_executor->isBusy(); }

jsi::Value Database::get(jsi::Runtime &rt, const jsi::PropNameID &name) {
  auto prop = name.utf8(rt);

  if (prop == "toString") {
    return createToString(rt, name, "Database");
  }

  // TODO(ville): get

  if (prop == "exec") {
    // (query: string): Promise<void>
    return jsi::Function::createFromHostFunction(
        rt, name, 1,
        [&](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *args,
            size_t count) {
          if (count != 1) {
            throw jsi::JSError(
                rt, sqliteJSIBadArgumentsError(rt, "invalid arguments"));
          }

          auto query =
              std::make_shared<std::string>(args[0].asString(rt).utf8(rt));

          return rt.global()
              .getPropertyAsFunction(rt, "Promise")
              .callAsConstructor(
                  rt, jsi::Function::createFromHostFunction(
                          rt, jsi::PropNameID::forUtf8(rt, "Promise"), 2,
                          createExec(query)));
        });
  }

  if (prop == "select") {
    // Proxy to db.prepare(query).select(params)
    return jsi::Function::createFromHostFunction(
        rt, name, 1,
        [&](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *args,
            size_t count) {
          if (count < 1) {
            throw jsi::JSError(rt, "query argument missing");
          }

          auto queryParams = std::make_shared<std::vector<jsi::Value>>();
          for (size_t i = 1; i < count; i++) {
            queryParams->push_back(jsi::Value(rt, args[i]));
          }

          auto then = jsi::Function::createFromHostFunction(
              rt, jsi::PropNameID::forUtf8(rt, "then"), 1,
              [=](jsi::Runtime &rt, const jsi::Value &thisVal,
                  const jsi::Value *thenArgs, size_t thenCount) {
                if (thenCount < 1) {
                  throw jsi::JSError(rt, "invalid result");
                }
                auto stmt =
                    thenArgs[0].asObject(rt).asHostObject<Statement>(rt);

                auto fn = stmt->get(rt, jsi::PropNameID::forUtf8(rt, "select"))
                              .asObject(rt)
                              .asFunction(rt);

                // TODO(ville): Is there no nicer way to do this?
                if (queryParams->size() > 0) {
                  return fn.call(rt, *queryParams->data());
                } else {
                  return fn.call(rt);
                }
              });

          auto promise = this->get(rt, jsi::PropNameID::forUtf8(rt, "prepare"))
                             .asObject(rt)
                             .asFunction(rt)
                             .callWithThis(rt, thisVal.asObject(rt), args[0])
                             .asObject(rt);

          return promise.getPropertyAsFunction(rt, "then")
              .callWithThis(rt, promise, then);
        });
  }

  if (prop == "prepare") {
    // (query: string) -> Promise<Statement>
    return jsi::Function::createFromHostFunction(
        rt, name, 1,
        [&](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *args,
            size_t count) {
          if (count < 1) {
            throw jsi::JSError(rt, "query argument missing");
          }

          auto query =
              std::make_shared<std::string>(args[0].asString(rt).utf8(rt));

          return rt.global()
              .getPropertyAsFunction(rt, "Promise")
              .callAsConstructor(
                  rt, jsi::Function::createFromHostFunction(
                          rt, jsi::PropNameID::forUtf8(rt, "Promise"), 2,
                          createPrepare(query)));
        });
  }

  return jsi::Value::undefined();
}

jsi::HostFunctionType
Database::createPrepare(std::shared_ptr<std::string> query) {
  return [&](jsi::Runtime &rt, const jsi::Value &thisVal,
             const jsi::Value *args, size_t count) {
    if (count < 2) {
      throw jsi::JSError(rt, "Invalid promise constructor");
    }

    auto resolve = std::make_shared<jsi::Value>(rt, args[0]);
    auto reject = std::make_shared<jsi::Value>(rt, args[1]);

    m_executor->queue([=]() {
      ConnectionGuard conn(*m_conn);

      sqlite3_stmt *stmt = NULL;
      const char *tail = NULL;

      int res = sqlite3_prepare_v2(&conn, query->c_str(), -1, &stmt, &tail);
      if (res != SQLITE_OK) {
        std::string errMsg = sqlite3_errmsg(&conn);

        sqlite3_finalize(stmt);

        return (*m_invoker)([reject, res, errMsg](jsi::Runtime &rt) {
          auto err = sqliteError(rt, res, errMsg);
          reject->asObject(rt).asFunction(rt).call(rt, err);
        });
      }

      if (!stmt) {
        // Probably not needed, but calling finalize on NULL pointer is
        // harmless no-op.
        sqlite3_finalize(stmt);

        return (*m_invoker)([reject](jsi::Runtime &rt) {
          reject->asObject(rt).asFunction(rt).call(
              rt, sqliteJSIBadQueryError(
                      rt, "query doesn't contain any sql statements"));
        });
      }

      if (tail[0]) {
        sqlite3_finalize(stmt);

        return (*m_invoker)([reject](jsi::Runtime &rt) {
          reject->asObject(rt).asFunction(rt).call(
              rt, sqliteJSIBadQueryError(
                      rt, "query must contain only one sql statement"));
        });
      }

      return (*m_invoker)([=](jsi::Runtime &rt) {
        auto stmtObj =
            std::make_shared<Statement>(m_conn, m_executor, m_invoker, stmt);
        auto obj = jsi::Object::createFromHostObject(rt, stmtObj);
        resolve->asObject(rt).asFunction(rt).call(rt, obj);
      });
    });

    return jsi::Value::undefined();
  };
}

jsi::HostFunctionType
Database::createExec(std::shared_ptr<std::string> queryStr) {
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
      const char *query = queryStr->c_str();
      const char *tail = query;
      sqlite3_stmt *stmt = NULL;

      while (query[0]) {
        // If we have multiple statements, be sure to finalize the earlier one.
        sqlite3_finalize(stmt);
        stmt = NULL;

        int prepareRes = sqlite3_prepare_v2(&conn, query, -1, &stmt, &tail);
        if (prepareRes != SQLITE_OK) {
          auto errMsg = std::make_shared<std::string>(sqlite3_errmsg(&conn));

          sqlite3_finalize(stmt);

          return (*m_invoker)([reject, prepareRes, errMsg](jsi::Runtime &rt) {
            auto err = sqliteError(rt, prepareRes, *errMsg);
            reject->asObject(rt).asFunction(rt).call(rt, err);
          });
        }

        if (!stmt) {
          // Probably not needed, but calling finalize on NULL pointer is
          // harmless no-op.
          sqlite3_finalize(stmt);

          // SQLITE_OK and no statement means the query is whitespace or a
          // comment.
          query = tail;
          continue;
        }

        int stepRes;
        while ((stepRes = sqlite3_step(stmt)) != SQLITE_DONE) {
          if (stepRes == SQLITE_ROW) {
            // In case the statement gives us rows, ignore them.
            continue;
          }

          auto msg = std::make_shared<std::string>(sqlite3_errmsg(&conn));

          sqlite3_finalize(stmt);

          return (*m_invoker)([reject, stepRes, msg](jsi::Runtime &rt) {
            auto err = sqliteError(rt, stepRes, *msg);
            reject->asObject(rt).asFunction(rt).call(rt, err);
          });
        }

        query = tail;
      }

      // Finalize the statement.
      sqlite3_finalize(stmt);

      return (*m_invoker)([resolve](jsi::Runtime &rt) {
        resolve->asObject(rt).asFunction(rt).call(rt);
      });
    });

    return jsi::Value::undefined();
  };
}

} // namespace sqlitejsi
