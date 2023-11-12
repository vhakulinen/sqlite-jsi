#include "sqlite-jsi/executor.h"

namespace sqlitejsi {
using namespace sqlitejsi;

Executor::Executor() : m_exit(false) {
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

  assert(m_workitems.empty() && "work queue should be empty");
}

bool Executor::isBusy() {
  std::lock_guard lock(m_m);
  // The worker removes items from the queue once its done with them, so we
  // can "just" check the size of the queue.
  return m_workitems.size() > 0;
}

void Executor::worker() {
  for (;;) {
    std::unique_lock<std::mutex> lock(m_m);
    m_cv.wait(lock, [&] { return !m_workitems.empty() || m_exit; });

    while (!m_workitems.empty()) {
      // Take the item and remove it from the que.
      auto work = m_workitems.front();
      lock.unlock();

      work();

      lock.lock();
      // Pop the item from the queue.
      m_workitems.pop_front();
    }

    // If we're done, break.
    if (m_exit) {
      break;
    }
  }
}

void Executor::queue(jsi::Runtime &rt, WorkItem work) {
  std::lock_guard lock(m_m);

  // TODO(ville): Test.
  if (m_exit) {
    throw jsi::JSError(rt, "can't queue work on executor after its done");
  }

  m_workitems.push_back(work);
  m_cv.notify_all();
}

} // namespace sqlitejsi
