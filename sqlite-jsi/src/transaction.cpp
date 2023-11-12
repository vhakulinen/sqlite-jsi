#include "jsi/jsi.h"

#include "sqlite-jsi/connection.h"
#include "sqlite-jsi/database.h"
#include "sqlite-jsi/executor.h"
#include "sqlite-jsi/promise.h"
#include "sqlite-jsi/statement.h"
#include "sqlite-jsi/transaction.h"
#include "sqlite-jsi/utils.h"
#include <iostream>

namespace sqlitejsi {
using namespace facebook;
using namespace sqlitejsi;

void TransactionExecutor::queue(WorkItem work) {
  std::lock_guard lock(m_m);
  // TODO(ville): Should probably throw a jsi error instead?
  assert(((void)"can't add work to closed executor", m_done == false));
  m_workitems.push_back(work);
  m_cv.notify_all();
};

void TransactionExecutor::done() {
  std::lock_guard lock(m_m);
  m_done = true;
  m_cv.notify_all();
}

bool TransactionExecutor::workerdone() {
  std::lock_guard lock(m_m);
  return m_workerdone;
}

void TransactionExecutor::worker() {
  for (;;) {
    std::unique_lock<std::mutex> lock(m_m);
    m_cv.wait(lock, [&] { return !m_workitems.empty() || m_done; });

    // Consume all queued items.
    while (!m_workitems.empty()) {
      auto work = m_workitems.front();
      m_workitems.pop_front();

      // Unlock when executing the queued work.
      lock.unlock();
      work();

      // Lock again for checking m_workitems and m_done.
      lock.lock();
    }

    // Break the loop if we're done.
    if (m_done) {
      break;
    }
  }

  std::lock_guard lock(m_m);
  m_workerdone = true;
}

#define STATEMENT_FROM_VALUE(query)                                            \
  query.isString()                                                             \
      ? m_db->prepare(                                                         \
                rt, m_txexecutor,                                              \
                std::make_shared<std::string>(query.asString(rt).utf8(rt)))    \
            .asObject(rt)                                                      \
      : Promise::staticResolve(rt, query.asObject(rt)).asObject(rt)

jsi::Value Transaction::get(jsi::Runtime &rt, const jsi::PropNameID &name) {
  auto prop = name.utf8(rt);

  if (prop == "toString") {
    return createToString(rt, name, "Transaction");
  }

  if (prop == "exec") {
    return jsi::Function::createFromHostFunction(
        rt, name, 1,
        [&](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *args,
            size_t count) { return this->sqlExec(rt, args, count); });
  }

  if (prop == "select") {
    return jsi::Function::createFromHostFunction(
        rt, name, 1,
        [&](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *args,
            size_t count) { return this->sqlSelect(rt, args, count); });
  }

  if (prop == "get") {
    return jsi::Function::createFromHostFunction(
        rt, name, 1,
        [&](jsi::Runtime &rt, const jsi::Value &thisVal, const jsi::Value *args,
            size_t count) { return this->sqlGet(rt, args, count); });
  }

  return jsi::Value::undefined();
}

void Transaction::begin(Executor &executor) {
  // Queue our own worker in the executor. I'll block it and allows only us
  // to do any work.
  executor.queue([=, txexecutor = m_txexecutor] { txexecutor->worker(); });
}

void Transaction::end() { m_txexecutor->done(); }

jsi::Value Transaction::sqlExec(jsi::Runtime &rt, const jsi::Value *args,
                                size_t count) {
  if (count < 1) {
    return Promise::staticReject(
        rt, jsi::String::createFromAscii(rt, "invalid arguments"));
  }

  auto promise = STATEMENT_FROM_VALUE(args[0]);

  auto queryParams = std::make_shared<std::vector<jsi::Value>>();
  for (size_t i = 1; i < count; i++) {
    queryParams->push_back(jsi::Value(rt, args[i]));
  }

  auto then = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "then"), 1,
      [queryParams,
       executor = m_txexecutor](jsi::Runtime &rt, const jsi::Value &thisVal,
                                const jsi::Value *args, size_t count) {
        auto stmt = args->asObject(rt).asHostObject<Statement>(rt);
        return stmt->sqlExec(rt, executor, queryParams->data(),
                             queryParams->size());
      });

  return promise.getPropertyAsFunction(rt, "then")
      .callWithThis(rt, promise, then);
};

jsi::Value Transaction::sqlSelect(jsi::Runtime &rt, const jsi::Value *args,
                                  size_t count) {
  if (count < 1) {
    return Promise::staticReject(
        rt, jsi::String::createFromAscii(rt, "invalid arguments"));
  }

  auto promise = STATEMENT_FROM_VALUE(args[0]);

  auto queryParams = std::make_shared<std::vector<jsi::Value>>();
  for (size_t i = 1; i < count; i++) {
    queryParams->push_back(jsi::Value(rt, args[i]));
  }

  auto then = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "then"), 1,
      [queryParams,
       executor = m_txexecutor](jsi::Runtime &rt, const jsi::Value &thisVal,
                                const jsi::Value *args, size_t count) {
        auto stmt = args->asObject(rt).asHostObject<Statement>(rt);
        return stmt->sqlSelect(rt, executor, queryParams->data(),
                               queryParams->size());
      });

  return promise.getPropertyAsFunction(rt, "then")
      .callWithThis(rt, promise, then);
};

jsi::Value Transaction::sqlGet(jsi::Runtime &rt, const jsi::Value *args,
                               size_t count) {
  if (count < 1) {
    return Promise::staticReject(
        rt, jsi::String::createFromAscii(rt, "invalid arguments"));
  }

  auto promise = STATEMENT_FROM_VALUE(args[0]);

  auto queryParams = std::make_shared<std::vector<jsi::Value>>();
  for (size_t i = 1; i < count; i++) {
    queryParams->push_back(jsi::Value(rt, args[i]));
  }

  auto then = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "then"), 1,
      [queryParams,
       executor = m_txexecutor](jsi::Runtime &rt, const jsi::Value &thisVal,
                                const jsi::Value *args, size_t count) {
        auto stmt = args->asObject(rt).asHostObject<Statement>(rt);
        return stmt->sqlGet(rt, executor, queryParams->data(),
                            queryParams->size());
      });

  return promise.getPropertyAsFunction(rt, "then")
      .callWithThis(rt, promise, then);
};
}; // namespace sqlitejsi
