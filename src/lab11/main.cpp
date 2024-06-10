#include <chrono>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>

#include <spdlog/spdlog.h>
#include <fmt/format.h>

#include <pistache/description.h>
#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/mime.h>
#include <pistache/serializer/rapidjson.h>

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
std::vector<Message> dbGetMessagesStartingWith(std::string_view query) {
    std::vector<Message> result;
    for (const auto& message : messages) {
        if (std::string_view(message.contents).substr(0, query.size()).compare(query.data()) == 0) {
            result.push_back(message);
        }
    }
    return result;
}
std::vector<Message> dbGetMessagesMatching(const Message& m) {
    std::vector<Message> result;
    for (const auto& message : messages) {
        if (message.contents == m.contents && message.author == m.author) {
            result.push_back(message);
        }
    }
    return result;
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
    Rest::Description _desc{ "Message API", "0.1" };
    Rest::Router _router;

    MessagesService(uint16_t port, uint num_threads = std::thread::hardware_concurrency())
        : _port(port),
          _num_threads(num_threads) {}

    void getMessages(const Rest::Request& request, Http::ResponseWriter response) {
        response.send(Http::Code::Ok, toJSON(messages), MIME(Application, Json));
    }
    void findMessages(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            const auto query = request.param(":startswith").as<std::string>();
            if (const auto result = dbGetMessagesStartingWith(query); !result.empty()) {
                response.send(Http::Code::Ok, toJSON(result), MIME(Application, Json));
            } else {
                response.send(Http::Code::Ok, "No such messages...");
            }
        } catch (const std::exception& e) {
            response.send(
                Pistache::Http::Code::Internal_Server_Error,
                fmt::format("Internal error: {}", e.what()),
                MIME(Text, Plain)
            );
        }
    }
    void findMessagesObject(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            const Message message = nlohmann::json::parse(request.body());
            if (const auto result = dbGetMessagesMatching(message); !result.empty()) {
                response.send(Http::Code::Ok, toJSON(result), MIME(Application, Json));
            } else {
                response.send(Http::Code::Ok, "No such messages...");
            }
        } catch (const std::exception& e) {
            response.send(
                Pistache::Http::Code::Internal_Server_Error,
                fmt::format("Internal error: {}", e.what()),
                MIME(Text, Plain)
            );
        }
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

        describe();

        _router.initFromDescription(_desc);

        Rest::Swagger swagger(_desc);
        swagger.uiPath("/doc")
            .uiDirectory("/home/regu/cool_tools/swagger-ui/dist")
            .apiPath("/message-api.json")
            .serializer(&Rest::Serializer::rapidJson)
            .install(_router);

        _end_point->setHandler(_router.handler());

        _end_point->serve();
    }

    void describe() {
        _desc.info().license("Apache", "http://www.apache.org/licenses/LICENSE-2.0");
        
        _desc.schemes(Rest::Scheme::Http).basePath("/v1")
            .produces(MIME(Application, Json))
            .consumes(MIME(Application, Json));

        auto version_path = _desc.path("/v1");

        version_path.route(_desc.get("/messages")).bind(&Self::getMessages, this)
            .produces(MIME(Application, Json))
            .response(Http::Code::Ok, "You are OK")
            .response(Http::Code::Internal_Server_Error, "You are NOT OK!!!");

        version_path.route(_desc.post("/messages")).bind(&Self::findMessagesObject, this)
            .consumes(MIME(Application, Json))
            .produces(MIME(Application, Json))
            .response(Http::Code::Ok, "You are OK")
            .response(Http::Code::Internal_Server_Error, "You are NOT OK!!!");

        version_path.route(_desc.get("/messages/:startswith")).bind(&Self::findMessages, this)
            .produces(MIME(Application, Json))
            .parameter<Rest::Type::String>("startswith", "Query string to filter the messages with.")
            .response(Http::Code::Ok, "You are OK")
            .response(Http::Code::Internal_Server_Error, "You are NOT OK!!!");

        version_path.route(_desc.get("/message/:id")).bind(&Self::getMessage, this)
            .produces(MIME(Application, Json))
            .parameter<Rest::Type::Integer>("id", "Id of the message.")
            .response(Http::Code::Ok, "You are OK")
            .response(Http::Code::Internal_Server_Error, "You are NOT OK!!!");

        version_path.route(_desc.post("/message")).bind(&Self::createMessage, this)
            .produces(MIME(Text, Plain))
            .consumes(MIME(Application, Json))
            .response(Http::Code::Ok, "You are OK")
            .response(Http::Code::Internal_Server_Error, "You are NOT OK!!!");

        version_path.route(_desc.put("/message/:id")).bind(&Self::updateMessage, this)
            .produces(MIME(Text, Plain))
            .consumes(MIME(Application, Json))
            .parameter<Rest::Type::Integer>("id", "Id of the message.")
            .response(Http::Code::Ok, "You are OK")
            .response(Http::Code::Internal_Server_Error, "You are NOT OK!!!");

        version_path.route(_desc.del("/message/:id")).bind(&Self::deleteMessage, this)
            .produces(MIME(Text, Plain))
            .parameter<Rest::Type::Integer>("id", "Id of the message.")
            .response(Http::Code::Ok, "You are OK")
            .response(Http::Code::Internal_Server_Error, "You are NOT OK!!!");
    }
};
int main(int argc, char** argv) {
    try {
        MessagesService service(argc <= 1 ? 8080 : std::strtoll(argv[1], NULL, 10), 1);
        service.run();
    } catch (const std::exception &e) {
        spdlog::error(e.what());
        return 1;
    } catch (...) {
        return 1;
    }

    return 0;
}

