#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <pistache/client.h>

using namespace Pistache;

std::atomic<bool> response_str_set = false;
std::string response_str = "";

void print(Async::Promise<Http::Response>&& response) {
    try {
        response.then(
            [](const Http::Response& response) { response_str = response.body(); response_str_set = true; },
            Async::Throw
        );    
    } catch (std::exception e) {
        spdlog::error(e.what());
    }
}

int main(int argc, char** argv) {
    const auto base_addr = fmt::format("localhost:{}/", argv[1]);

    Http::Experimental::Client client{};

    auto opts = Http::Experimental::Client::options().threads(1).maxConnectionsPerHost(8);
    client.init(opts);

    std::string body = "{ \"author\": \"Eliasz\", \"id\": 4, \"contents\": \"Czesc\" }";

    switch (argv[2][0]) {
        // get messages
    case '0':
        print(client.get(base_addr + "messages").send());
        break;
        // get message
    case '1':
        print(client.get(base_addr + fmt::format("message/{}", argv[3])).send());
        break;
        // post message
    case '2':
        print(client.post(base_addr + "message").body(std::move(body)).send());
        break;
        // put message
    case '3':
        print(client.put(base_addr + fmt::format("message/{}", argv[3])).body(std::move(body)).send());
        break;
        // del message
    case '4':
        print(client.del(base_addr + fmt::format("message/{}", argv[3])).send());
        break;
        // query messages
    case '5':
        print(client.get(base_addr + fmt::format("messages/{}", argv[3])).send());
        break;
    default:
        spdlog::error("No such option");
        break; 
    }
    while (!response_str_set) {}

    fmt::print("{}\n", response_str);

    client.shutdown();

}