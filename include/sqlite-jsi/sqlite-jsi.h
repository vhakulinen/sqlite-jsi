#pragma once

#include "jsi/jsi.h"

#include "sqlite-jsi/database.h"
#include <vector>

namespace sqlitejsi {

using namespace facebook;
using namespace sqlitejsi;

class Sqlite : public jsi::HostObject {
public:
  Sqlite(std::shared_ptr<RtInvoker> invoker);

  static std::shared_ptr<Sqlite> install(jsi::Runtime &rt,
                                         std::shared_ptr<RtInvoker> invoker);

  jsi::Value get(jsi::Runtime &rt, const jsi::PropNameID &name) override;

  bool isBusy();

private:
  std::shared_ptr<RtInvoker> m_invoker;

  std::mutex m_mdatabases;
  std::vector<std::shared_ptr<Database>> m_databases;

  jsi::Function createOpen(jsi::Runtime &);
};
} // namespace sqlitejsi
