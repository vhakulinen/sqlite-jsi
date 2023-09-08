const unreachable = () => {
  throw 'unreachable';
}

const db = SQLite.open(":memory:", SQLite.OPEN_READWRITE);

const start = () => db.exec(`
  CREATE TABLE t1 (v TEXT NOT NULL, i INTEGER);
  INSERT INTO t1 (v, i) VALUES ('v1', NULL), ('v2', 20);
`).then(() => {
  print("exec done");
}).then(() => selectNoParams())
  .catch(err => {
    print("failure", JSON.stringify(err));
  })
// CHECK: exec done

const selectNoParams = () => {
  print("no params");
  return db.prepare('SELECT v, i FROM t1 ORDER BY v')
    .then(stmt => {
      stmt.select()
        .then(rows => {
          print("no params results", JSON.stringify(rows));
        });
    });
}
// CHECK-NEXT: no params
// CHECK-NEXT: no params results [{"v":"v1","i":null},{"v":"v2","i":20}]


start();
