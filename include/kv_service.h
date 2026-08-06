#pragma once

#include <grpcpp/grpcpp.h>

#include "kv.grpc.pb.h"
#include "kv_store.h"

class KVServiceImpl : public KVService::Service {
  public:
    KVServiceImpl(KVStore &kvStore);
    virtual ::grpc::Status Set(::grpc::ServerContext *context, const ::SetRequest *request,
                               ::SetResponse *response) override;
    virtual ::grpc::Status Get(::grpc::ServerContext *context, const ::GetRequest *request,
                               ::GetResponse *response) override;
    virtual ::grpc::Status Delete(::grpc::ServerContext *context, const ::DelRequest *request,
                                  ::DelResponse *response) override;

  private:
    KVStore &kvStore_;
};