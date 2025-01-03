#include "ControlPanelWindow.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/enums.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/object.h>
#include <gtkmm/scrolledwindow.h>

#include "SelectItemWindow.h"
#include "ServiceData.h"
#include "ServiceRow.h"
#include "utils.h"

ControlPanelWindow::ControlPanelWindow(const char* _fifo_in, const char* _fifo_out, std::vector<std::string> init_list)
    : window_selector(nullptr)
    , available_services(get_all_services())
    , fifo_in(_fifo_in)
    , fifo_out(_fifo_out)
{
    set_title("Systemd Control Panel");
    set_default_size(500, 300);

    auto vbox = Gtk::make_managed<Gtk::Box>();
    vbox->set_orientation(Gtk::ORIENTATION_VERTICAL);

    auto add_button = Gtk::make_managed<Gtk::Button>();
    auto add_image = Gtk::make_managed<Gtk::Image>("list-add", Gtk::ICON_SIZE_BUTTON);
    add_button->set_image(*add_image);
    add_button->set_label("Add service");
    add_button->signal_clicked().connect(sigc::mem_fun(*this, &ControlPanelWindow::action_add_service));
    vbox->add(*add_button);

    grid.set_row_spacing(5);
    grid.set_column_spacing(10);
    grid.set_margin_top(10);
    grid.set_margin_bottom(10);
    grid.set_margin_start(10);
    grid.set_margin_end(10);

    menu_move_up.set_label("Move up");
    menu_move_down.set_label("Move down");
    auto menu_remove = Gtk::make_managed<Gtk::MenuItem>("Remove");
    menu_move_up.signal_activate().connect(
        sigc::mem_fun(*this, &ControlPanelWindow::action_selected_move_up));
    menu_move_down.signal_activate().connect(
        sigc::mem_fun(*this, &ControlPanelWindow::action_selected_move_down));
    menu_remove->signal_activate().connect(
        sigc::mem_fun(*this, &ControlPanelWindow::action_selected_remove));
    menu_toggle_autostart.signal_activate().connect(
        sigc::mem_fun(*this, &ControlPanelWindow::action_selected_toggle_autostart));

    menu.add(menu_toggle_autostart);
    menu.add(menu_move_up);
    menu.add(menu_move_down);
    menu.add(*menu_remove);
    menu.show_all();

    for (auto& service : init_list) {
        add_service(service);
    }
    update_view();

    auto scrolled_window = Gtk::make_managed<Gtk::ScrolledWindow>();
    scrolled_window->add(grid);
    scrolled_window->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    vbox->pack_start(*scrolled_window, Gtk::PACK_EXPAND_WIDGET);

    add(*vbox);

    show_all_children();
}

void
ControlPanelWindow::add_header_row()
{
    auto label_name = Gtk::make_managed<Gtk::Label>("Service");
    auto label_runnning = Gtk::make_managed<Gtk::Label>("Running");
    auto label_action = Gtk::make_managed<Gtk::Label>("Action");

    label_runnning->set_alignment(0.5, 0.5);
    label_name->set_xalign(0);
    label_action->set_xalign(0);

    label_name->set_hexpand();

    grid.attach(*label_name, 1, 0, 1, 1);
    grid.attach(*label_runnning, 2, 0, 1, 1);
    grid.attach(*label_action, 3, 0, 1, 1);
}

void
ControlPanelWindow::update_view()
{
    for (auto child : grid.get_children()) {
        grid.remove(*child);
    }

    int i = 1;
    add_header_row();
    for (auto& row : rows) {
        row->put(grid, i++);
    }
    grid.show_all_children();
}

void
ControlPanelWindow::action_add_service()
{
    if (window_selector) {
        window_selector->grab_focus();
        return;
    }
    window_selector = new SelectItemWindow("Add Service", "Service", available_services);
    window_selector->signal_selected().connect(
        sigc::mem_fun(*this, &ControlPanelWindow::on_add_service));
    window_selector->signal_hide().connect([&]() {
        delete window_selector;
        window_selector = nullptr;
        set_sensitive(true);
    });
    window_selector->show();
    window_selector->set_transient_for(*this);
    set_sensitive(false);
}

void
ControlPanelWindow::on_add_service(std::string name)
{
    add_service(name);
    trigger_list_changed();
}

void
ControlPanelWindow::add_service(std::string name)
{
    available_services.erase(std::find(available_services.begin(), available_services.end(), name));
    rows.push_back(std::make_unique<ServiceRow>(std::make_unique<ServiceData>(fifo_in, fifo_out, name)));
    auto& row = rows.back();
    row->get_data()->update();
    row->put(grid, rows.size());
    row->signal_on_open_menu().connect(
        sigc::mem_fun(*this, &ControlPanelWindow::action_open_menu));
    grid.show_all_children();
}

sigc::signal<void, std::vector<std::string>&>&
ControlPanelWindow::signal_list_changed()
{
    return list_changed;
}

void
ControlPanelWindow::trigger_list_changed()
{
    std::vector<std::string> result;
    for (auto& row : rows) {
        result.push_back(row->get_data()->get_name());
    }
    list_changed.emit(result);
}

void
ControlPanelWindow::action_open_menu(ServiceRow* row, Gtk::Button& button)
{
    if (row->get_data()->is_enabled()) {
        menu_toggle_autostart.set_label("Disable autostart");
    } else {
        menu_toggle_autostart.set_label("Enable autostart");
    }

    selected_row = -1;
    for (int i = 0; i < rows.size(); i++) {
        if (rows[i].get() == row) {
            selected_row = i;
            break;
        }
    }

    menu_move_up.property_sensitive() = selected_row != 0;
    menu_move_down.property_sensitive() = selected_row != (rows.size() - 1);
    menu.popup_at_widget(&button, Gdk::GRAVITY_SOUTH, Gdk::GRAVITY_NORTH, nullptr);
}

void
ControlPanelWindow::action_selected_move_up()
{
    // std::cout << "Move up: " << rows[selected_row]->get_data()->get_name() << "\n";
    std::swap(rows[selected_row], rows[selected_row - 1]);
    trigger_list_changed();
    update_view();
}

void
ControlPanelWindow::action_selected_move_down()
{
    // std::cout << "Move down: " << rows[selected_row]->get_data()->get_name() << "\n";
    std::swap(rows[selected_row], rows[selected_row + 1]);
    trigger_list_changed();
    update_view();
}

void
ControlPanelWindow::action_selected_remove()
{
    auto row = std::move(rows[selected_row]);
    rows.erase(rows.begin() + selected_row);
    available_services.push_back(row->get_data()->get_name());
    std::sort(available_services.begin(), available_services.end());
    trigger_list_changed();
    update_view();
}

void
ControlPanelWindow::action_selected_toggle_autostart()
{
    auto& data = rows[selected_row]->get_data();
    if (data->is_enabled()) {
        data->disable();
    } else {
        data->enable();
    }
}
