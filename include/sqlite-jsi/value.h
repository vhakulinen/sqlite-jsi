#pragma once

#include <string>
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
    std::vector<char>,
    // sqlite NULL, js null & undefined
    std::monostate>
    ValueType;

class Value final {
public:
  Value(std::string val) : m_val(ValueType(val)){};
  Value(double val) : m_val(ValueType(val)){};
  Value(std::vector<char> val) : m_val(ValueType(val)){};
  Value() : m_val(ValueType(std::monostate())){};

  static Value fromJsi(jsi::Runtime &rt, const jsi::Value &arg);

  static std::vector<Value> fromJsiArgs(jsi::Runtime &rt,
                                        const jsi::Value *args, size_t count);

  jsi::Value toJsi(jsi::Runtime &rt);

  int bind(sqlite3_stmt *stmt, int pos);

private:
  ValueType m_val;
};

} // namespace sqlitejsi
