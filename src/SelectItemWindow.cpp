#include "SelectItemWindow.h"

SelectItemWindow::SelectItemWindow(
    std::string title,
    std::string name,
    std::vector<std::string>& list)
    : list(list)
{
    set_title(title);
    set_default_size(300, 200);

    list_store = Gtk::ListStore::create(columns);
    tree_view.set_model(list_store);
    tree_view.append_column(name, columns.col_item);

    for (auto& item : list) {
        add_item(item);
    }

    tree_view.signal_row_activated().connect(
        sigc::mem_fun(*this, &SelectItemWindow::on_row_activated));

    scrolled_window.add(tree_view);
    scrolled_window.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    vbox.pack_start(scrolled_window, Gtk::PACK_EXPAND_WIDGET);

    choose_button.set_label("Choose");
    choose_button.signal_clicked().connect(
        sigc::mem_fun(*this, &SelectItemWindow::on_choose_button_clicked));
    vbox.pack_start(choose_button, Gtk::PACK_SHRINK);

    vbox.set_orientation(Gtk::ORIENTATION_VERTICAL);

    add(vbox);
    show_all_children();
}

void
SelectItemWindow::add_item(const Glib::ustring& item_name)
{
    auto row = *(list_store->append());
    row[columns.col_item] = item_name;
}

void
SelectItemWindow::on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column)
{
    std::string path_str = path.to_string();
    int index = std::stoi(path_str);
    selected.emit(list[index]);
    close();
}

void
SelectItemWindow::on_choose_button_clicked()
{
    auto selection = tree_view.get_selection();
    if (auto iter = selection->get_selected()) {
        Gtk::TreeModel::Path path = list_store->get_path(iter);
        std::string path_str = path.to_string();
        int index = std::stoi(path_str);
        selected.emit(list[index]);
        close();
    }
}

sigc::signal<void, std::string>&
SelectItemWindow::signal_selected()
{
    return selected;
}
