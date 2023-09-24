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
        .then(row => print("only one row of many", JSON.stringify(row)))
        .then(() => stmt))
    .then(stmt =>
      stmt.get(1, 2, 3, 5)
        .catch(err => print("too many arguments", err instanceof SQLiteError, err.code))
        .then(() => stmt)
    );
}
// CHECK-NEXT: get with params {"v":"v2","i":20}
// CHECK-NEXT: get no rows true
// CHECK-NEXT: only one row of many {"v":"v3","i":10}
// CHECK-NEXT: too many arguments true 25

const getInsert = () => {
  return db.exec(`
    CREATE TABLE getinsert (v INTEGER NOT NULL);
    INSERT INTO getinsert (v) VALUES (1), (2);
  `)
    .then(() => db.prepare(`INSERT INTO getinsert VALUES (?), (?) RETURNING v`))
    // This should still insert all the rows, while we only get the first row.
    .then(stmt => stmt.get(3, 4))
    .then(row => print('getinsert row', JSON.stringify(row)))
    .then(() => db.select(`SELECT v FROM getinsert ORDER BY v`))
    .then(rows => print("getinsert all rows", JSON.stringify(rows)))
}
// CHECK-NEXT: getinsert row {"v":3}
// CHECK-NEXT: getinsert all rows [{"v":1},{"v":2},{"v":3},{"v":4}]

start()
  .then(select)
  .then(get)
  .then(getInsert)
  .catch(err => {
    print("failure", err, JSON.stringify(err));
  });
