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

class Param final {
public:
  Param(std::string val) : m_val(ValueType(val)){};
  Param(double val) : m_val(ValueType(val)){};
  Param(std::vector<char> val) : m_val(ValueType(val)){};
  Param() : m_val(ValueType(std::monostate())){};

  static Param fromJsi(jsi::Runtime &rt, const jsi::Value &arg);

  static std::vector<Param> fromJsiArgs(jsi::Runtime &rt,
                                        const jsi::Value *args, size_t count);

  jsi::Value toJsi(jsi::Runtime &rt);

  int bind(sqlite3_stmt *stmt, int pos);

private:
  ValueType m_val;
};

} // namespace sqlitejsi
