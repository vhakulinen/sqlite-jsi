#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <thread>

#include "jsi/jsi.h"

namespace sqlitejsi {
using namespace sqlitejsi;
using namespace facebook;

using WorkItem = std::function<void()>;

class Executor {
public:
  Executor();
  ~Executor();

  void queue(jsi::Runtime &rt, WorkItem work);

  bool isBusy();

private:
  std::mutex m_m;
  std::deque<WorkItem> m_workitems;
  std::thread m_thread;
  bool m_exit;

  std::condition_variable m_cv;

  void worker();
};
} // namespace sqlitejsi
