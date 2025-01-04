#include "utils.h"

#include <dirent.h>
#include <strings.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <iostream>
#include <string>

#include "constant.h"

std::vector<std::string>
get_all_services()
{
    std::vector<std::string> services;
    DIR* dir = opendir(SERVICE_DIR);
    struct dirent* de;
    int len;
    while ((de = readdir(dir)) != NULL) {
        if (de->d_name[0] == '.') {
            continue;
        }
        for (char* ch = de->d_name; *ch; ch++) {
            if (*ch == '@') {
                goto skip;
            }
        }
        len = strlen(de->d_name);
        if (len < 8) goto skip;
        if (strcmp(de->d_name + len - 8, ".service") != 0) goto skip;
        services.push_back(std::string(de->d_name).substr(0, len - 8));
        skip:;
    }
    closedir(dir);

    std::sort(services.begin(), services.end());
    return services;
}

int
send_command(const char* in, const char* out, MessageCode code, const char* service)
{
    FILE* file;
    int len = strlen(service);
    if (len > 255) return 0;

    for (;;) {
        file = fopen(out, "wb");
        if (!file) return 0;

        fputc(static_cast<int>(code), file);
        fputc(len, file);
        fwrite(service, len, 1, file);
        fclose(file);

        file = fopen(in, "rb");
        if (!file) return 0;

        int reply = fgetc(file);
        fclose(file);

        if (reply == -1) return 0;
        if (static_cast<MessageCode>(reply) == MessageCode::done) {
            return 1;
        }
    }
}

void
send_message(const char* target, MessageCode code)
{
    FILE* file = fopen(target, "wb");
    if (!file) {
        std::cerr << "Failed to send code " << static_cast<int>(code) << "\n";
        return;
    }

    fputc(static_cast<int>(code), file);
    fclose(file);
}
