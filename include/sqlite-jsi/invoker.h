#pragma once

#include <functional>

#include "jsi/jsi.h"

namespace sqlitejsi {

using namespace facebook;

using RtExec = std::function<void(jsi::Runtime &)>;
/**
 * Invoker to gain access to the run time from another thread.
 */
using RtInvoker = std::function<void(RtExec)>;
} // namespace sqlitejsi
