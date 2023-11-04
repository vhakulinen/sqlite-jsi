#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <thread>

#include "sqlite-jsi/executor.h"

namespace sqlitejsi {
using namespace sqlitejsi;

using WorkItem = std::function<void()>;

class Executor {
public:
  Executor();
  ~Executor();

  void queue(WorkItem work);

  bool isBusy();

private:
  std::mutex m_m;
  std::deque<WorkItem> m_workitems;
  std::thread m_thread;
  bool m_exit;
  bool m_busy;

  std::condition_variable m_cv;

  void worker();
};
} // namespace sqlitejsi
