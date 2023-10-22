#pragma once

#include <variant>

#include "jsi/jsi.h"
#include "sqlite3.h"

namespace sqlitejsi {

using namespace facebook;

typedef std::variant<
    // sqlite integer
    int,
    // sqlite float, js number
    double,
    // sqlite text, js string
    std::string,
    // sqlite blob
    std::vector<uint8_t>,
    // sqlite NULL, js null & undefined
    std::monostate>
    ValueType;

/// A brige value between jsi <-> sqlite.
class Value final {
public:
  Value(std::string val) : m_val(ValueType(val)){};
  Value(double val) : m_val(ValueType(val)){};
  Value(std::vector<uint8_t> &&val) : m_val(ValueType(std::move(val))){};
  Value() : m_val(ValueType(std::monostate())){};

  static Value fromJsi(jsi::Runtime &rt, const jsi::Value &arg);

  static std::vector<Value> fromJsiArgs(jsi::Runtime &rt,
                                        const jsi::Value *args, size_t count);

  static Value fromSqlite(sqlite3_stmt *stmt, int i);

  jsi::Value toJsi(jsi::Runtime &rt);

  int bind(sqlite3_stmt *stmt, int pos);

private:
  ValueType m_val;
};

class Buffer : public jsi::MutableBuffer {
public:
  Buffer(std::vector<uint8_t> data) : m_data(data) {}

  size_t size() const override { return m_data.size(); }
  uint8_t *data() override { return m_data.data(); }

private:
  std::vector<uint8_t> m_data;
};

} // namespace sqlitejsi
