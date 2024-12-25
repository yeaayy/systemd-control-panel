#include "ControlPanelWindow.h"

#include <algorithm>
#include <unistd.h>
#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <future>
#include <iostream>
#include <sstream>
#include <thread>

#include <glibmm/error.h>
#include <glibmm/optioncontext.h>
#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>
#include <vector>

#include "constant.h"
#include "glib.h"
#include "utils.h"

int
monitor(const char *target)
{
    if (getuid() != 0) {
        std::cerr << "Monitor mode must be run as root\n";
        return 1;
    }

    FILE* file;
    char buff[BUFSIZ];

    send_message(target, "start\n");

    while (true) {
        file = fopen(target, "r+");
        if (!file) return 0;
        char *cmd = fgets(buff, BUFSIZ, file);
        fclose(file);

        std::cout << "Running: " << cmd;
        if (strcmp(cmd, "exit\n") == 0) return 0;
        system(cmd);
        send_message(target, "done\n");
    }
    return 0;
}

void
run_monitor(int argc, char** argv, const char* target, std::promise<bool>* promise)
{
    std::stringstream ss;
    ss << "pkexec " << argv[0] << " --monitor=" << target;
    for (int i = 1; i < argc; i++) {
        ss << " " << argv[i];
    }
    std::string cmd = ss.str();
    int result = system(cmd.c_str());
    if (result) {
        promise->set_value(false);
    }
}

void
wait_monitor_started(const char* target, std::promise<bool>* promise)
{
    char buff[BUFSIZ];
    FILE* file = fopen(target, "r");
    if (!file) return;
    char* cmd = fgets(buff, BUFSIZ, file);
    fclose(file);

    if (strcmp(cmd, "start\n") == 0) {
        promise->set_value(true);
    }
}

std::vector<std::string>
read_service_list(std::string& config_path)
{
    std::vector<std::string> result;
    if (access(config_path.c_str(), F_OK) != 0) {
        // File not exists.
        return result;
    }
    FILE* file = fopen(config_path.c_str(), "r");
    if (!file) {
        perror("Config file");
        return result;
    }
    char buff[BUFSIZ];
    int len;
    while (fgets(buff, BUFSIZ, file)) {
        std::string service = buff;
        service.pop_back();
        result.push_back(service);
    }
    return result;
}

void
write_service_list(std::vector<std::string>& list, std::string& config_path)
{
    FILE* file = fopen(config_path.c_str(), "w");
    if (!file) {
        perror("Config file");
        return;
    }
    for (auto& service : list) {
        fwrite(service.c_str(), service.size(), 1, file);
        fputc('\n', file);
    }
}

int
main(int argc, char *argv[])
{
    if (argc > 1 && strncmp(argv[1], "--monitor=", 10) == 0) {
        return monitor(&argv[1][10]);
    }

    std::stringstream ss;
    ss << g_get_user_data_dir() << "/" DATA_DIR_NAME;
    std::string s_tmpfile = ss.str();
    const char* tmpfile = s_tmpfile.c_str();
    if (access(tmpfile, F_OK) != 0) {
        mkdir(tmpfile, 0700);
    }

    std::string config_path = s_tmpfile + "/" CONFIG_NAME;

    ss << "/ipc";
    s_tmpfile = ss.str();
    tmpfile = s_tmpfile.c_str();
    if (access(tmpfile, F_OK) == 0) {
        remove(tmpfile);
    }
    if (mkfifo(tmpfile, 0777) != 0) {
        perror("mkfifo");
        return 1;
    }

    std::promise<bool> promise;
    auto future = promise.get_future();
    std::thread monitor_thread(run_monitor, argc, argv, tmpfile, &promise);
    std::thread wait_monitor(wait_monitor_started, tmpfile, &promise);

    future.wait();
    int result = 1;
    if (future.get()) {
        auto app = Gtk::Application::create(argc, argv, APP_ID);
        ControlPanelWindow win(tmpfile, read_service_list(config_path));
        win.signal_list_changed().connect(
            sigc::bind(sigc::ptr_fun(write_service_list), config_path));
        result = app->run(win);
        send_message(tmpfile, "exit\n");
    } else {
        send_message(tmpfile, "no_perm\n");
    }
    wait_monitor.join();
    monitor_thread.join();
    remove(tmpfile);
    return result;
}
