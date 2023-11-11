#pragma once

#include "jsi/jsi.h"

#include "sqlite-jsi/connection.h"
#include "sqlite-jsi/database.h"
#include "sqlite-jsi/executor.h"
#include <cassert>
#include <iostream>

namespace sqlitejsi {
using namespace facebook;
using namespace sqlitejsi;

class TransactionExecutor {
  friend class Transaction;

public:
  TransactionExecutor() : m_done(false), m_workerdone(false) {}
  ~TransactionExecutor() {}

  void queue(WorkItem work) {
    std::lock_guard lock(m_m);
    // TODO(ville): Should probably throw a jsi error instead?
    assert(((void)"can't add work to closed executor", m_done == false));
    m_workitems.push_back(work);
    m_cv.notify_all();
  };

private:
  std::mutex m_m;
  std::deque<WorkItem> m_workitems;
  std::condition_variable m_cv;
  bool m_done;
  bool m_workerdone;

  void done() {
    std::lock_guard lock(m_m);
    m_done = true;
    m_cv.notify_all();
  }

  bool workerdone() {
    std::lock_guard lock(m_m);
    return m_workerdone;
  }

  void worker() {
    for (;;) {
      std::unique_lock<std::mutex> lock(m_m);
      m_cv.wait(lock, [&] { return !m_workitems.empty() || m_done; });

      if (m_workitems.empty()) {
        if (m_done) {
          break;
        }

        assert(((void)"transaction worker woke without work", false));
      }

      auto work = m_workitems.front();
      m_workitems.pop_front();
      lock.unlock();
      work();
    }

    std::lock_guard lock(m_m);
    m_workerdone = true;
  }
};

class Transaction : public jsi::HostObject {
public:
  Transaction(std::shared_ptr<Database> db)
      : m_db(db), m_txexecutor(std::make_shared<TransactionExecutor>()) {}
  ~Transaction() {
    assert(((void)"Transaction executor worker should be done",
            m_txexecutor->workerdone()));
  }

  jsi::Value get(jsi::Runtime &rt, const jsi::PropNameID &name) override;

  /// Begin puts the transaction's own executor to the provided executor's
  /// queue, blocking it until the trasnaction completes.
  void begin(Executor &executor);
  /// Ends the transaction's own exectuor, returning the control back to the
  /// original executor's loop.
  void end();

  jsi::Value sqlExec(jsi::Runtime &rt, const jsi::Value *args, size_t count);
  // jsi::Value sqlSelect(jsi::Runtime &rt, const jsi::Value *args, size_t
  // count);
  // jsi::Value sqlGet(jsi::Runtime &rt, const jsi::Value *args, size_t
  // count);

private:
  std::shared_ptr<Database> m_db;
  std::shared_ptr<TransactionExecutor> m_txexecutor;
};
} // namespace sqlitejsi
