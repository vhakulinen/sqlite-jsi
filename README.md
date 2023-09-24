***NOTE***: At the moment this is just a code dump from a couple weekends'
worth of rage coding. I've little to no experience on cpp, hence the code might
not look that idiomatic cpp.

# SQLite jsi

In progress jsi bindings for sqlite, born out from frustration.

While sqlite-jsi is not yet packaged for react-native, this is what the api
looks like:

```javascript
// Open database connection.
//
// Each connection has a dedicated background thread to avoid blocking the UI
// thread. Synchronous variant of the API is not yet implemented.
const db = SQLite.open('foo.db', SQLite.OPEN_READWRITE | SQLite.OPEN_CREATE);

// Execute arbitrary.
await db.exec(`
    CREATE TABLE t1 (v INTEGER NOT NULL);

    INSERT INTO t1 (v) VALUES (1), (2), (2);
`)

// Create prepared statements.
const stmt = await db.prepare('SELECT v FROM t1 WHERE v = ?');

// Get single row.
const row1 = await stmt.get(1);

// Select multiple rows.
const rows = await stmt.select(2);

// Or just execute without retrieving any rows.
await stmt.execute(3);

// Because of the async nature, doing transactions is more involved.
//
// While the following is not yet implemented, transactions would look something
// like this:
db.trasnaction(tx => 
    // tx is another database connection which lives for the duration of the
    // following promise. SQlite is trusted to handle concurrency (at least
    // with the async api).
    tx.exec('....')
)
```
# Code

The repo is split in two:

* sqlite-jsi, the binding library
* test-cli, a testing tool

sqlite-jsi is the part that is supposed to go in a react-native application,
while the test-cli is a (hermes) js interpreter to run basic test scripts.

See the test script (i.e. `./test.sh`) for running the test-cli.

## Building and running

The build process could use some work.

```
$ # Install dependecies, see https://hermesengine.dev/docs/building-and-running
$ mkdir build
$ (cd build && cmake -G Ninja ..)
$ # First build the hermes-engine, which provides libhermes targer for our test-cli.
$ (cd build && cmake --build . --target hermes-engine)
$ # Build the test-cli, which includes the sqlite-jsi target.
$ (cd build && cmake --build .)
$ # Run the tests.
$ bash test.sh
```
