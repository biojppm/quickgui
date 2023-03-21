
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <api/schema.grpc.fb.h>
#include <api/schema_generated.h>
#include <grpcpp/grpcpp.h>
#pragma GCC diagnostic pop

#include <iostream>
#include <memory>
#include <string>

class GreeterServiceImpl final : public Greeter::Service
{
    virtual grpc::Status
    SayHello(grpc::ServerContext *context,
             const flatbuffers::grpc::Message<HelloRequest> *request_msg,
             flatbuffers::grpc::Message<HelloReply> *response_msg)
        override
    {
        (void)context;
        // We call GetRoot to "parse" the message. Verification is already
        // performed by default. See the notes below for more details.
        const HelloRequest *request = request_msg->GetRoot();

        // Fields are retrieved as usual with FlatBuffers
        const std::string &name = request->name()->str();

        // `flatbuffers::grpc::MessageBuilder` is a `FlatBufferBuilder` with a
        // special allocator for efficient gRPC buffer transfer, but otherwise
        // usage is the same as usual.
        auto msg_offset = mb_.CreateString("Hello, " + name);
        auto hello_offset = CreateHelloReply(mb_, msg_offset);
        mb_.Finish(hello_offset);

        // The `ReleaseMessage<T>()` function detaches the message from the
        // builder, so we can transfer the resopnse to gRPC while simultaneously
        // detaching that memory buffer from the builer.
        *response_msg = mb_.ReleaseMessage<HelloReply>();
        assert(response_msg->Verify());

        // Return an OK status.
        return grpc::Status::OK;
    }

    virtual grpc::Status
    SayManyHellos(grpc::ServerContext *context,
                  const flatbuffers::grpc::Message<ManyHellosRequest> *request_msg,
                  grpc::ServerWriter<flatbuffers::grpc::Message<HelloReply>> *writer)
        override
    {
        (void)context;
        // The streaming usage below is simply a combination of standard gRPC
        // streaming with the FlatBuffers usage shown above.
        const ManyHellosRequest *request = request_msg->GetRoot();
        const std::string &name = request->name()->str();
        int num_greetings = request->num_greetings();

        for (int i = 0; i < num_greetings; i++) {
            auto msg_offset = mb_.CreateString("Many hellos, " + name);
            auto hello_offset = CreateHelloReply(mb_, msg_offset);
            mb_.Finish(hello_offset);
            writer->Write(mb_.ReleaseMessage<HelloReply>());
        }

        return grpc::Status::OK;
    }

    flatbuffers::grpc::MessageBuilder mb_;
};


void RunServer(const char *server_address)
{
    GreeterServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    server->Wait();
}

int main(int argc, const char *argv[])
{
    const char *server_address = "0.0.0.0:50051";
    if(argc > 1)
        server_address = argv[1];
    RunServer(server_address);
    return 0;
}
