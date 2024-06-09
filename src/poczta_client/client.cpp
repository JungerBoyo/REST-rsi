#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <pistache/client.h>
#include <CLI/CLI.hpp>
// #include <png.h>
#include <libbase64.h>
#include <nlohmann/json.hpp>

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

bool load_png(const char* filename, std::string image, unsigned& width, unsigned& height);

int main(int argc, char** argv) {
    CLI::App app("Simple CLI app implementing client interface for poczta polska API.");

    std::string username = "";
    std::string password = "";
    std::string ipv4 = "";
    int port = 0;
    bool ssl = false;
    app.add_option("-a,--ipv4-address", ipv4, "Server ip address.");
    app.add_option("-o,--port", port, "Server port.");
    app.add_option("-u,--username", username, "BasicAuth username.");
    app.add_option("-p,--password", password, "BasicAuth password.");
    app.add_flag("-s,--ssl", ssl, "Use SSL/HTTPS.");

    std::string final_address = "";
    auto auth_header = std::make_shared<Http::Header::Authorization>();
    auto content_type_header = std::make_shared<Http::Header::ContentType>(MIME(Application, Json));

    bool post = false;
    std::string body = "";

    const auto app_base_callback = [&]() {
        final_address = fmt::format("{}://{}:{}/poczta/", ssl ? "https" : "http", ipv4, port);
        auth_header->setBasicUserPassword(username, password);
    };

    // GET (OK)
    auto get_welcome_message = app.add_subcommand("getWelcomeMessage", "Gets welcome message.");
    std::string get_welcome_message_name = "";
    get_welcome_message->add_option("-n,--name", get_welcome_message_name, "Name w witaj X.");
    get_welcome_message->callback([&](){
        app_base_callback();
        final_address += fmt::format("welcome?name={}", get_welcome_message_name);
    });

    // GET (OK)
    auto get_version = app.add_subcommand("getVersion", "");
    get_version->callback([&](){
        app_base_callback();
        final_address += "version";
    });

    // GET (OK)
    auto check_single_shipment = app.add_subcommand("checkSingleShipment", "");
    std::string check_single_shipment_tracking_num = "";
    check_single_shipment->add_option("-n,--num", check_single_shipment_tracking_num, "");
    check_single_shipment->callback([&](){
        app_base_callback();
        final_address += fmt::format("checkSingleShipment?number={}", check_single_shipment_tracking_num);
    });

    // GET (OK)
    auto check_single_local_shipment = app.add_subcommand("checkSingleLocalShipment", "");
    std::string check_single_local_shipment_tracking_num = "";
    check_single_local_shipment->add_option("-n,--num", check_single_local_shipment_tracking_num, "");
    check_single_local_shipment->callback([&](){
        app_base_callback();
        final_address += fmt::format("checkSingleLocalShipment?number={}", check_single_local_shipment_tracking_num);
    });

    // POST
    auto check_shipments_by_date = app.add_subcommand("checkShipmentsByDate", "");
    std::vector<std::string> check_shipments_by_date_nums{};
    check_shipments_by_date->add_option("-n,--nums", check_shipments_by_date_nums, "");
    std::string check_shipments_by_date_begin = "";
    check_shipments_by_date->add_option("-b,--begin", check_shipments_by_date_begin, "");
    std::string check_shipments_by_date_end = "";
    check_shipments_by_date->add_option("-e,--end", check_shipments_by_date_end, "");
    check_shipments_by_date->callback([&](){
        app_base_callback();
        post = true;
        nlohmann::json j {
            {"numbers", check_shipments_by_date_nums},
            {"startDate", check_shipments_by_date_begin},
            {"endDate", check_shipments_by_date_end}
        };
        body = j.dump();
        final_address += "checkShipmentsByDate";
    });

    // POST
    auto check_local_shipments_by_date = app.add_subcommand("checkLocalShipmentsByDate", "");
    std::vector<std::string> check_local_shipments_by_date_nums{};
    check_local_shipments_by_date->add_option("-n,--nums", check_local_shipments_by_date_nums, "");
    std::string check_local_shipments_by_date_begin = "";
    check_local_shipments_by_date->add_option("-b,--begin", check_local_shipments_by_date_begin, "");
    std::string check_local_shipments_by_date_end = "";
    check_local_shipments_by_date->add_option("-e,--end", check_local_shipments_by_date_end, "");
    check_local_shipments_by_date->callback([&](){
        app_base_callback();
        post = true;
        nlohmann::json j {
            {"numbers", check_local_shipments_by_date_nums},
            {"startDate", check_local_shipments_by_date_begin},
            {"endDate", check_local_shipments_by_date_end}
        };
        body = j.dump();
        final_address += "checkLocalShipmentsByDate";
    });

    // GET (OK)
    auto get_max_shipments = app.add_subcommand("getMaxShipments", "");
    get_max_shipments->callback([&](){
        app_base_callback();
        final_address += "getMaxShipments";
    });

    // GET (OK)
    auto check_local_shipments = app.add_subcommand("checkLocalShipments", "");
    std::vector<std::string> check_local_shipments_nums;
    check_local_shipments->add_option("-n,--nums", check_local_shipments_nums, "");
    check_local_shipments->callback([&](){
        app_base_callback();
        final_address += "checkLocalShipments?";
        for (const auto& value : check_local_shipments_nums) {
            final_address += fmt::format("numbers={}&", value);
        }
        final_address.pop_back();
    });

    // GET (OK)
    auto check_shipments = app.add_subcommand("checkShipments", "");
    std::vector<std::string> check_shipments_nums;
    check_shipments->add_option("-n,--nums", check_shipments_nums, "");
    check_shipments->callback([&](){
        app_base_callback();
        final_address += "checkShipments?";
        for (const auto& value : check_shipments_nums) {
            final_address += fmt::format("numbers={}&", value);            
        }
        final_address.pop_back();
    });

    // POST (OK)
    auto get_single_shipment_by_bar_code = app.add_subcommand("getSingleShipmentByBarCode", "");
    get_single_shipment_by_bar_code->callback([&](){
        app_base_callback();
        post = true;

        std::ifstream s_barcode("barcode.png");
        std::stringstream buffer;
        buffer << s_barcode.rdbuf();
        body = buffer.str();
        std::string str(4ULL * body.length() / 3ULL, 0);
        size_t outlen = 4ULL * body.length() / 3ULL;
        base64_encode(body.c_str(), body.length(), str.data(), &outlen, 0);
        str.resize(outlen);
        final_address += "getSingleShipmentByBarCode";

        body = fmt::format("{{ \"imageData\": \"{}\" }}", std::move(str));
    });

    CLI11_PARSE(app, argc, argv);

    spdlog::info("final address = \"{}\", body = {}", final_address, body);
    
    Http::Experimental::Client client{};
    auto opts = Http::Experimental::Client::options().threads(1).maxConnectionsPerHost(8);
    client.init(opts);

    try {
        if (post) {
            print(client.post(final_address).body(std::move(body)).header(auth_header).header(content_type_header).send());
        } else {
            print(client.get(final_address).header(auth_header).send());
        }
    } catch (std::invalid_argument e) {
        spdlog::error(e.what());
        client.shutdown();
        return 1;
    }

    while (!response_str_set) {}
    fmt::print("{}\n", response_str);

    client.shutdown();
}
