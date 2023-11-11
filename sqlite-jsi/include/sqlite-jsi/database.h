#pragma once

#include "jsi/jsi.h"
#include "sqlite3.h"

#include "sqlite-jsi/connection.h"
#include "sqlite-jsi/database.h"
#include "sqlite-jsi/executor.h"
#include "sqlite-jsi/invoker.h"
#include "sqlite-jsi/value.h"

namespace sqlitejsi {
using namespace facebook;
using namespace sqlitejsi;

class Database : public jsi::HostObject {
public:
  Database(jsi::Runtime &, std::string name, int flags,
           std::shared_ptr<RtInvoker> invoker);

  ~Database();

  jsi::Value get(jsi::Runtime &, const jsi::PropNameID &name) override;

  template <typename T>
  jsi::Value prepare(jsi::Runtime &rt, std::shared_ptr<T> executor,
                     std::shared_ptr<std::string> query);

  bool isBusy();

private:
  /// Runtime invoker.
  std::shared_ptr<RtInvoker> m_invoker;
  /// Executor for executing work on another thread.
  std::shared_ptr<Executor> m_executor;

  /// Database connection.
  std::shared_ptr<Connection> m_conn;
};
} // namespace sqlitejsi
