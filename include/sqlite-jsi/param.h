#pragma once

#include <string>

#include "jsi/jsi.h"
#include "sqlite3.h"

namespace sqlitejsi {

using namespace facebook;

enum ValueType {
  // null, undefined
  Null,
  // string
  String,
  // number
  Number,
};

class Param final {
public:
  Param(std::string val) : m_type(ValueType::String), m_string(val){};
  Param(double val) : m_type(ValueType::Number), m_number(val){};
  Param() : m_type(ValueType::Null){};

  static std::vector<Param> parseJsi(jsi::Runtime &rt, const jsi::Value *args,
                                     size_t count);

  int bind(sqlite3_stmt *stmt, int pos);

private:
  ValueType m_type;
  std::string m_string;
  double m_number;
};

} // namespace sqlitejsi
