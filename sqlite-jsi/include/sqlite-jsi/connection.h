#pragma once

#include <mutex>

#include "sqlite3.h"

namespace sqlitejsi {
class ConnectionGuard;

class Connection {
  friend class ConnectionGuard;

public:
  Connection(sqlite3 *conn) : m_conn(conn){};

  ConnectionGuard lock();

private:
  std::mutex m;
  sqlite3 *m_conn;
};

/// RAII guard for Connection.
class ConnectionGuard {
  friend class Connection;

public:
  ConnectionGuard(Connection &conn) : m_conn(conn) { m_conn.m.lock(); };
  ~ConnectionGuard() { m_conn.m.unlock(); };

  sqlite3 *operator&() { return m_conn.m_conn; }

private:
  Connection &m_conn;
};

/// Shorthand for taking ConnectionGuard.
ConnectionGuard conn_guard(Connection &conn);

} // namespace sqlitejsi
