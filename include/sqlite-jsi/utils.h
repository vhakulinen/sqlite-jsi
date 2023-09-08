#pragma once

#include "jsi/jsi.h"

namespace sqlitejsi {
using namespace facebook;

jsi::Object sqliteError(jsi::Runtime &rt, int code, std::string msg);

jsi::Object sqliteJSIBadQueryError(jsi::Runtime &rt, std::string msg);

jsi::Object sqliteJSIBadArgumentsError(jsi::Runtime &rt, std::string msg);

jsi::Function createToString(jsi::Runtime &rt, const jsi::PropNameID &name,
                             std::string str);

void installUtils(jsi::Runtime &rt);

} // namespace sqlitejsi
