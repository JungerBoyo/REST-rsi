#include <spdlog/spdlog.h>
#include <fmt/format.h>

#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/mime.h>

#include <chrono>
#include <vector>

using namespace Pistache;

#define ZAD_1
#define ZAD_2
#define ZAD_3
#define ZAD_4
//#define ZAD_5

#if defined(ZAD_4) && defined(ZAD_5)
#error "Only 4 or 5"
#endif

#if defined(ZAD_5)
#include <nlohmann/json.hpp>
using namespace nlohmann::literals;
#endif

#if defined(ZAD_4)
struct Message {
    std::string author;
    uint id;
    std::string contents;
};
#elif defined (ZAD_5)
namespace ns {
    struct Message {
        std::string author;
        uint id;
        std::string contents;
    };
}
using Message = ns::Message;
#endif

#if defined(ZAD_4) || defined(ZAD_5)
std::vector<Message> messages {
    { "Piotr", 0, "Cześć" },    
    { "Jacek", 1, "Cześć" },   
    { "Jarek", 2, "Cześć" }    
};
#endif

#if defined(ZAD_4)
auto toXML(const Message& m) {
    return fmt::format(R"(
    <message>
        <author>{}</author>
        <id>{}</id>
        <message>{}</message>
    </message>
    )",
        m.author,
        m.id,
        m.contents
    );
}
auto toXML(const std::vector<Message>& messages) {
    std::string result = "<messages>\n";
    for (const auto& m : messages) {
        result += toXML(m) + "\n";
    }
    result += "</messages>";
    return result;
}
#endif

#if defined(ZAD_5)
namespace ns {
    void to_json(nlohmann::json& j, const Message& m) {
        j = nlohmann::json{
            {"author", m.author},
            {"id", m.id},
            {"address", m.contents}
        };
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
#endif    

#if defined(ZAD_1) || defined(ZAD_2) || defined(ZAD_3) || defined(ZAD_4) || defined(ZAD_5)
struct HelloEchoSerivce {
    uint16_t _port;
    uint _num_threads;
    Address _address{ "localhost", _port };
    std::shared_ptr<Http::Endpoint> _end_point{ std::make_shared<Http::Endpoint>(_address) };
    Rest::Router _router;

    HelloEchoSerivce(uint16_t port, uint num_threads = std::thread::hardware_concurrency())
        : _port(port),
          _num_threads(num_threads) {}

    #if defined(ZAD_1)
    void getHello(const Rest::Request& request, Http::ResponseWriter response) {
        response.send(Http::Code::Ok, "Witaj C++ pistache");
    }
    #endif

    #if defined(ZAD_2)
    void getEcho(const Rest::Request& request, Http::ResponseWriter response) {
        response.send(Http::Code::Ok, "Witaj echo");
    }
    #endif

    #if defined(ZAD_3)
    void getEchoParam(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            const auto id = request.param(":id").as<std::size_t>();
            response.send(Pistache::Http::Code::Ok, "Witaj echo: " + std::to_string(id));
        }
        catch (...) {
            response.send(Pistache::Http::Code::Internal_Server_Error, "Internal error", MIME(Text, Plain));
        }
    }
    #endif

    #if defined(ZAD_4)
    void getMessages(const Rest::Request& request, Http::ResponseWriter response) {
        response.send(Http::Code::Ok, toXML(messages), MIME(Application, Xml));
    }
    #elif defined(ZAD_5)
    void getMessages(const Rest::Request& request, Http::ResponseWriter response) {
        response.send(Http::Code::Ok, toJSON(messages), MIME(Application, Json));
    }
    #endif

    void run() {
        spdlog::info("Server started on port {} with {} threads", _port, _num_threads);

        _end_point->init(Http::Endpoint::options().threads(_num_threads));

    #if defined(ZAD_1)
        Rest::Routes::Get(_router, "/hello", Rest::Routes::bind(&HelloEchoSerivce::getHello, this));
    #endif
        
    #if defined(ZAD_2)
        Rest::Routes::Get(_router, "/hello/echo", Rest::Routes::bind(&HelloEchoSerivce::getEcho, this));
    #endif

    #if defined(ZAD_3)
        Rest::Routes::Get(_router, "/hello/echo2/:id", Rest::Routes::bind(&HelloEchoSerivce::getEchoParam, this));
    #endif

    #if defined(ZAD_4) || defined(ZAD_5)
        Rest::Routes::Get(_router, "/messages", Rest::Routes::bind(&HelloEchoSerivce::getMessages, this));
    #endif

        _end_point->setHandler(_router.handler());

        _end_point->serve();
    }
};
int main(int argc, char** argv) {
    try {
        HelloEchoSerivce service(8080, 1);
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
#endif

