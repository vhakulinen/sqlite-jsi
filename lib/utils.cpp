#include "sqlite-jsi/utils.h"
#include <iostream>

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

jsi::Object sqliteJSINoRowsError(jsi::Runtime &rt) {
  auto err = rt.global()
                 .getPropertyAsFunction(rt, "SQLiteJSINoRowsError")
                 .callAsConstructor(rt)
                 .asObject(rt);
  err.setProperty(rt, "msg", "no rows");

  return err;
}

jsi::Object sqliteJSITooManyRows(jsi::Runtime &rt) {
  auto err = rt.global()
                 .getPropertyAsFunction(rt, "SQLiteJSITooManyRowsError")
                 .callAsConstructor(rt)
                 .asObject(rt);
  err.setProperty(rt, "msg", "too many rows");

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

  rt.evaluateJavaScript(
      std::make_shared<jsi::StringBuffer>("function SQLiteJSINoRowsError(){}"),
      "<eval>");

  rt.evaluateJavaScript(std::make_shared<jsi::StringBuffer>(
                            "function SQLiteJSITooManyRowsError(){}"),
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
