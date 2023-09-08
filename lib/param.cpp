#include "jsi/jsi.h"
#include "sqlite3.h"

#include "sqlite-jsi/param.h"

namespace sqlitejsi {

// using namespace facebook;
// using namespace sqlitejsi;

Param parse(jsi::Runtime &rt, const jsi::Value &val) {

  if (val.isNull() || val.isUndefined()) {
    return Param();
  }

  if (val.isString()) {
    return Param(val.asString(rt).utf8(rt));
  }

  if (val.isNumber()) {
    return Param(val.asNumber());
  }

  // jsi::Value(&val);
  throw jsi::JSError("unsupported type", rt, jsi::Value(rt, val));
  // throw jsi::JSError(rt, "unsupported type");
}

std::vector<Param> Param::parseJsi(jsi::Runtime &rt, const jsi::Value *args,
                                   size_t count) {
  std::vector<Param> vals(count);

  for (size_t i = 0; i < count; i++) {
    vals.push_back(parse(rt, (const jsi::Value &)args[i]));
  }

  return vals;
}

void Param::bind(sqlite3_stmt *stmt, int pos) {
  switch (m_type) {
  case Null: {
    sqlite3_bind_null(stmt, pos);
  } break;
  case String: {
    sqlite3_bind_text(stmt, pos, m_string.c_str(), -1, SQLITE_TRANSIENT);
  } break;
  case Number: {
    sqlite3_bind_double(stmt, pos, m_number);
  } break;
  }
}
} // namespace sqlitejsi
