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

str().then(integer).then(_float).then(_null).then(_undefined)
