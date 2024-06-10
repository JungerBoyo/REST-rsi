#ifndef LAB13_SHARED_H
#define LAB13_SHARED_H

#include <string>

struct Message {
    std::string author;
    std::string contents;
};

struct Subscription {
    std::string client_callback_url;
};

#endif