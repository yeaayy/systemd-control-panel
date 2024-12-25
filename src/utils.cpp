#include "utils.h"

#include <cstdlib>
#include <dirent.h>

#include <cstring>

#include <algorithm>
#include <iostream>

#include "constant.h"

std::vector<std::string>
get_all_services()
{
    std::vector<std::string> services;
    DIR* dir = opendir(SERVICE_DIR);
    struct dirent* de;
    while ((de = readdir(dir)) != NULL) {
        if (de->d_name[0] != '.') {
            services.push_back(de->d_name);
        }
    }
    closedir(dir);

    std::sort(services.begin(), services.end());
    return services;
}

int
send_command(const char *target, const char *cmd)
{
    FILE* file;
    char buff[BUFSIZ], *result;
    int len = strlen(cmd);

    for (;;) {
        file = fopen(target, "w+");
        if (!file) return 0;

        fwrite(cmd, len, 1, file);
        fputc('\n', file);

        result = fgets(buff, BUFSIZ, file);
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
