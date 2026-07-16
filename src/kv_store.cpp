#include "kv_store.h"
#include <iostream>      
#include <sstream>   
#include <unordered_map>

std::unordered_map<std::string, std::string> database;

void set(const std::string& key, const std::string& value) {
    database[key] = value;
}

std::string get(const std::string& key) {
    return database.at(key); //throws if the key doesn't exist
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

std::string execute(const std::string &input) {
    std::istringstream parser(input);
    std::string command, key, value;

    //creates parser to go through arguments
    parser >> command >> key >> value;
            
    //based on user request, execute proper command
    if (command == "SET") {
        set(key,value);
        return "DONE";
    } 
            
    else if (command == "GET") {
        if (exists(key)) {
            return get(key);
        } else {
            return "Key \"" + key + "\" does not exist\n";
        }
    } 
            
    else if (command == "DEL") {
        if (del(key)) {
            return "DONE";
        } else {
            return "Key \"" + key + "\" does not exist\n";
        }
    } 
           
    else {
        return "Unknown command";
    }
}