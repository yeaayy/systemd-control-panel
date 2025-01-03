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
send_command(const char* in, const char* out, const char* cmd)
{
    FILE* file;
    char buff[BUFSIZ], *result;
    int len = strlen(cmd);

    for (;;) {
        file = fopen(out, "w");
        if (!file) return 0;

        fwrite(cmd, len, 1, file);
        fputc('\n', file);
        fclose(file);

        file = fopen(in, "r");
        if (!file) return 0;

        result = fgets(buff, BUFSIZ, file);
        if (!result) return 0;
        fclose(file);

        if (strcmp(result, "done\n") == 0) {
            return 1;
        }
    }
}

void
send_message(const char *target, const char* msg)
{
    FILE* file = fopen(target, "w");
    if (!file) {
        std::cerr << "Failed to send \"" << msg << "\"\n";
        return;
    }

    fwrite(msg, strlen(msg), 1, file);
    fclose(file);
}
