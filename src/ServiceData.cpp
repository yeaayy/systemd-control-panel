#define __USE_POSIX2

#include "ServiceData.h"

#include <unistd.h>

#include <cstdio>

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#include "utils.h"

std::ostream& operator<<(std::ostream& out, ServiceStatus status)
{
    switch (status) {
        case ServiceStatus::UNKNOWN:
            return out << "UNKNOWN";
        case ServiceStatus::RUNNING:
            return out << "RUNNING";
        case ServiceStatus::STOPPED:
        return out << "STOPPED";
        default:
            return out << "(Unknown status)";
    }
}

ServiceData::ServiceData(const char* _fifo_in, const char* _fifo_out, std::string name)
    : status(ServiceStatus::UNKNOWN)
    , enabled(false)
    , name(name)
    , fifo_in(_fifo_in)
    , fifo_out(_fifo_out)
{}

void
ServiceData::update()
{
    std::stringstream ss;
    ss << "systemctl status " << name;
    std::string command = ss.str();

    FILE *process = popen(command.c_str(), "r");
    if (!process) {
        status = ServiceStatus::UNKNOWN;
        return;
    }

    ss.seekp(0);
    char buff[BUFSIZ];
    int readed;
    while((readed = fread(buff, 1, BUFSIZ, process))) {
        ss.write(buff, readed);
    }

    log = ss.str();
    pclose(process);

    check_active();
    check_loaded();
}

void
ServiceData::check_active()
{
    size_t start = log.find("Active: ");
    size_t end;
    if (start == std::string::npos) goto unknown;
    start += 8;

    end = log.find(" ", start);
    if (end == std::string::npos) goto unknown;

    set_status(
        log.substr(start, end - start) == "active"
        ? ServiceStatus::RUNNING
        : ServiceStatus::STOPPED
    );
    return;

unknown:
    set_status(ServiceStatus::UNKNOWN);
}

void
ServiceData::check_loaded()
{
    size_t start = log.find("Loaded:");
    size_t end;

    if (start == std::string::npos) goto not_found;
    start = log.find("; ", start);
    if (start == std::string::npos) goto not_found;
    start += 2;

    end = log.find("; ", start);
    if (end == std::string::npos) goto not_found;

    set_enabled(log.substr(start, end - start) == "enabled");
    return;

    not_found:
    set_enabled(false);
}

std::string
ServiceData::get_name()
{
    return name;
}

ServiceStatus
ServiceData::get_status()
{
    return status;
}

bool
ServiceData::is_enabled()
{
    return enabled;
}

void
ServiceData::set_status(ServiceStatus status)
{
    if (this->status == status) return;

    this->status = status;
    status_changed.emit(status);
}

void
ServiceData::set_enabled(bool enabled)
{
    if (this->enabled == enabled) return;

    this->enabled = enabled;
    enabled_changed.emit(enabled);
}

sigc::signal<void, ServiceStatus>&
ServiceData::signal_status_changed()
{
    return status_changed;
}

sigc::signal<void, bool>&
ServiceData::signal_enabled_changed()
{
    return enabled_changed;
}

void
ServiceData::enable()
{
    send_command(fifo_in, fifo_out, MessageCode::enable, name.c_str());
    update();
}

void
ServiceData::disable()
{
    send_command(fifo_in, fifo_out, MessageCode::disable, name.c_str());
    update();
}

void
ServiceData::start()
{
    send_command(fifo_in, fifo_out, MessageCode::start, name.c_str());
    update();
}

void
ServiceData::stop()
{
    send_command(fifo_in, fifo_out, MessageCode::stop, name.c_str());
    update();
}
