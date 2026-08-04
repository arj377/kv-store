#pragma once

#include <optional>
#include <string>

#include "wal.h"
#include "buffer_pool_manager.h"

class KVStore
{
public:
    KVStore();
    ~KVStore();
    std::string execute(const std::string &); // Runs GET, SET, or DEL depending on user input

    bool set(const std::string &, const std::string &, bool log = true); // Sets a key and value in the database
    std::optional<std::string> get(const std::string &); // Returns a value associated with a key from the database
    bool del(const std::string &, bool log = true); // Deletes a key and its value

    void checkpoint();
    void recover();


private:
    bool parseCommand(const std::string& line, LogRecord& rec, std::string& error);
    WAL wal;
    int writeCount = 0;
    BufferPoolManager bufferPoolManager;
};
