const error = err => print("error", err, JSON.stringify(err));

const db = SQLite.open(":memory:", SQLite.OPEN_READWRITE);

const str = () => db.select('SELECT ? as val', 'my string')
  .then(res => print("string", JSON.stringify(res)))
  .catch(error)
// CHECK: string [{"val":"my string"}]

const integer = () => db.select('select ? as val', 32)
  .then(res => print("integer", JSON.stringify(res)))
  .catch(error)
// CHECK-NEXT: integer [{"val":32}]

const _float = () => db.select('select ? as val', 9.2)
  .then(res => print("float", JSON.stringify(res)))
  .catch(error)
// CHECK-NEXT: float [{"val":9.2}]

const _null = () => db.select('select ? as val', null)
  .then(res => print("null", JSON.stringify(res)))
  .catch(error)
// CHECK-NEXT: null [{"val":null}]

const _undefined = () => db.select('select ? as val', undefined)
  .then(res => print("undefined", JSON.stringify(res)))
  .catch(error)
// CHECK-NEXT: undefined [{"val":null}]

const blob = () => {
  const buf = new Int8Array(4);
  buf[0] = 5;
  buf[1] = 6;
  buf[2] = 7;
  buf[3] = 8;

  return db.select('select ? as val', buf.buffer)
    .then(res => print('blob', JSON.stringify(res.map(v => v.val = new Int8Array(v.val)))))
    .catch(error)
}
// CHECK-NEXT: blob [{"0":5,"1":6,"2":7,"3":8}]

str().then(integer).then(_float).then(_null).then(_undefined).then(blob)
