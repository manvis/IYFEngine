#include "ProgressDialog.hpp"
#include <glibmm/i18n.h>
#include <iostream>

namespace iyf::launcher {
ProgressDialog::ProgressDialog(Gtk::Window& parent, std::function<void()>&& task)
    : dialog(_("Please Wait"), parent, true), vbox(Gtk::ORIENTATION_VERTICAL, 1), progressBar(), task(std::move(task)) {
    dialog.set_attached_to(parent);
    dialog.set_transient_for(parent);
    
    dialog.set_default_size(300, -1);
    dialog.get_content_area()->add(progressBar);

    sigc::slot<bool> slot = sigc::mem_fun(*this, &ProgressDialog::timerTick);
    recurringTask = Glib::signal_timeout().connect(slot, TickIntervalMs);
}

ProgressDialog::~ProgressDialog() {
    if (recurringTask.connected()) {
        recurringTask.disconnect();
    }
}

int ProgressDialog::run() {
    future = std::async(std::launch::async, task);

    dialog.show_all();
    return dialog.run();
}

bool ProgressDialog::timerTick() {
    if (future.valid()) {
        const auto status = future.wait_for(std::chrono::milliseconds(0));
        if (status == std::future_status::ready) {
            future.get();
            dialog.response(0);
            return false;
        } else {
            progressBar.pulse();
        }
    }

    return true;
}
}