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

void to_json(nlohmann::json& j, const Message& m) {
    j = nlohmann::json{
        {"author", m.author},
        {"contents", m.contents}
    };
}
void from_json(const nlohmann::json& j, Message& m) {
    j.at("author").get_to(m.author);
    j.at("contents").get_to(m.contents);
}
void to_json(nlohmann::json& j, const Subscription& s) {
    j = nlohmann::json{
        {"client_callback_url", s.client_callback_url},
    };
}
void from_json(const nlohmann::json& j, Subscription& s) {
    j.at("client_callback_url").get_to(s.client_callback_url);
}

}

auto logger = spdlog::stdout_color_mt("server");

struct Server {
    using Self = Server;

    uint16_t _port;
    uint _num_threads;
    Address _address{ "localhost", _port };
    std::shared_ptr<Http::Endpoint> _end_point{ std::make_shared<Http::Endpoint>(_address) };
    Rest::Description _desc{ "Basic Server Pub/Sub API", "0.1" };
    Rest::Router _router;

    std::vector<ns::Subscription> _subscribers;
    std::queue<ns::Message> _published_messages;
    
    std::thread _deliverer_thread;

    mutable std::mutex _m;
    std::condition_variable _cv;

    Server(uint16_t port, uint num_threads = std::thread::hardware_concurrency())
        : _port(port),
          _num_threads(num_threads),
          _deliverer_thread(&Self::deliverer, this)   
           {}

    ~Server() {
        if (_deliverer_thread.joinable()) {
            _deliverer_thread.join();
        }
    }

    void deliverer() {
        Http::Experimental::Client client{};

        auto opts = Http::Experimental::Client::options().threads(1).maxConnectionsPerHost(1);
        client.init(opts);

        while (true) {
            std::unique_lock<std::mutex> lock(_m);
            _cv.wait(lock, [this] { return !this->_published_messages.empty(); });

            const auto& message = _published_messages.back();
            const nlohmann::json body = message;

            for (const auto& subscriber : _subscribers) {
                client.post(subscriber.client_callback_url).body(body.dump()).send();
            }

            _published_messages.pop();
        }
    }

    void subscribe(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            if (request.headers().has("/json")) {
                throw std::runtime_error(
                    fmt::format("Wrong MIME type, only JSON accepted, passed {}", MIME(Application, Json).toString())
                );
            }

            const auto subscription = nlohmann::json::parse(request.body()).template get<ns::Subscription>();

            logger->info("Received subscription request from {}.", subscription.client_callback_url); 
            
            std::lock_guard<std::mutex> lock(_m);
            _subscribers.push_back(std::move(subscription));

            response.send(Http::Code::Ok, "Subscribed!!");
        } catch (const std::exception& e) {
            response.send(
                Pistache::Http::Code::Internal_Server_Error,
                fmt::format("Internal error: {}", e.what()),
                MIME(Text, Plain)
            );
        }
    }
    void publish(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            if (request.headers().has("/json")) {
                throw std::runtime_error(
                    fmt::format("Wrong MIME type, only JSON accepted, passed {}", MIME(Application, Json).toString())
                );
            }
            const auto message = nlohmann::json::parse(request.body()).template get<ns::Message>();
            
            logger->info("Received message to publish from {}.", message.author); 

            std::lock_guard<std::mutex> lock(_m);
            _published_messages.push(std::move(message));

            _cv.notify_one();
            
            response.send(Http::Code::Ok, "Published!!");
        } catch (const std::exception& e) {
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
            .apiPath("/server-pubsub-api.json")
            .serializer(&Rest::Serializer::rapidJson)
            .install(_router);

        _end_point->setHandler(_router.handler());
    }
    void run() {
        logger->info("Server started on port {} with {} threads", _port, _num_threads);
        _end_point->serve();
    }
    void describe() {
        _desc.info().license("Apache", "http://www.apache.org/licenses/LICENSE-2.0");
        
        _desc.schemes(Rest::Scheme::Http).basePath("/v1")
            .produces(MIME(Application, Json))
            .consumes(MIME(Application, Json));

        auto version_path = _desc.path("/v1");

        version_path.route(_desc.post("/subscribe")).bind(&Self::subscribe, this)
            .consumes(MIME(Application, Json))
            .response(Http::Code::Ok, "Subscribed!")
            .response(Http::Code::Internal_Server_Error, "Couldn't subsribe!");

        version_path.route(_desc.post("/publish")).bind(&Self::publish, this)
            .consumes(MIME(Application, Json))
            .response(Http::Code::Ok, "Message published!")
            .response(Http::Code::Internal_Server_Error, "Couldn't publish!");
    }
};

int main(int argc, char** argv) {
    CLI::App app("Server pub/sub app");
    int port = 0;
    app.add_option("-o,--port", port, "Server port.");

    CLI11_PARSE(app, argc, argv);

    try {
        Server server(port, 2);
        server.init();
        server.run();
    }
    catch (const std::exception &e) {
        logger->error(e.what());
        return 1;
    }
    catch (...) {
        return 1;
    }
}