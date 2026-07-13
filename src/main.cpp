#include <iostream>      
#include <sstream>      
#include "kv_store.h"


int main() {
    std::cout << "KV Store" << std::endl;
    std::string input;

    while (true) {
        //get user input
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input == "EXIT") {
            break;
        }
        //creates parser to go through arguments
        std::istringstream parser(input);
        std::string command;
        std::string key;
        std::string value;
        
        //initialize values based on user input
        parser >> command >> key >> value;

        //based on user request, execute proper command
        if (command == "SET") {
            set(key,value);
            std::cout << "DONE" << std::endl;
        } 
        
        else if (command == "GET") {
            if (exists(key)) {
                std::cout << get(key) << std::endl;
            } else {
                std::cout << "Key \"" << key << "\" does not exist\n";
            }
        } 
        
        else if (command == "DEL") {
            if (del(key)) {
                std::cout << "DONE" << std::endl;
            } else {
                std::cout << "Key \"" << key << "\"" << " does not exist" << std::endl;
            }
        } else {
            std::cout << "Unknown command" << std::endl;
        }
    }
    return 0;
}