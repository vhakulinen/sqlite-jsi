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

// Transaction blocks (or "steals") the db's background thread and only allows
// queries through the tx object to be executed until the provided promise
// resolves.
//
// Throwing or returning rejected promise will cause a rollback.
db.trasnaction(async tx => {
    // i.e. this works.
    await tx.exec('...');
    // But this would deadlock.
    // await db.exec('...');


    // The promise will resolve to the returned value.
    return tx.select('...');
})
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
$ # Build the test-cli and FileCheck for running the tests.
$ (cd build && cmake --build --target test-cli --target FileCheck)
$ # Run the tests.
$ bash test.sh
```
