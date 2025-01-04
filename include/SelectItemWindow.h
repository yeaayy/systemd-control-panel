#pragma once

#include <vector>

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treemodelfilter.h>
#include <gtkmm/treeview.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/window.h>
#include <sigc++/signal.h>

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
    void on_search_query_changed();
    bool filter_func(const Gtk::TreeModel::const_iterator& iter);

private:
    std::vector<std::string>& list;
    sigc::signal<void, std::string> selected;
    Glib::ustring search_query;

    Gtk::Box vbox;
    Gtk::ScrolledWindow scrolled_window;
    Gtk::TreeView tree_view;
    Gtk::Button choose_button;
    Gtk::SearchEntry search_entry;

    Glib::RefPtr<Gtk::ListStore> list_store;
    Glib::RefPtr<Gtk::TreeModelFilter> filter;
    struct ModelColumns : public Gtk::TreeModel::ColumnRecord {
        ModelColumns() { add(col_item); }
        Gtk::TreeModelColumn<Glib::ustring> col_item;
    } columns;
};
