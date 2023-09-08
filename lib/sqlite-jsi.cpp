#include <cassert>
#include <iostream>
#include <mutex>
#include <thread>

#include "jsi/jsi.h"
#include "sqlite3.h"

#include "sqlite-jsi/database.h"
#include "sqlite-jsi/sqlite-jsi.h"
#include "sqlite-jsi/utils.h"

namespace sqlitejsi {

using namespace sqlitejsi;

Sqlite::Sqlite(std::shared_ptr<RtInvoker> invoker) : m_invoker(invoker) {}

std::shared_ptr<Sqlite> Sqlite::install(jsi::Runtime &rt,
                                        std::shared_ptr<RtInvoker> invoker) {
  auto sqlite = std::make_shared<sqlitejsi::Sqlite>(invoker);
  auto obj = jsi::Object::createFromHostObject(rt, sqlite);
  rt.global().setProperty(rt, "SQLite", obj);

  installUtils(rt);
  // auto js = std::make_shared<jsi::StringBuffer>("function SQLiteError (){}");
  // rt.evaluateJavaScript(js, "<eval>");

  return sqlite;
}

jsi::Value Sqlite::get(jsi::Runtime &rt, const jsi::PropNameID &name) {
  auto prop = name.utf8(rt);

  if (prop == "toString") {
    return createToString(rt, name, "SQLite");
  }

  if (prop == "open") {
    return createOpen(rt);
  }

  if (prop == "OPEN_READONLY") {
    return jsi::Value(SQLITE_OPEN_READONLY);
  }
  if (prop == "OPEN_READWRITE") {
    return jsi::Value(SQLITE_OPEN_READWRITE);
  }
  if (prop == "OPEN_CREATE") {
    return jsi::Value(SQLITE_OPEN_CREATE);
  }
  if (prop == "OPEN_MEMORY") {
    return jsi::Value(SQLITE_OPEN_MEMORY);
  }
  if (prop == "OPEN_NOMUTEX") {
    return jsi::Value(SQLITE_OPEN_NOMUTEX);
  }
  if (prop == "OPEN_FULLMUTEX") {
    return jsi::Value(SQLITE_OPEN_FULLMUTEX);
  }

  return jsi::Value::undefined();
}

bool Sqlite::isBusy() {
  std::lock_guard lock(m_mdatabases);

  for (auto database : m_databases) {
    if (database->isBusy()) {
      return true;
    }
  }

  return false;
}

jsi::Function Sqlite::createOpen(jsi::Runtime &rt) {
  return jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forUtf8(rt, "open"), 2,
      [&](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *args,
          size_t count) {
        if (count != 2) {
          throw jsi::JSError(
              rt, sqliteJSIBadArgumentsError(rt, "invalid arguments"));
        }

        auto name = args[0].asString(rt).utf8(rt);
        auto flags = (int)args[1].asNumber();

        auto db = std::make_shared<Database>(rt, name, flags, m_invoker);

        std::lock_guard lock(m_mdatabases);
        m_databases.push_back(db);

        return jsi::Object::createFromHostObject(rt, db);
      });
}

} // namespace sqlitejsi
