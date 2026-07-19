#include "kv_store.h"
#include <iostream>      
#include <sstream>   
#include <unordered_map>


std::unordered_map<std::string, std::string> database;

void set(const std::string& key, const std::string& value) {
    database[key] = value;
}

std::string get(const std::string& key) {
    return database.at(key) + "\n"; //throws if the key doesn't exist
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
    parser >> command; //creates parser to get command
    if (command.empty()) {
        return "ERROR: Enter a command and its key and/or value.\n";
    }

    if (command == "SET") {
        //make sure there is a key
        if (!(parser >> key)) {
            return "ERROR: SET expects exactly one key.\n";
        }
        parser >> std::ws; //skip all whitespace
        getline(parser, value); //set the rest of parser to value

        //make sure there is a value
        if (value.empty()) {
            return "ERROR: SET expects exactly one value.\n";
        }
        set(key,value);
        return "DONE.\n";
    } 

    if (command == "GET") {
        //make sure there is at least one key
        if (!(parser >> key)) {
            return "ERROR: GET expects exactly one key.\n";
        }
        //make sure there is no more than one key
        if (parser >> value) {
            return "ERROR: GET expects exactly one key.\n";
        }

        if (exists(key)) { //if the key exists, execute the function
            return get(key);
        } else {
            return "ERROR: Key \"" + key + "\" does not exist.\n";
        }
    } 

    if (command == "DEL") {
        //make sure there is at least one key
        if (!(parser >> key)) {
            return "ERROR: DEL expects exactly one key.\n";
        }
        //make sure there is no more than one key
        if (parser >> value) {
            return "ERROR: DEL expects exactly one key.\n";
        }
        if (del(key)) { //success
            return "DONE.\n";
        } else {
            return "ERROR: Key \"" + key + "\" does not exist.\n";
        }
    } 
           
    return "ERROR: Unknown command.\n";
}