#pragma once

#include <vector>
#include <string>

std::vector<std::string> get_all_services();

int send_command(const char *target, const char *cmd);
void send_message(const char *target, const char* msg);
