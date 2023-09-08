#include "sqlite3.h"

#include "sqlite-jsi/connection.h"

namespace sqlitejsi {
using namespace sqlitejsi;

ConnectionGuard Connection::lock() { return ConnectionGuard(*this); }

ConnectionGuard conn_guard(Connection &conn) { return ConnectionGuard(conn); }

} // namespace sqlitejsi
