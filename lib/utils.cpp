#include "sqlite-jsi/utils.h"
#include <iostream>

#define C_STRING(x) #x

namespace sqlitejsi {
using namespace facebook;

jsi::Object sqliteError(jsi::Runtime &rt, int code, std::string msg) {
  auto err = rt.global()
                 .getPropertyAsFunction(rt, "SQLiteError")
                 .callAsConstructor(rt)
                 .asObject(rt);
  err.setProperty(rt, "code", code);
  err.setProperty(rt, "msg", msg);

  return err;
}

jsi::Object sqliteJSIBadQueryError(jsi::Runtime &rt, std::string msg) {
  auto err = rt.global()
                 .getPropertyAsFunction(rt, "SQLiteJSIBadQueryError")
                 .callAsConstructor(rt)
                 .asObject(rt);
  err.setProperty(rt, "msg", msg);

  return err;
}

jsi::Object sqliteJSIBadArgumentsError(jsi::Runtime &rt, std::string msg) {
  auto err = rt.global()
                 .getPropertyAsFunction(rt, "SQLiteJSIBadArgumentsError")
                 .callAsConstructor(rt)
                 .asObject(rt);
  err.setProperty(rt, "msg", msg);

  return err;
}
void installUtils(jsi::Runtime &rt) {
  // Errors from sqlite3.
  rt.evaluateJavaScript(
      std::make_shared<jsi::StringBuffer>("function SQLiteError(){}"),
      "<eval>");

  rt.evaluateJavaScript(std::make_shared<jsi::StringBuffer>(
                            "function SQLiteJSIBadQueryError(){}"),
                        "<eval>");

  rt.evaluateJavaScript(std::make_shared<jsi::StringBuffer>(
                            "function SQLiteJSIBadArgumentsError(){}"),
                        "<eval>");
}

jsi::Function createToString(jsi::Runtime &rt, const jsi::PropNameID &name,
                             std::string str) {
  return jsi::Function::createFromHostFunction(
      rt, name, 0,
      [str](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *args,
            size_t count) {
        return jsi::Value(rt, jsi::String::createFromUtf8(rt, str));
      });
}

} // namespace sqlitejsi
