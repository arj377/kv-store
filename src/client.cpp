#include "kv_service.h"
#include "kv_store.h"
#include <iostream>
#include <sstream>

int main() {
    const std::string server_address = "localhost:3490";
    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    auto stub = KVService::NewStub(channel);

    while (true) {
        grpc::ClientContext context;
        std::string input;
        std::getline(std::cin, input);

        std::istringstream parser(input);
        std::string command, key, value;
        parser >> command; // Creates parser to get command
        if (command.empty()) {
            std::cout << "No command.\n\n";
            continue;
        }

        if (command == "EXIT") {
            std::cout << "Goodbye!\n";
            break;
        } else if (command == "SET") {
            SetRequest request;
            SetResponse response;
            // Make sure there is a key
            if (!(parser >> key)) {
                std::cout << "Missing key.\n\n";
                continue;
            }
            parser >> std::ws;
            getline(parser, value); // Set the rest of parser to value

            // Make sure there is a value
            if (value.empty()) {
                std::cout << "Missing value.\n\n";
                continue;
            }
            request.set_key(key);
            request.set_value(value);
            grpc::Status status = stub->Set(&context, request, &response);

            if (!status.ok()) {
                std::cout << "RPC failed: " << status.error_message() << "\n\n";
                continue;
            } else if (!response.success()) {
                std::cout << response.error() << "\n\n";
                continue;
            } else {
                std::cout << "SET successful.\n\n";
            }

        } else if (command == "GET") {
            GetRequest request;
            GetResponse response;
            // make sure there is one key
            if (!(parser >> key)) {
                std::cout << "Missing key.\n\n";
                continue;
            }
            // make sure there is no more than one key
            if (parser >> value) {
                std::cout << "Too many arguments for GET.\n\n";
                continue;
            }
            request.set_key(key);
            grpc::Status status = stub->Get(&context, request, &response);

            if (!status.ok()) {
                std::cout << "RPC failed: " << status.error_message() << "\n\n";
                continue;
            } else if (!response.success()) {
                std::cout << response.error() << "\n\n";
                continue;
            } else {
                std::cout << response.value() << "\n\n";
            }
        }

        else if (command == "DEL") {
            DelRequest request;
            DelResponse response;

            // Make sure there is at least one key
            if (!(parser >> key)) {
                std::cout << "Missing key.\n\n";
                continue;
            }
            // Make sure there is no more than one key
            if (parser >> value) {
                std::cout << "Too many arguments for DEL.\n\n";
                continue;
            }
            request.set_key(key);
            grpc::Status status = stub->Delete(&context, request, &response);

            if (!status.ok()) {
                std::cout << "RPC failed: " << status.error_message() << "\n\n";
                continue;
            } else if (!response.success()) {
                std::cout << response.error() << "\n\n";
                continue;
            } else {
                std::cout << "Key deleted.\n\n";
            }
        } else {
            std::cout << "Invalid command.\n\n";
            continue;
        }
    }

    return 0;
}