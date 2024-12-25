#include "ServiceRow.h"

#include <gtkmm/enums.h>

#include "ServiceData.h"

ServiceRow::ServiceRow(std::unique_ptr<ServiceData> _data)
    : data(std::move(_data))
{
    label.set_alignment(0);
    action_button.set_alignment(0, 0.5);

    autostart.property_sensitive() = false;
    running.property_sensitive() = false;
    running.set_halign(Gtk::ALIGN_CENTER);

    action_button.signal_clicked().connect(
        sigc::mem_fun(*this, &ServiceRow::on_action_button_clicked));
    menu_button.signal_clicked().connect([&]() {
        on_open_menu.emit(this, menu_button);
    });
    data->signal_enabled_changed().connect(
        sigc::mem_fun(*this, &ServiceRow::update_autostart));
    data->signal_status_changed().connect(
        sigc::mem_fun(*this, &ServiceRow::update_button));

    update_button(data->get_status());
    update_autostart(data->is_enabled());
    label.set_label(data->get_name());
    menu_button.set_label("...");
}

void
ServiceRow::put(Gtk::Grid& dst, int row)
{
    dst.attach(autostart, 0, row);
    dst.attach(label, 1, row);
    dst.attach(running, 2, row);
    dst.attach(action_button, 3, row);
    dst.attach(menu_button, 4, row);
}

std::unique_ptr<ServiceData>&
ServiceRow::get_data()
{
    return data;
}

void
ServiceRow::update_button(ServiceStatus status)
{
    switch (status) {
        case ServiceStatus::RUNNING:
            action_button.set_label("Stop");
            action_button.property_sensitive() = true;
            running.set_tooltip_text("Service running");
            running.set_active(true);
            break;

        case ServiceStatus::STOPPED:
            action_button.set_label("Start");
            action_button.property_sensitive() = true;
            running.set_tooltip_text("Service not running");
            running.set_active(false);
            break;

        case ServiceStatus::UNKNOWN:
            action_button.set_label("Start");
            action_button.property_sensitive() = false;
            break;
    }
}

void
ServiceRow::update_autostart(bool autostart)
{
    this->autostart.set_active(autostart);
    if (autostart) {
        this->autostart.set_tooltip_text("Autostart enabled");
    } else {
        this->autostart.set_tooltip_text("Autostart not enabled");
    }
}

void
ServiceRow::on_action_button_clicked()
{
    switch (data->get_status()) {
        case ServiceStatus::RUNNING:
            data->stop();
            break;
        case ServiceStatus::STOPPED:
            data->start();
            break;
        default:;
    }
}

sigc::signal<void, ServiceRow*, Gtk::Button&>&
ServiceRow::signal_on_open_menu()
{
    return on_open_menu;
}
