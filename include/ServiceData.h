#pragma once

#include <string>

#include <sigc++/signal.h>

enum class ServiceStatus {
    UNKNOWN,
    RUNNING,
    STOPPED
};

class ServiceData
{
public:
    ServiceData(const char *ipc, std::string name);
    void update();
    ServiceStatus get_status();
    std::string get_name();
    bool is_enabled();

    sigc::signal<void, ServiceStatus>& signal_status_changed();
    sigc::signal<void, bool>& signal_enabled_changed();

    void enable();
    void disable();
    void start();
    void stop();

private:
    void check_active();
    void check_loaded();
    void set_status(ServiceStatus status);
    void set_enabled(bool enabled);

private:
    const char *ipc;
    bool enabled;
    ServiceStatus status;
    std::string name;
    std::string log;

    sigc::signal<void, bool> enabled_changed;
    sigc::signal<void, ServiceStatus> status_changed;
};
