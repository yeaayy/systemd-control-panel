#include "ControlPanelWindow.h"

#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <chrono>
#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <glibmm/error.h>
#include <glibmm/optioncontext.h>
#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>

#include "constant.h"
#include "utils.h"

int
monitor(const char* in, const char* out)
{
    if (getuid() != 0) {
        std::cerr << "Monitor mode must be run as root\n";
        return 1;
    }

    FILE* file;
    int len;
    char buff[256];

    send_message(out, MessageCode::begin);

    while (true) {
        file = fopen(in, "rb");
        if (!file) return 0;

        const char* action;
        switch (static_cast<MessageCode>(fgetc(file))) {
            case MessageCode::exit:
                return 0;
            case MessageCode::start:
                action = "start";
                break;
            case MessageCode::stop:
                action = "stop";
                break;
            case MessageCode::enable:
                action = "enable";
                break;
            case MessageCode::disable:
                action = "disable";
                break;
            default:
                return 1;
        }
        len = fgetc(file);
        if (fread(buff, 1, len, file) != len) {
            return 1;
        }
        buff[len] = '\0';
        fclose(file);

        // Check if service contain valid service name
        for (const char* ch = buff; *ch; ch++) {
            if (!(*ch >= '0' && *ch <= '9'
            || *ch >= 'A' && *ch <= 'Z'
            || *ch >= 'a' && *ch <= 'z')) {
                return 1;
            }
        }

        std::string cmd = std::string("systemctl ") + action + " " + buff;
#ifndef NDEBUG
        std::cout << "Running: " << cmd << std::endl;
#endif
        system(cmd.c_str());
        send_message(out, MessageCode::done);
    }
}

void
run_monitor(
    int argc,
    char** argv,
    const char* in,
    const char* out,
    std::promise<bool>* monitor_started,
    std::promise<bool>* monitor_ended,
    std::future<ControlPanelWindow*>* window_result)
{
    std::stringstream ss;
    ss << "pkexec " << argv[0] << " monitor " << in << " " << out;
    for (int i = 1; i < argc; i++) {
        ss << " " << argv[i];
    }
    std::string cmd = ss.str();
    int result = system(cmd.c_str());
    if (result == 256 || result == 0) {
        monitor_ended->set_value(true);
        auto window = window_result->get();
        if (window->gobj()) {
            window->close();
        }
    } else if (result) {
        monitor_started->set_value(false);
    }
}

void
wait_monitor_started(const char* target, std::promise<bool>* monitor_started)
{
    FILE* file = fopen(target, "rb");
    if (!file) return;
    auto code = static_cast<MessageCode>(fgetc(file));
    fclose(file);

    if (code == MessageCode::begin) {
        monitor_started->set_value(true);
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

    std::promise<bool> monitor_started_promise;
    std::promise<bool> monitor_ended_promise;
    std::promise<ControlPanelWindow*> window_promise;
    auto monitor_started = monitor_started_promise.get_future();
    auto monitor_ended = monitor_ended_promise.get_future();
    auto window_result = window_promise.get_future();
    std::thread monitor_thread(run_monitor, argc, argv, c_fifo2, c_fifo1, &monitor_started_promise, &monitor_ended_promise, &window_result);
    std::thread wait_monitor(wait_monitor_started, c_fifo1, &monitor_started_promise);

    monitor_started.wait();
    int result = 1;
    if (monitor_started.get()) {
        auto app = Gtk::Application::create(argc, argv, APP_ID);
        ControlPanelWindow win(c_fifo1, c_fifo2, read_service_list(config_path));
        win.signal_list_changed().connect(
            sigc::bind(sigc::ptr_fun(write_service_list), config_path));
        window_promise.set_value(&win);
        result = app->run(win);

        if (monitor_ended.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout) {
            send_message(c_fifo2, MessageCode::exit);
        }
    } else {
        send_message(c_fifo1, MessageCode::exit);
    }
    wait_monitor.join();
    monitor_thread.join();
    remove(c_fifo1);
    remove(c_fifo2);
    return result;
}
