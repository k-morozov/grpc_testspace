#include <iostream>

#include <grpcpp/grpcpp.h>
#include "route.grpc.pb.h"

#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpc/support/log.h>

class ChatServer final {
public:
    ~ChatServer() {
        server_->Shutdown();
        cq_->Shutdown();
    }

    void Run() {
        std::string server_address("127.0.0.1:7777");

        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service_);
        cq_ = builder.AddCompletionQueue();
        server_ = builder.BuildAndStart();
        std::cout << "Server listening on " << server_address << std::endl;

        HandleRPCs();
    }

private:

    class CallData {
    public:
        explicit CallData(App::AppChat::AsyncService* service, grpc::ServerCompletionQueue* cq):
            service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
        {
            Proceed();
        }

        void Proceed() {
            if (status_ == CREATE) {
                status_ = PROCESS;
                service_->RequestChat(&ctx_, &request_, &responder_, cq_, cq_, this);
            } else if (status_==PROCESS) {
                new CallData(service_, cq_);
                response_.set_text("done");
                status_ = FINISH;
                responder_.Finish(response_, grpc::Status::OK, this);
            } else {
                GPR_ASSERT(status_ == FINISH);
                delete this;
            }
        }
    private:
        App::AppChat::AsyncService *service_;
        grpc::ServerCompletionQueue *cq_;
        grpc::ServerContext ctx_;

        App::Msg request_;
        App::Msg response_;

        grpc::ServerAsyncResponseWriter<App::Msg> responder_;
        enum CallStatus {CREATE, PROCESS, FINISH};

        CallStatus status_;

    };

    void HandleRPCs() {
        std::cout << "new connection" << std::endl;
        new CallData(&service_, cq_.get());
        void *tag;
        bool ok;
        while(true) {
            GPR_ASSERT(cq_->Next(&tag, &ok));
            GPR_ASSERT(ok);
            static_cast<CallData*>(tag)->Proceed();
        }
    }

    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    App::AppChat::AsyncService service_;
    std::unique_ptr<grpc::Server> server_;
};


int main() {
    ChatServer server;
    server.Run();

    return 0;
}
