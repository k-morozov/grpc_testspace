#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include "route.grpc.pb.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::ClientAsyncReader;
using grpc::ClientAsyncWriter;
using grpc::ClientAsyncReaderWriter;
using grpc::CompletionQueue;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;


class AbstractAsyncClientCall
{
public:
	enum CallStatus { PROCESS, FINISH, DESTROY };

	explicit AbstractAsyncClientCall():callStatus(PROCESS){}
	virtual ~AbstractAsyncClientCall(){}
	HelloReply reply;
	ClientContext context;
	Status status;
	CallStatus callStatus ;
	void printReply(const char* from)
	{
		if(!reply.message().empty())
			std::cout << "[" << from << "]: reply message = " << reply.message() << std::endl;
		else
			std::cout << "[" << from << "]: reply message empty" << std::endl;

	}
	virtual void Proceed(bool = true) = 0;
};

class AsyncClientCallMM : public AbstractAsyncClientCall
{
	std::unique_ptr< ClientAsyncReaderWriter<HelloRequest,HelloReply> > responder;
	unsigned mcounter;
	bool writing_mode_;
public:
	AsyncClientCallMM(CompletionQueue& cq_, std::unique_ptr<Greeter::Stub>& stub_):
	AbstractAsyncClientCall(), mcounter(0), writing_mode_(true)
	{
		std::cout << "[ProceedMM]: new client M-M" << std::endl;
    	responder = stub_->AsyncBothGladToSee(&context, &cq_, (void*)this);
		callStatus = PROCESS ;
	}
	virtual void Proceed(bool ok = true) override
	{
		if(callStatus == PROCESS)
		{

			if(writing_mode_)
			{
				static std::vector<std::string> greeting = {"Hello, server!",
    	                                    	"Glad to see you!",
        	                                	"Haven't seen you for thousand years!",
            	                            	"I'm client now. Call me later."};
				//std::cout << "[ProceedMM]: mcounter = " << mcounter << std::endl;
    			if(mcounter < greeting.size())
    			{
        			HelloRequest request;
        			request.set_name(greeting.at(mcounter));
           			responder->Write(request, (void*)this);
        			++mcounter;
    			}
    			else
    			{
            		responder->WritesDone((void*)this);
					std::cout << "[ProceedMM]: changing state to reading" << std::endl;
        			writing_mode_ = false;
    			}
				return ;
			}
			else //reading mode
			{
				if(!ok)
				{
					std::cout << "[ProceedMM]: trying finish" << std::endl;
					callStatus = FINISH;
					responder->Finish(&status, (void*)this);
					return;
				}
				responder->Read(&reply, (void*)this);
				printReply("ProceedMM");
			}
		    return;
		}
		else if(callStatus == FINISH)
		{
			std::cout << "[ProceedMM]: Good Bye" << std::endl;
			delete this;
		}

	}
};

class GreeterClient
{
public:
    explicit GreeterClient(std::shared_ptr<Channel> channel)
            :stub_(Greeter::NewStub(channel))
	{}

	void BothGladToSee()
	{
    	new AsyncClientCallMM(cq_, stub_);
	}

	void AsyncCompleteRpc()
	{
		void* got_tag;
    	bool ok = false;
		while(cq_.Next(&got_tag, &ok))
		{
        	AbstractAsyncClientCall* call = static_cast<AbstractAsyncClientCall*>(got_tag);
			call->Proceed(ok);
    	}
		std::cout << "Completion queue is shutting down." << std::endl;
	}

private:
    std::unique_ptr<Greeter::Stub> stub_;

    CompletionQueue cq_;
};


int main(int argc, char* argv[])
{
	GreeterClient greeter(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
	std::thread thread_ = std::thread(&GreeterClient::AsyncCompleteRpc, &greeter);
    
	greeter.BothGladToSee();
	thread_.join();

}