#pragma once

#include "sigc++/signal.h"
#include <vector>

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>
#include <gtkmm/window.h>

class SelectItemWindow
    : public Gtk::Window
{
public:
    SelectItemWindow(std::string title, std::string name, std::vector<std::string>& list);
    sigc::signal<void, std::string>& signal_selected();

private:
    void add_item(const Glib::ustring& item_name);
    void on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
    void on_choose_button_clicked();

private:
    std::vector<std::string>& list;
    sigc::signal<void, std::string> selected;

    Gtk::Box vbox;
    Gtk::ScrolledWindow scrolled_window;
    Gtk::TreeView tree_view;
    Gtk::Button choose_button;

    Glib::RefPtr<Gtk::ListStore> list_store;
    struct ModelColumns : public Gtk::TreeModel::ColumnRecord {
        ModelColumns() { add(col_item); }
        Gtk::TreeModelColumn<Glib::ustring> col_item;
    } columns;
};
