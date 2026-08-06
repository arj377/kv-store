#include "kv_service.h"

KVServiceImpl::KVServiceImpl(KVStore &kvStore) : kvStore_(kvStore) {
}

::grpc::Status KVServiceImpl::Set(::grpc::ServerContext *context, const ::SetRequest *request,
                                  ::SetResponse *response) {
    (void)context;
    if (kvStore_.set(request->key(), request->value())) {
        response->set_success(true);
        response->set_error("");
        return grpc::Status::OK;
    }
    response->set_success(false);
    response->set_error("Failed to store key.");
    return grpc::Status::OK;
}

::grpc::Status KVServiceImpl::Get(::grpc::ServerContext *context, const ::GetRequest *request,
                                  ::GetResponse *response) {
    (void)context;
    auto it = kvStore_.get(request->key());
    if (it) {
        response->set_value(it.value());
        response->set_success(true);
        response->set_error("");
        return grpc::Status::OK;
    }
    response->set_success(false);
    response->set_error("Key not found.");
    return grpc::Status::OK;
}

::grpc::Status KVServiceImpl::Delete(::grpc::ServerContext *context, const ::DelRequest *request,
                                     ::DelResponse *response) {
    (void)context;
    if (kvStore_.del(request->key())) {
        response->set_success(true);
        response->set_error("");
        return grpc::Status::OK;
    }
    response->set_success(false);
    response->set_error("Key not found.");
    return grpc::Status::OK;
}
