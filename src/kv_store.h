#pragma once
#include <string>
#include "wal.h"
#include <unordered_map>

class KVStore
{
public:
    KVStore();
    ~KVStore();
    void loadDatabase(); // Loads in the database from the disk
    void saveDatabase(); // Saves any new changes
    bool exists(const std::string &); // Checks if the key exists in the database
    void set(const std::string &, const std::string &, bool log = true); // Sets a key and value in the database
    std::string get(const std::string &); // Returns a value associated with a key from the database
    bool del(const std::string &, bool log = true); // Deletes a key and its value
    std::string execute(const std::string &); // Runs GET, SET, or DEL depending on user input
    bool parseCommand(const std::string& line, LogRecord& rec, std::string& error);
    void checkpoint();
    void recover();


private:
    std::unordered_map<std::string, std::string> database;
    WAL wal;
    int writeCount = 0;
};
