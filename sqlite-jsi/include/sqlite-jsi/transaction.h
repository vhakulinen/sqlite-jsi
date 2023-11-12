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

  void queue(WorkItem work);

private:
  std::mutex m_m;
  std::deque<WorkItem> m_workitems;
  std::condition_variable m_cv;
  bool m_done;
  bool m_workerdone;

  void done();

  bool workerdone();

  void worker();
};

class Transaction : public jsi::HostObject {
public:
  Transaction(std::shared_ptr<Database> db)
      : m_db(db), m_txexecutor(std::make_shared<TransactionExecutor>()) {}

  ~Transaction() {
    assert(m_txexecutor->workerdone() &&
           "transaction executor should be done on dtor");
  }

  jsi::Value get(jsi::Runtime &rt, const jsi::PropNameID &name) override;

  /// Begin puts the transaction's own executor to the provided executor's
  /// queue, blocking it until the trasnaction completes.
  void begin(Executor &executor);
  /// Ends the transaction's own exectuor, returning the control back to the
  /// original executor's loop.
  void end();

  jsi::Value sqlExec(jsi::Runtime &rt, const jsi::Value *args, size_t count);
  jsi::Value sqlSelect(jsi::Runtime &rt, const jsi::Value *args, size_t count);
  jsi::Value sqlGet(jsi::Runtime &rt, const jsi::Value *args, size_t count);

private:
  std::shared_ptr<Database> m_db;
  std::shared_ptr<TransactionExecutor> m_txexecutor;
};
} // namespace sqlitejsi
