# KV Store

A high-performance key-value store written in C++ as part of a systems engineering roadmap. The project is being built incrementally, adding networking, concurrency, persistence, storage internals, and distributed systems features over time.

## Features

Current functionality:

- In-memory key-value storage using `std::unordered_map`
- TCP client/server architecture using POSIX sockets
- Thread pool for concurrent client handling
- Persistent storage using POSIX file I/O (`open`, `read`, `write`, `fsync`)
- Supports the following commands:
  - `SET key value`
  - `GET key`
  - `DEL key`

## Example

```text
SET name Arjun
DONE.

GET name
Arjun

DEL name
DONE.
```

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running

Start the server:

```bash
./server
```

In another terminal, start the client:

```bash
./client localhost
```

## Implementation

- Loads all key-value pairs from `database.db` into memory when the server starts.
- Rewrites `database.db` after every successful `SET` and `DEL`.
- Uses `fsync()` to flush writes to disk before closing the file.

## Technologies

- C++17
- POSIX sockets
- POSIX file I/O
- CMake
- `std::thread`
- `std::mutex`
- `std::condition_variable`

## Future Improvements

- Write-ahead logging (WAL)
- Binary storage format
- Snapshotting
- Crash recovery
- Replication between multiple nodes
