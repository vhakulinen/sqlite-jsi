const error = err => {
  print("error: ", JSON.stringify(err));
}

const db = SQLite.open(":memory:", SQLite.OPEN_READWRITE);

// Execute multiple statements in one go.
db.exec(`
  CREATE TABLE exec (v TEXT NOT NULL, w INTEGER);
  INSERT INTO exec (v, w) VALUES ('foo', NULL), ('bar', 5);
`).then(() => {
  print("exec done");

  return db.select(`SELECT v, w FROM exec ORDER BY v`)
    .then(rows => {
      print(JSON.stringify(rows));
    })
    .catch(error)
}).catch(error)
  .finally(() => print('done'));
// CHECK: exec done
// CHECK-NEXT: [{"v":"bar","w":5},{"v":"foo","w":null}]
// CHECK-NEXT: done
