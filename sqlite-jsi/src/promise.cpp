#include "sqlite-jsi/promise.h"

namespace sqlitejsi {
using namespace facebook;

void Promise::resolve(jsi::Runtime &rt, jsi::Value &&v) {
  m_resolve.asObject(rt).asFunction(rt).call(rt, v);
};
void Promise::reject(jsi::Runtime &rt, jsi::Value &&v) {
  m_reject.asObject(rt).asFunction(rt).call(rt, v);
};

jsi::Value Promise::createPromise(
    jsi::Runtime &rt,
    std::function<void(jsi::Runtime &rt, std::shared_ptr<Promise>)> fn) {
  auto ctor = rt.global().getPropertyAsFunction(rt, "Promise");

  auto exec = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "PromiseExecutor"), 2,
      [&](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *args,
          size_t count) {
        if (count < 2) {
          throw jsi::JSError(rt, "Invalid promise constructor");
        }

        auto promise = std::make_shared<Promise>(jsi::Value(rt, args[0]),
                                                 jsi::Value(rt, args[1]));

        fn(rt, promise);

        return jsi::Value::undefined();
      });

  return ctor.callAsConstructor(rt, exec);
};

jsi::Value Promise::staticReject(jsi::Runtime &rt, const jsi::Value &value) {
  return rt.global()
      .getProperty(rt, "Promise")
      .asObject(rt)
      .getPropertyAsFunction(rt, "reject")
      .call(rt, value);
}

jsi::Value Promise::staticResolve(jsi::Runtime &rt, const jsi::Value &value) {
  return rt.global()
      .getProperty(rt, "Promise")
      .asObject(rt)
      .getPropertyAsFunction(rt, "resolve")
      .call(rt, value);
}

} // namespace sqlitejsi
