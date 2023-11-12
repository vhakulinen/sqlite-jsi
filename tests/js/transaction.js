print('begin');
// CHECK-LABEL: begin
const db = SQLite.open(':memory:', SQLite.OPEN_READWRITE);

const init = () => db.exec('CREATE TABLE txtable (col1 INTEGER, col2 TEXT)');

// Transaction commits when the body doesn't throw.
const commits = () => {
  print("commits:");
  return db.transaction(tx => {
    return tx.exec(`
      INSERT INTO txtable (col1, col2)
      VALUES
        (1, 'tx1')
    `).then(() => tx.exec(`
      INSERT INTO txtable (col1, col2)
        VALUES
          (2, 'tx1')
    `));
  }).then(() =>
    db.select("SELECT * FROM txtable WHERE col2 = 'tx1' ORDER BY col1")
      .then(rows => print('commits:', JSON.stringify(rows)))
  ).catch(err => print('ERROR!', err));
}
// CHECK-LABEL: commits:
// CHECK-NEXT: commits: [{"col1":1,"col2":"tx1"},{"col1":2,"col2":"tx1"}]

// Transaction resolves to the returned value.
const commitResolves = () => {
  print("commit resolves:")
  return db.transaction(_tx => 'resolve value')
    .then(res => print('transaction resolved to:', JSON.stringify(res)))
}
// CHECK-LABEL: commit resolves:
// CHECK-NEXT: transaction resolved to: "resolve value"

// Transaction rejects to the returned value.
const commitRejects = () => {
  print("commit rejects:")
  return db.transaction(_tx => Promise.reject('reject value'))
    .catch(res => print('transaction rejected to:', JSON.stringify(res)))
}
// CHECK-LABEL: commit rejects:
// CHECK-NEXT: transaction rejected to: "reject value"

const select = () => {
  print("select:")
  return db.transaction(tx => tx.select('SELECT ? as c', 'select value'))
    .then(res => print('transaction select:', JSON.stringify(res)))
}
// CHECK-LABEL: select:
// CHECK-NEXT: transaction select: [{"c":"select value"}]

const get = () => {
  print("get:")
  return db.transaction(tx => tx.get('SELECT ? as c', 'get value'))
    .then(res => print('transaction get:', JSON.stringify(res)))
}
// CHECK-LABEL: get:
// CHECK-NEXT: transaction get: {"c":"get value"}

const preparedStatements = () => {
  print("prepared statements:")
  const p1 = db.prepare('SELECT ? as p')
    .then(stmt => Promise.all([
      db.transaction(tx => tx.select(stmt, 'prepared select'))
        .then(res => print('transaction prepared select:', JSON.stringify(res))),
      db.transaction(tx => tx.get(stmt, 'prepared get'))
        .then(res => print('transaction prepared get:', JSON.stringify(res))),
    ]));

  const p2 = db.prepare('INSERT INTO txtable (col1, col2) VALUES (?, ?)')
    .then(stmt => db.transaction(tx => tx.exec(stmt, 30, 'tx prepared')))
    .then(() =>
      db.select("SELECT * FROM txtable WHERE col2 = 'tx prepared'")
        .then(res => print('transaction prepared exec:', JSON.stringify(res)))
    )

  return Promise.all([p1, p2]);
}
// CHECK-LABEL: prepared statements:
// CHECK-NEXT: transaction prepared select: [{"p":"prepared select"}]
// CHECK-NEXT: transaction prepared get: {"p":"prepared get"}
// CHECK-NEXT: transaction prepared exec: [{"col1":30,"col2":"tx prepared"}]

// Transaction rolls back if the body rejects.
const rollbacks = () => {
  print("rollbacks:")
  return db.transaction(tx =>
    tx.exec(`INSERT INTO txtable (col1, col2) VALUES (3, 'throws')`)
      .then(() => Promise.reject('reject in transaction'))
  ).finally(() =>
    db.select("SELECT * FROM txtable WHERE col2 = 'throws'")
      .then(rows => print('rows after threw', JSON.stringify(rows)))
  ).catch(err => print('threw', err));
}
// CHECK-LABEL: rollbacks:
// CHECK-NEXT: rows after threw []
// CHECK-NEXT: threw reject in transaction

const blockOthers = () => {
  print("block others:")
  let p = null;
  const ptx = db.transaction(tx => {
    // While transaction is going, if we do queries throught the db object,
    // they shouldn't start before the transaction completes.
    p = db.select("SELECT col1, col2 FROM txtable WHERE col2 = 'blocked'")
      .then(rows => print('non-tx query:', JSON.stringify(rows)));

    return tx.exec(`
      INSERT INTO txtable (col1, col2)
      VALUES (33, 'blocked'),
             (34, 'blocked')
    `).then(() => tx.select("SELECT col1, col2 FROM txtable WHERE col2 = 'blocked'"))
      .then(rows => {
        print('tx values:', JSON.stringify(rows))
        return tx.exec("DELETE FROM txtable WHERE col2 = 'blocked' AND col1 = 33")
          .then(() => print('tx deleted'));
      });
  }).catch(err => print("ERROR!", JSON.stringify(err)));

  return Promise.all([ptx, p]);
}
// CHECK-LABEL: block others:
// CHECK-NEXT: tx values: [{"col1":33,"col2":"blocked"},{"col1":34,"col2":"blocked"}]
// CHECK-NEXT: tx deleted
// CHECK-NEXT: non-tx query: [{"col1":34,"col2":"blocked"}]

init()
  .then(commits)
  .then(commitResolves)
  .then(commitRejects)
  .then(select)
  .then(get)
  .then(preparedStatements)
  .then(rollbacks)
  .then(blockOthers)
  .then(() => print("done."))
  .catch(err => print("OH NO", err));
// CHECK-NEXT: done.
