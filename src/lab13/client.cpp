#include <vector>
#include <string_view>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <fmt/format.h>
#include <CLI/CLI.hpp>

#include <pistache/description.h>
#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/mime.h>
#include <pistache/serializer/rapidjson.h>
#include <pistache/client.h>

using namespace Pistache;

#include <nlohmann/json.hpp>
using namespace nlohmann::literals;

namespace ns {

#include "shared.h"

void to_json(nlohmann::json &j, const Message &m) {
    j = nlohmann::json{
        {"author", m.author},
        {"contents", m.contents}
    };
}
void from_json(const nlohmann::json &j, Message &m) {
    j.at("author").get_to(m.author);
    j.at("contents").get_to(m.contents);
}
void to_json(nlohmann::json &j, const Subscription &s) {
    j = nlohmann::json{
        {"client_callback_url", s.client_callback_url},
    };
}
void from_json(const nlohmann::json &j, Subscription &s) {
    j.at("client_callback_url").get_to(s.client_callback_url);
}

}

auto logger = spdlog::stdout_color_mt("client");

struct ClientSubscriber {
    using Self = ClientSubscriber;

    uint16_t _port;
    uint _num_threads;
    Address _address{"localhost", _port};
    std::shared_ptr<Http::Endpoint> _end_point{std::make_shared<Http::Endpoint>(_address)};
    Rest::Description _desc{"Basic Client Pub/Sub API", "0.1"};
    Rest::Router _router;

    ClientSubscriber(uint16_t port, uint num_threads = std::thread::hardware_concurrency())
        : _port(port),
          _num_threads(num_threads) {}

    void inbox(const Rest::Request &request, Http::ResponseWriter response) {
        try {
            if (request.headers().has("/json")) {
                throw std::runtime_error(
                    fmt::format("Wrong MIME type, only JSON accepted, passed {}", MIME(Application, Json).toString()));
            }
            const auto message = nlohmann::json::parse(request.body()).template get<ns::Message>();

            logger->info("Received : {}", request.body());

            response.send(Http::Code::Ok, "Received!");
        }
        catch (const std::exception &e) {
            response.send(
                Pistache::Http::Code::Internal_Server_Error,
                fmt::format("Internal error: {}", e.what()),
                MIME(Text, Plain)
            );
        }
    }

    void init() {
        _end_point->init(Http::Endpoint::options().threads(_num_threads));

        describe();

        _router.initFromDescription(_desc);

        Rest::Swagger swagger(_desc);
        swagger.uiPath("/doc")
            .uiDirectory("/home/regu/cool_tools/swagger-ui/dist")
            .apiPath("/client-pubsub-api.json")
            .serializer(&Rest::Serializer::rapidJson)
            .install(_router);

        _end_point->setHandler(_router.handler());
    }
    void run() {
        _end_point->serve();
    }
    void describe() {
        _desc.info().license("Apache", "http://www.apache.org/licenses/LICENSE-2.0");

        _desc.schemes(Rest::Scheme::Http).basePath("/v1/client")
            .produces(MIME(Application, Json))
            .consumes(MIME(Application, Json));

        auto version_path = _desc.path("/v1/client");

        version_path.route(_desc.post("/inbox")).bind(&Self::inbox, this)
            .consumes(MIME(Application, Json))
            .response(Http::Code::Ok, "Received message!")
            .response(Http::Code::Internal_Server_Error, "Error during receiving message!");
    }
};

int main(int argc, char **argv)
{
    CLI::App app("Client pub/sub app");

    auto content_type_header = std::make_shared<Http::Header::ContentType>(MIME(Application, Json));

    std::string author = "";
    std::string contents = "";
    int port = 0;
    int client_port = 0;

    auto *app_subcriber = app.add_subcommand("subscriber");
    app_subcriber->add_option("-o,--port", port, "Server port.");
    app_subcriber->add_option("-c,--client-port", client_port, "Client port to send to delivered messages.");
    app_subcriber->callback([&] {
        auto inbox_addr = fmt::format("localhost:{}/v1/client/inbox", client_port);
        auto server_base_addr = fmt::format("localhost:{}/v1", port);

        Http::Experimental::Client client{};
        auto opts = Http::Experimental::Client::options().threads(1).maxConnectionsPerHost(1);
        client.init(opts);

        ns::Subscription sub{ .client_callback_url = inbox_addr };
        nlohmann::json body = sub;
    
        std::atomic<bool> got_response = false;
        auto promise = client
            .post(server_base_addr + "/subscribe")
            .body(body.dump())
            .header(content_type_header)
            .send().then([&](const Http::Response& response) { 
                got_response = true;
                logger->info(response.body());
            },
            Async::Throw
        );

        while (!got_response) {}
        
        client.shutdown();

        logger->info("Polling...");
        ClientSubscriber client_subscriber(client_port, 1);
        client_subscriber.init();
        client_subscriber.run();
    });

    auto *app_publisher = app.add_subcommand("publisher");
    app_publisher->add_option("-o,--port", port, "Server port.");
    app_publisher->add_option("-a,--author", author, "Author of to be published message.");
    app_publisher->add_option("-m,--contents", contents, "Contents of to be published message.");
    app_publisher->callback([&] {
        auto inbox_addr = fmt::format("localhost:{}/v1/client/inbox", client_port);
        auto server_base_addr = fmt::format("localhost:{}/v1", port);

        Http::Experimental::Client client{};
        auto opts = Http::Experimental::Client::options().threads(1).maxConnectionsPerHost(1);
        client.init(opts);

        ns::Message sub{ .author = std::move(author), .contents = std::move(contents) };
        nlohmann::json body = sub;

        std::atomic<bool> got_response = false;
        auto promise = client
            .post(server_base_addr + "/publish")
            .body(body.dump())
            .header(content_type_header)
            .send().then([&](const Http::Response& response) { 
                got_response = true;
                logger->info(response.body());
            },
            Async::Throw
        );

        while (!got_response) {}
        
        client.shutdown();
    });

    CLI11_PARSE(app, argc, argv);
}