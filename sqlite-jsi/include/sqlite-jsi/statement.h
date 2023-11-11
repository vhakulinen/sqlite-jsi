#pragma once

#include "jsi/jsi.h"
#include "sqlite3.h"

#include "sqlite-jsi/connection.h"
#include "sqlite-jsi/database.h"
#include "sqlite-jsi/invoker.h"

namespace sqlitejsi {
using namespace facebook;
using namespace sqlitejsi;

using Params = std::vector<Value>;

class Statement : public jsi::HostObject {
public:
  Statement(std::shared_ptr<Connection> conn,
            std::shared_ptr<Executor> executor,
            std::shared_ptr<RtInvoker> invoker, sqlite3_stmt *stmt)
      : m_conn(conn), m_executor(executor), m_invoker(invoker), m_stmt(stmt){};

  ~Statement() {
    // Finalize the statement.
    sqlite3_finalize(m_stmt);
  }

  jsi::Value get(jsi::Runtime &, const jsi::PropNameID &name) override;

  template <typename T>
  jsi::Value sqlExec(jsi::Runtime &rt, std::shared_ptr<T> executor,
                     const jsi::Value *args, size_t count);

  template <typename T>
  jsi::Value sqlSelect(jsi::Runtime &rt, std::shared_ptr<T> executor,
                       const jsi::Value *args, size_t count);

  template <typename T>
  jsi::Value sqlGet(jsi::Runtime &rt, std::shared_ptr<T> executor,
                    const jsi::Value *args, size_t count);

private:
  std::shared_ptr<Connection> m_conn;
  std::shared_ptr<Executor> m_executor;
  std::shared_ptr<RtInvoker> m_invoker;

  sqlite3_stmt *m_stmt;
};

} // namespace sqlitejsi
