#include <iostream>
#include <mutex>
#include <thread>

#include "sqlite-jsi/executor.h"

namespace sqlitejsi {
using namespace sqlitejsi;

Executor::Executor() : m_exit(false), m_busy(false) {
  m_thread = std::thread([&] { worker(); });
};

Executor::~Executor() {
  std::unique_lock lock(m_m);
  m_exit = true;
  m_cv.notify_all();
  lock.unlock();

  if (m_thread.joinable()) {
    m_thread.join();
  }
}

bool Executor::isBusy() {
  std::lock_guard lock(m_m);
  // We're busy if were executing some callback or have work queued for us.
  return m_busy || m_workitems.size() > 0;
}

void Executor::worker() {
  for (;;) {
    std::unique_lock<std::mutex> lock(m_m);
    m_cv.wait(lock, [&] { return !m_workitems.empty() || m_exit; });

    if (m_exit) {
      break;
    }

    // Take the item and remove it from the que.
    auto work = m_workitems.front();
    m_workitems.pop_front();
    // Mark us busy.
    m_busy = true;
    lock.unlock();

    work();

    lock.lock();
    // Not busy any more.
    m_busy = false;
    lock.unlock();
  }
}

void Executor::queue(WorkItem work) {
  std::lock_guard lock(m_m);

  // TODO(ville): Check m_exit.

  m_workitems.push_back(work);
  m_cv.notify_all();
}

} // namespace sqlitejsi
