#include "kv_service.h"
#include "kv_store.h"
#include <grpcpp/grpcpp.h>
#include <memory>

KVStore store;
KVServiceImpl service(store);

int main() {
    const std::string server_address = "0.0.0.0:3490";
    grpc::ServerBuilder builder;

    builder.AddListeningPort(server_address,
                             grpc::InsecureServerCredentials()); // Listen for clients
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();

    std::cout << "KV Store listening on " << server_address << '\n';
    server->Wait(); // Wait while the service is completing

    return 0;
}