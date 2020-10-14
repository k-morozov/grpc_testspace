#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "route.grpc.pb.h"

class ChatClient {
public:
    ChatClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(App::AppChat::NewStub(channel)) {}


    std::string SayHello(const std::string& text) {
        ::App::Msg request;
        request.set_text(text);

        ::App::Msg reply;
        grpc::ClientContext context;
        grpc::CompletionQueue cq;
        grpc::Status status;

        std::unique_ptr<grpc::ClientAsyncResponseReader<::App::Msg> > rpc(
            stub_->PrepareAsyncChat(&context, request, &cq));

        rpc->StartCall();

        rpc->Finish(&reply, &status, (void*)1);
        void* got_tag;
        bool ok = false;

        GPR_ASSERT(cq.Next(&got_tag, &ok));
        GPR_ASSERT(got_tag == (void*)1);
        GPR_ASSERT(ok);

        if (status.ok()) {
          return reply.text();
        } else {
          return "RPC failed";
        }
      }
private:
    std::unique_ptr<App::AppChat::Stub> stub_;
};

int main(int, char**) {
    ChatClient client(grpc::CreateChannel(
        "127.0.0.1:7777", grpc::InsecureChannelCredentials()));
    std::string reply = client.SayHello("test msg #1");
    std::cout << "Greeter received: " << reply << std::endl;
    return 0;
}
