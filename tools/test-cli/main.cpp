#include <fstream>
#include <iostream>
#include <sstream>

#include "hermes/hermes.h"

#include "sqlite-jsi/invoker.h"
#include "sqlite-jsi/sqlite-jsi.h"

using namespace facebook;

auto invoke_callbacks = std::make_shared<std::deque<sqlitejsi::RtExec>>();
std::mutex mutex_invoke_callbacks;

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "usage: prg <file>";
    return 1;
  }

  std::ifstream file(argv[1]);
  std::stringstream buf;
  buf << file.rdbuf();

  auto jsiBuffer =
      std::make_shared<jsi::StringBuffer>(jsi::StringBuffer(buf.str()));

  std::unique_ptr<jsi::Runtime> rt(facebook::hermes::makeHermesRuntime());

  std::shared_ptr<sqlitejsi::RtInvoker> invoker =
      std::make_shared<sqlitejsi::RtInvoker>([&](sqlitejsi::RtExec cb) {
        std::lock_guard<std::mutex> gurad(mutex_invoke_callbacks);
        invoke_callbacks->push_back(cb);
      });

  rt->global().setProperty(
      *rt, "setImmediate",
      jsi::Function::createFromHostFunction(
          *rt, jsi::PropNameID::forUtf8(*rt, "setImmediate"), 1,
          [&](jsi::Runtime &rt, const jsi::Value &thisVal,
              const jsi::Value *args, size_t count) {
            auto fn = std::make_shared<jsi::Value>(rt, args[0]);
            (*invoker)([fn](jsi::Runtime &rt) {
              fn->asObject(rt).asFunction(rt).call(rt);
            });
            // TODO(ville): Actual IDs?
            return jsi::Value::undefined();
          }));

  auto sqlite = sqlitejsi::Sqlite::install(*rt, invoker);

  auto js = rt->prepareJavaScript(jsiBuffer, "<eval>");

  rt->evaluatePreparedJavaScript(js);

  for (;;) {
    std::unique_lock<std::mutex> lock(mutex_invoke_callbacks, std::defer_lock);
    lock.lock();

    sqlitejsi::RtExec fn = nullptr;
    if (!invoke_callbacks->empty()) {
      fn = invoke_callbacks->front();
      invoke_callbacks->pop_front();
    }
    lock.unlock();

    // We must've ran out of things to do.
    if (fn == nullptr) {
      if (sqlite->isBusy()) {
        // Some database is still doing some work.
        continue;
      }

      break;
    }

    fn(*rt);
  }

  return 0;
}
