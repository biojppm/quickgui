#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <api/schema.grpc.fb.h>
#include <api/schema_generated.h>
#include <grpcpp/grpcpp.h>
#pragma GCC diagnostic pop

#include <iostream>
#include <memory>
#include <string>

class GreeterClient {
public:
    GreeterClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(Greeter::NewStub(channel)) {}

    std::string SayHello(const std::string &name)
    {
        flatbuffers::grpc::MessageBuilder mb;
        auto name_offset = mb.CreateString(name);
        auto request_offset = CreateHelloRequest(mb, name_offset);
        mb.Finish(request_offset);
        auto request_msg = mb.ReleaseMessage<HelloRequest>();

        flatbuffers::grpc::Message<HelloReply> response_msg;

        grpc::ClientContext context;

        auto status = stub_->SayHello(&context, request_msg, &response_msg);
        if (status.ok()) {
            const HelloReply *response = response_msg.GetRoot();
            return response->message()->str();
        } else {
            std::cerr << status.error_code() << ": " << status.error_message()
                      << std::endl;
            return "RPC failed";
        }
    }

    void SayManyHellos(const std::string &name, int num_greetings,
                       std::function<void(const std::string &)> callback)
    {
        flatbuffers::grpc::MessageBuilder mb;
        auto name_offset = mb.CreateString(name);
        auto request_offset =
            CreateManyHellosRequest(mb, name_offset, num_greetings);
        mb.Finish(request_offset);
        auto request_msg = mb.ReleaseMessage<ManyHellosRequest>();

        flatbuffers::grpc::Message<HelloReply> response_msg;

        grpc::ClientContext context;

        auto stream = stub_->SayManyHellos(&context, request_msg);
        while (stream->Read(&response_msg)) {
            const HelloReply *response = response_msg.GetRoot();
            callback(response->message()->str());
        }
        auto status = stream->Finish();
        if (!status.ok()) {
            std::cerr << status.error_code() << ": " << status.error_message()
                      << std::endl;
            callback("RPC failed");
        }
    }

private:
    std::unique_ptr<Greeter::Stub> stub_;
};
