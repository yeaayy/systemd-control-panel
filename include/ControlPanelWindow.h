#pragma once

#include <memory>
#include <string>
#include <vector>

#include <gtkmm/grid.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/window.h>

#include "ServiceData.h"
#include "ServiceRow.h"

class SelectItemWindow;

class ControlPanelWindow
    : public Gtk::Window
{
public:
    ControlPanelWindow(const char* fifo_in, const char* fifo_out, std::vector<std::string> init_list);
    sigc::signal<void, std::vector<std::string>&>& signal_list_changed();

private:
    void add_header_row();
    void update_view();
    void add_service(std::string name);

    void action_add_service();
    void action_selected_move_up();
    void action_selected_move_down();
    void action_selected_remove();
    void action_selected_toggle_autostart();
    void action_open_menu(ServiceRow* row, Gtk::Button& button);
    void on_add_service(std::string name);
    void trigger_list_changed();

private:
    const char* fifo_in;
    const char* fifo_out;
    Gtk::Grid grid;
    std::vector<std::unique_ptr<ServiceRow>> rows;

    int selected_row;
    Gtk::Menu menu;
    Gtk::MenuItem menu_move_up;
    Gtk::MenuItem menu_move_down;
    Gtk::MenuItem menu_toggle_autostart;

    SelectItemWindow* window_selector;
    std::vector<std::string> available_services;
    sigc::signal<void, std::vector<std::string>&> list_changed;
};
