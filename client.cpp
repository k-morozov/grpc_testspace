#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "route.grpc.pb.h"

class RouteClient {
public:
    RouteClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(Route::RouteGuide::NewStub(channel)) {}


    Route::Feature GetFeature() {
        Route::Point request;
        request.set_x(1);
        grpc::ClientContext context;
        Route::Feature reply;
        grpc::Status status = stub_->GetFeature(&context, request, &reply);

        return reply;
    }

    bool GetOneFeature(const Route::Point& point, Route::Feature* feature) {
      grpc::ClientContext context;
      grpc::Status status = stub_->GetFeature(&context, point, feature);
      std::cout << "Found feature called " << feature->name()  << " at " << feature->loacation().x()
                << "." << feature->loacation().y() << std::endl;
      return true;
    }
private:
    std::unique_ptr<Route::RouteGuide::Stub> stub_;
};

int main(int, char**) {
    RouteClient client(grpc::CreateChannel(
        "localhost:7777", grpc::InsecureChannelCredentials()));

    Route::Point point;
    Route::Feature feature;
    point.set_x(1);
    point.set_y(1);

    client.GetOneFeature(point, &feature);
    return 0;
}
