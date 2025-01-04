#pragma once

#include <vector>
#include <string>

enum class MessageCode {
    // Additional code
    exit = 1,
    begin,
    done,

    // Action code
    enable = 8,
    disable,
    start,
    stop,
};

std::vector<std::string> get_all_services();

int send_command(const char* in, const char* out, MessageCode, const char* service);
void send_message(const char* target, MessageCode);
