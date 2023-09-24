#pragma once

#include "jsi/jsi.h"
#include "sqlite3.h"

#include "sqlite-jsi/connection.h"
#include "sqlite-jsi/database.h"
#include "sqlite-jsi/invoker.h"

namespace sqlitejsi {
using namespace facebook;
using namespace sqlitejsi;

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

private:
  std::shared_ptr<Connection> m_conn;
  std::shared_ptr<Executor> m_executor;
  std::shared_ptr<RtInvoker> m_invoker;

  sqlite3_stmt *m_stmt;

  jsi::HostFunctionType createExec(std::shared_ptr<std::vector<Value>> &params);

  jsi::HostFunctionType
  createSelect(std::shared_ptr<std::vector<Value>> &params);

  jsi::HostFunctionType createGet(std::shared_ptr<std::vector<Value>> params);
};

} // namespace sqlitejsi
