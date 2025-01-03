#include "ControlPanelWindow.h"

#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <future>
#include <iostream>
#include <sstream>
#include <vector>

#include <glibmm/error.h>
#include <glibmm/optioncontext.h>
#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>

#include "constant.h"
#include "glib.h"
#include "utils.h"

int
monitor(const char* in, const char* out)
{
    if (getuid() != 0) {
        std::cerr << "Monitor mode must be run as root\n";
        return 1;
    }

    FILE* file;
    char buff[BUFSIZ];

    send_message(out, "start\n");

    while (true) {
        file = fopen(in, "r");
        if (!file) return 0;
        char *cmd = fgets(buff, BUFSIZ, file);
        fclose(file);

#ifndef NDEBUG
        std::cout << "Running: " << cmd;
#endif
        if (strcmp(cmd, "exit\n") == 0) return 0;
        system(cmd);
        send_message(out, "done\n");
    }
    return 0;
}

void
run_monitor(int argc, char** argv, const char* in, const char* out, std::promise<bool>* promise)
{
    std::stringstream ss;
    ss << "pkexec " << argv[0] << " monitor " << in << " " << out;
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
    fclose(file);
}

bool
create_fifo(const char* path)
{
    if (access(path, F_OK) == 0) {
        if (remove(path)) {
            perror("remove");
            return false;
        }
    }
    if (mkfifo(path, 0777) != 0) {
        perror("mkfifo");
        return false;
    }
    return true;
}

int
main(int argc, char *argv[])
{
    if (argc == 4 && strcmp(argv[1], "monitor") == 0) {
        return monitor(argv[2], argv[3]);
    }

    std::string data_dir = std::string(g_get_user_data_dir()) + "/" DATA_DIR_NAME;
    if (access(data_dir.c_str(), F_OK) != 0) {
        mkdir(data_dir.c_str(), 0700);
    }

    auto config_path = data_dir + "/" CONFIG_NAME;
    auto fifo1 = data_dir + "/fifo1";
    auto fifo2 = data_dir + "/fifo2";
    auto c_fifo1 = fifo1.c_str();
    auto c_fifo2 = fifo2.c_str();

    if (!create_fifo(c_fifo1) || !create_fifo(c_fifo2)) {
        return 1;
    }

    std::promise<bool> promise;
    auto future = promise.get_future();
    std::thread monitor_thread(run_monitor, argc, argv, c_fifo2, c_fifo1, &promise);
    std::thread wait_monitor(wait_monitor_started, c_fifo1, &promise);

    future.wait();
    int result = 1;
    if (future.get()) {
        auto app = Gtk::Application::create(argc, argv, APP_ID);
        ControlPanelWindow win(fifo1.c_str(), fifo2.c_str(), read_service_list(config_path));
        win.signal_list_changed().connect(
            sigc::bind(sigc::ptr_fun(write_service_list), config_path));
        result = app->run(win);
        send_message(c_fifo2, "exit\n");
    } else {
        send_message(c_fifo1, "no_perm");
    }
    wait_monitor.join();
    monitor_thread.join();
    remove(c_fifo1);
    remove(c_fifo2);
    return result;
}
