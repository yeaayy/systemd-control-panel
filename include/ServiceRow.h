#pragma once

#include <memory>

#include <gtkmm/checkbutton.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>

#include "ServiceData.h"

class ServiceRow
{
public:
    ServiceRow(std::unique_ptr<ServiceData> data);
    void put(Gtk::Grid& dst, int row);
    std::unique_ptr<ServiceData>& get_data();
    sigc::signal<void, ServiceRow*, Gtk::Button&>& signal_on_open_menu();

private:
    void update_button(ServiceStatus);
    void update_autostart(bool);
    void on_action_button_clicked();

private:
    sigc::signal<void, ServiceRow*, Gtk::Button&> on_open_menu;
    std::unique_ptr<ServiceData> data;
    Gtk::Button action_button;
    Gtk::Button menu_button;
    Gtk::Label label;
    Gtk::CheckButton autostart;
    Gtk::CheckButton running;
};
