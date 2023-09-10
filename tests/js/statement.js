const db = SQLite.open(":memory:", SQLite.OPEN_READWRITE);

const start = () => db.exec(`
  CREATE TABLE t1 (v TEXT NOT NULL, i INTEGER);
  INSERT INTO t1 (v, i)
  VALUES
    ('v1', NULL),
    ('v2', 20),
    ('v3', 10),
    ('v3', 10);
`).then(() => {
  print("exec done");
});
// CHECK: exec done

const select = () => {
  // TODO(ville): with params, no results, bad params
  print("no params");
  return db.prepare("SELECT v, i FROM t1 WHERE v != 'v3' ORDER BY v")
    .then(stmt => stmt.select())
    .then(rows => {
      print("no params results", JSON.stringify(rows));
    });
}
// CHECK-NEXT: no params
// CHECK-NEXT: no params results [{"v":"v1","i":null},{"v":"v2","i":20}]

const get = () => {
  // TODO(ville): without params, bad params
  return db.prepare("SELECT v, i FROM t1 WHERE v = ? AND i = ?")
    .then(stmt =>
      stmt.get('v2', 20)
        .then(row => print("get with params", JSON.stringify(row)))
        .then(() => stmt))
    .then(stmt =>
      stmt.get('v3', 1)
        .catch(err => {
          print('get no rows', err instanceof SQLiteJSINoRowsError)
        })
        .then(() => stmt))
    .then(stmt =>
      stmt.get('v3', 10)
        .catch(err => print('get too many rows', err instanceof SQLiteJSITooManyRowsError)));
}
// CHECK-NEXT: get with params {"v":"v2","i":20}
// CHECK-NEXT: get no rows true
// CHECK-NEXT: get too many rows true

start()
  .then(select)
  .then(get)
  .catch(err => {
    print("failure", err, JSON.stringify(err));
  });
