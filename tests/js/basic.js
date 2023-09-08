print(SQLite);
// CHECK: SQLite

try {
  SQLite.open(':memory:')
} catch (err) {
  print(err instanceof SQLiteJSIBadArgumentsError)
}
// CHECK-NEXT: true

const db = SQLite.open(':memory:', SQLite.OPEN_READWRITE);
print(db)
// CHECK-NEXT: Database

// TODO(ville): Should the error be thrown in the promise instead?
try {
  db.exec()
} catch (err) {
  print(err instanceof SQLiteJSIBadArgumentsError)
}
// CHECK-NEXT: true
