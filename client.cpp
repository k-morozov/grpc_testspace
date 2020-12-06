#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include "route.grpc.pb.h"

class ChatClient {
public:
    ChatClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(App::AppChat::NewStub(channel)) {}


    void SendMsg(const std::string& text) {
        ::App::Msg request;
        request.set_text(text);

        AsyncClientCall* call = new AsyncClientCall;

        call->response_reader = stub_->PrepareAsyncChat(&call->context, request, &cq_);
            

        call->response_reader->StartCall();
        call->response_reader->Finish(&call->reply, &call->status, (void*)call);
      }

    void AsyncCompleteRpc() {
        void* got_tag;
        bool ok = false;

        while (cq_.Next(&got_tag, &ok)) {
            AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);

            GPR_ASSERT(ok);

            if (call->status.ok())
                std::cout << "Server reply: " << call->reply.text() << std::endl;
            else
                std::cout << "RPC failed" << std::endl;

            delete call;
        }
    }

private:
    std::unique_ptr<App::AppChat::Stub> stub_;
    grpc::CompletionQueue cq_;

    struct AsyncClientCall {
        ::App::Msg reply;
        ::grpc::ClientContext context;
        ::grpc::Status status;

        std::unique_ptr<::grpc::ClientAsyncResponseReader<::App::Msg>> response_reader;
    };

};

int main(int, char**) {
    ChatClient client(grpc::CreateChannel(
        "127.0.0.1:7777", grpc::InsecureChannelCredentials()));
    std::thread thread_ = std::thread(&ChatClient::AsyncCompleteRpc, &client);
    
    std::string text;
    for(int i=0; i<10; ++i) {
        std::cin >> text;
        client.SendMsg(text);
    }
    

    thread_.join();
    return 0;
}
