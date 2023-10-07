#include "jsi/jsi.h"
#include "sqlite3.h"

#include "sqlite-jsi/value.h"

namespace sqlitejsi {

// From cppreference.com's std::visit docs.
template <class> inline constexpr bool always_false_v = false;

Value Value::fromJsi(jsi::Runtime &rt, const jsi::Value &val) {

  if (val.isNull() || val.isUndefined()) {
    return Value();
  }

  if (val.isString()) {
    return Value(val.asString(rt).utf8(rt));
  }

  if (val.isNumber()) {
    return Value(val.asNumber());
  }

  if (val.isObject()) {
    auto obj = val.asObject(rt);

    if (obj.isArrayBuffer(rt)) {
      auto buf = obj.getArrayBuffer(rt);
      auto data = buf.data(rt);
      auto size = buf.size(rt);

      return Value(std::vector(data, data + size));
    }
  }

  // TODO(ville): Use dedicated error value
  throw jsi::JSError("unsupported type", rt, jsi::Value(rt, val));
}

std::vector<Value> Value::fromJsiArgs(jsi::Runtime &rt, const jsi::Value *args,
                                      size_t count) {
  std::vector<Value> vals = {};
  vals.reserve(count);

  for (size_t i = 0; i < count; i++) {
    vals.push_back(fromJsi(rt, (const jsi::Value &)args[i]));
  }

  return vals;
}

jsi::Value Value::toJsi(jsi::Runtime &rt) {
  jsi::Value val = std::visit(
      [&](auto arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>) {
          return jsi::Value(arg);
        } else if constexpr (std::is_same_v<T, double>) {
          return jsi::Value(arg);
        } else if constexpr (std::is_same_v<T, std::string>) {
          return jsi::Value(jsi::String::createFromUtf8(rt, arg));
        } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
          // TODO(ville): Would be nice to not have to copy.
          auto buf = std::make_shared<Buffer>(arg);
          return jsi::Value(jsi::ArrayBuffer(rt, buf));
        } else if constexpr (std::is_same_v<T, std::monostate>) {
          return jsi::Value::null();
        } else {
          static_assert(always_false_v<T>, "non-exhaustive visitor");
        }
      },
      m_val);

  return val;
}

int Value::bind(sqlite3_stmt *stmt, int pos) {
  assert(pos > 0); // SQLite bind positions start at 1.

  return std::visit(
      [&](auto arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>) {
          return sqlite3_bind_int(stmt, pos, arg);
        } else if constexpr (std::is_same_v<T, double>) {
          return sqlite3_bind_double(stmt, pos, arg);
        } else if constexpr (std::is_same_v<T, std::string>) {
          return sqlite3_bind_text(stmt, pos, arg.c_str(), -1,
                                   SQLITE_TRANSIENT);
        } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
          return sqlite3_bind_blob(stmt, pos, arg.data(), arg.size(),
                                   SQLITE_TRANSIENT);
        } else if constexpr (std::is_same_v<T, std::monostate>) {
          return sqlite3_bind_null(stmt, pos);
        } else {
          static_assert(always_false_v<T>, "non-exhaistive visitor");
        }
      },
      m_val);
}
} // namespace sqlitejsi
