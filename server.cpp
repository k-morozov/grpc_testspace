#include <iostream>

#include <grpcpp/grpcpp.h>
#include "route.grpc.pb.h"

#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

class RouteGuideImpl final : public Route::RouteGuide::Service {
public:
    grpc::Status GetFeature(::grpc::ServerContext* /*context*/, const ::Route::Point* request, ::Route::Feature* response) override {
        response->set_name("server_set_name");
        response->mutable_loacation()->CopyFrom(*request);
        return grpc::Status::OK;
    }
};

void RunServer() {
    std::string server_address("127.0.0.1:7777");
    RouteGuideImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    server->Wait();
}

int main() {
    RunServer();
    return 0;
}
