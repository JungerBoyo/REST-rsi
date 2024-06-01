#include <chrono>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>

#include <spdlog/spdlog.h>
#include <fmt/format.h>

#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/mime.h>

using namespace Pistache;

#include <nlohmann/json.hpp>
using namespace nlohmann::literals;

namespace ns {
    struct Comment {
        std::string author;
        std::string contents;
    };
    struct Message {
        std::string author;
        std::string contents;
        std::vector<Comment> comments;
    };
}
using Message = ns::Message;
using Comment = ns::Comment;

std::vector<Message> messages {
    { "Piotr", "Witaj", {{"Piotr", "Cześć"}, {"Piotr", "Cześć"}, {"Piotr", "Cześć"}}},    
    { "Jacek", "Witaj", {{"Jacek", "Cześć"}, {"Jacek", "Cześć"}, {"Jacek", "Cześć"}}},   
    { "Jarek", "Witaj", {{"Jarek", "Cześć"}, {"Jarek", "Cześć"}, {"Jarek", "Cześć"}}}    
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
    void to_json(nlohmann::json& j, const Comment& c) {
        j = nlohmann::json{
            {"author", c.author},
            {"contents", c.contents}
        };
    }
    void from_json(const nlohmann::json& j, Comment& c) {
        j.at("author").get_to(c.author);
        j.at("contents").get_to(c.contents);
    }
    void to_json(nlohmann::json& j, const Message& m) {
        j = nlohmann::json{
            {"author", m.author},
            {"contents", m.contents},
            {"comments", m.comments}
        };
    }
    void from_json(const nlohmann::json& j, Message& m) {
        j.at("author").get_to(m.author);
        j.at("contents").get_to(m.contents);
        j.at("comments").get_to(m.comments);
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

struct SpdlogStringLogger : Log::StringLogger {
    void log(Log::Level level, const std::string& message) {
        switch (level) {
        case Log::Level::TRACE: spdlog::trace("{}", message); break;
        case Log::Level::DEBUG: spdlog::debug("{}", message); break;
        case Log::Level::INFO:  spdlog::info("{}", message); break;
        case Log::Level::WARN:  spdlog::warn("{}", message); break;
        case Log::Level::ERROR: spdlog::error("{}", message); break;
        case Log::Level::FATAL: spdlog::critical("{}", message); break;
        }
    }
    bool isEnabledFor(Log::Level level) const {
        return static_cast<int>(level) == static_cast<int>(spdlog::get_level());
    }
};

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
    void getMessageComments(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            const auto id = request.param(":id").as<std::size_t>();
            const auto& m = dbGetMessage(id);
            nlohmann::json j = m.comments;
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
            response.headers().add<Http::Header::Location>(
                fmt::format("localhost:{}/message/{}", _address.port().toString(), messages.size()-1UL)
            );
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

        _end_point->init(Http::Endpoint::options().threads(_num_threads).logger(std::make_shared<SpdlogStringLogger>()));

        _router.addMiddleware(+[](Http::Request& request, Http::ResponseWriter& writer) -> bool {
            const auto auth_header = request.headers().get<Http::Header::Authorization>();
            const auto user = auth_header->getBasicUser();
            const auto passwd = auth_header->getBasicPassword();
            if (user != "test" || passwd != "test") {
                writer.send(Http::Code::Forbidden, fmt::format("Forbidden for user {}!!!!", user));
                return false;
            }
            spdlog::info("Hello World!");
            writer.headers().add<Http::Header::Location>("Poland");
            return true;
        });
        Rest::Routes::Get(_router, "/messages", Rest::Routes::bind(&Self::getMessages, this));
        Rest::Routes::Get(_router, "/messages/:startswith", Rest::Routes::bind(&Self::findMessages, this));
        Rest::Routes::Get(_router, "/message/:id", Rest::Routes::bind(&Self::getMessage, this));
        Rest::Routes::Get(_router, "/message/:id/comments", Rest::Routes::bind(&Self::getMessageComments, this));
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

