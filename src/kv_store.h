#include <string>

void loadDatabase(); // Loads in the database from the disk

void saveDatabase(); // Saves any new changes

bool exists(const std::string&); // Checks if the key exists in the database

void set(const std::string&, const std::string&); // Sets a key and value in the database

std::string get(const std::string&); // Returns a value associated with a key from the database

bool del(const std::string&); // Deletes a key and its value

std::string execute(const std::string&); // Runs GET, SET, or DEL depending on user input
