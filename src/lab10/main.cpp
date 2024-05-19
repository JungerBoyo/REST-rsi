#include <spdlog/spdlog.h>
#include <fmt/format.h>

#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/mime.h>

#include <chrono>
#include <vector>
using namespace Pistache;

#include <nlohmann/json.hpp>
using namespace nlohmann::literals;

namespace ns {
    struct Message {
        std::string author;
        uint id;
        std::string contents;
    };
}
using Message = ns::Message;

std::vector<Message> messages {
    { "Piotr", 0, "Cześć" },    
    { "Jacek", 1, "Cześć" },   
    { "Jarek", 2, "Cześć" }    
};

const Message& dbGetMessage(std::size_t id) {
    if (id >= messages.size()) {
        throw std::runtime_error(fmt::format("No such message with id {}", id));
    }
    return messages[id];
}
const std::vector<Message>& dbGetMessages() noexcept {
    return messages;
}
void dbCreateMessage(const Message& message) {
    messages.push_back(message);
}
void dbUpdateMessage(const Message& message, std::size_t id) {
    if (id >= messages.size()) {
        throw std::runtime_error(fmt::format("No such message with id {}", id));
    }
    messages[id] = message;
}
void dbDeleteMessage(std::size_t id) {
    if (id >= messages.size()) {
        throw std::runtime_error(fmt::format("No such message with id {}", id));
    }
    messages.erase(std::next(messages.begin(), id));
}

namespace ns {
    void to_json(nlohmann::json& j, const Message& m) {
        j = nlohmann::json{
            {"author", m.author},
            {"id", m.id},
            {"contents", m.contents}
        };
    }
    void from_json(const nlohmann::json& j, Message& m) {
        j.at("author").get_to(m.author);
        j.at("id").get_to(m.id);
        j.at("contents").get_to(m.contents);
    }
}
auto toJSON(const std::vector<Message>& messages) {
    nlohmann::json result;
    for (const auto& m : messages) {
        nlohmann::json j = m;
        result.push_back(j);
    }
    return result.dump();
}

struct MessagesService {
    using Self = MessagesService;

    uint16_t _port;
    uint _num_threads;
    Address _address{ "localhost", _port };
    std::shared_ptr<Http::Endpoint> _end_point{ std::make_shared<Http::Endpoint>(_address) };
    Rest::Router _router;

    MessagesService(uint16_t port, uint num_threads = std::thread::hardware_concurrency())
        : _port(port),
          _num_threads(num_threads) {}

    void getMessages(const Rest::Request& request, Http::ResponseWriter response) {
        response.send(Http::Code::Ok, toJSON(messages), MIME(Application, Json));
    }

    void getMessage(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            const auto id = request.param(":id").as<std::size_t>();
            const auto& m = dbGetMessage(id);
            nlohmann::json j = m;
            response.send(Http::Code::Ok, j.dump(), MIME(Application, Json));
        } catch (const std::exception& e) {
            response.send(
                Pistache::Http::Code::Internal_Server_Error,
                fmt::format("Internal error: {}", e.what()),
                MIME(Text, Plain)
            );
        }
    }
    void createMessage(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            if (request.headers().has("/json")) {
                throw std::runtime_error(
                    fmt::format("Wrong MIME type, only JSON accepted, passed {}", MIME(Application, Json).toString())
                );
            }
            dbCreateMessage(nlohmann::json::parse(request.body()).template get<Message>());
            response.send(Http::Code::Ok, "Message has been succesfully created!");
        } catch (const std::exception& e) {
            response.send(
                Pistache::Http::Code::Internal_Server_Error,
                fmt::format("Internal error: {}", e.what()),
                MIME(Text, Plain)
            );
        }
    }
    void updateMessage(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            const auto id = request.param(":id").as<std::size_t>();
            if (request.headers().has("/json")) {
                throw std::runtime_error(
                    fmt::format("Wrong MIME type, only JSON accepted, passed {}", MIME(Application, Json).toString())
                );
            }
            dbUpdateMessage(nlohmann::json::parse(request.body()).template get<Message>(), id);
            response.send(Http::Code::Ok, "Message has been succesfully updated!");
        } catch (const std::exception& e) {
            response.send(
                Pistache::Http::Code::Internal_Server_Error,
                fmt::format("Internal error: {}", e.what()),
                MIME(Text, Plain)
            );
        }
    }
    void deleteMessage(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            const auto id = request.param(":id").as<std::size_t>();
            dbDeleteMessage(id);
            response.send(Http::Code::Ok, "Message has been succesfully deleted!");
        } catch (const std::exception& e) {
            response.send(
                Pistache::Http::Code::Internal_Server_Error,
                fmt::format("Internal error: {}", e.what()),
                MIME(Text, Plain)
            );
        }
    }

    void run() {
        spdlog::info("Server started on port {} with {} threads", _port, _num_threads);

        _end_point->init(Http::Endpoint::options().threads(_num_threads));

        Rest::Routes::Get(_router, "/messages", Rest::Routes::bind(&Self::getMessages, this));
        Rest::Routes::Get(_router, "/message/:id", Rest::Routes::bind(&Self::getMessage, this));
        Rest::Routes::Post(_router, "/message", Rest::Routes::bind(&Self::createMessage, this));
        Rest::Routes::Put(_router, "/message/:id", Rest::Routes::bind(&Self::updateMessage, this));
        Rest::Routes::Delete(_router, "/message/:id", Rest::Routes::bind(&Self::deleteMessage, this));

        _end_point->setHandler(_router.handler());

        _end_point->serve();
    }
};
int main(int argc, char** argv) {
    try {
        MessagesService service(argc <= 1 ? 8080 : std::strtoll(argv[1], NULL, 10), 1);
        service.run();
    }
    catch (const std::exception &e) {
        spdlog::error(e.what());
        return 1;
    }
    catch (...) {
        return 1;
    }

    return 0;
}

