#include "kv_store.h"
#include <unordered_map>

std::unordered_map<std::string, std::string> database;


void set(const std::string& key, const std::string& value) {
    database[key] = value;
}

std::string get(const std::string& key) {
    return database[key];
}


bool del(const std::string& key) {
    if (database.find(key) == database.end()) {
        return false;
    }
    database.erase(key);
    return true;
}

bool exists(const std::string& key) {
    return database.find(key) != database.end();
}