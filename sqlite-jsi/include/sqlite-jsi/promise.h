#pragma once

#include "jsi/jsi.h"

namespace sqlitejsi {
using namespace facebook;

class Promise {
public:
  Promise(jsi::Value &&resolve, jsi::Value &&reject)
      : m_resolve(std::move(resolve)), m_reject(std::move(reject)){};

  void resolve(jsi::Runtime &rt, jsi::Value &&v);

  void reject(jsi::Runtime &rt, jsi::Value &&v);

  static jsi::Value createPromise(
      jsi::Runtime &rt,
      std::function<void(jsi::Runtime &rt, std::shared_ptr<Promise>)> fn);

  static jsi::Value staticReject(jsi::Runtime &rt, const jsi::Value &value);
  static jsi::Value staticResolve(jsi::Runtime &rt, const jsi::Value &value);

private:
  jsi::Value m_resolve;
  jsi::Value m_reject;
};

} // namespace sqlitejsi
