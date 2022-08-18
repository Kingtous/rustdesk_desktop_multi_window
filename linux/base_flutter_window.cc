//
// Created by boyan on 2022/1/27.
//

#include "base_flutter_window.h"

void BaseFlutterWindow::Show() {
  auto window = GetWindow();
  if (!window) {
    return;
  }
  gtk_widget_show(GTK_WIDGET(window));
}

void BaseFlutterWindow::Hide() {
  auto window = GetWindow();
  if (!window) {
    return;
  }
  gtk_widget_hide(GTK_WIDGET(window));
}

void BaseFlutterWindow::Focus() {
  auto window = GetWindow();
  if (!window) {
    return;
  }
  gtk_window_deiconify(window);
  gtk_window_present(window);
}

void BaseFlutterWindow::SetFullscreen(bool fullscreen) {
  auto window = GetWindow();
  if (!window) {
    return;
  }
  if (fullscreen)
    gtk_window_fullscreen(window);
  else
    gtk_window_unfullscreen(window);
}

void BaseFlutterWindow::SetBounds(double_t x, double_t y, double_t width, double_t height) {
  auto window = GetWindow();
  if (!window) {
    return;
  }
  gtk_window_move(GTK_WINDOW(window), static_cast<gint>(x), static_cast<gint>(y));
  gtk_window_resize(GTK_WINDOW(window), static_cast<gint>(width), static_cast<gint>(height));
}

void BaseFlutterWindow::SetTitle(const std::string &title) {
  auto window = GetWindow();
  if (!window) {
    return;
  }
  gtk_window_set_title(GTK_WINDOW(window), title.c_str());
}

void BaseFlutterWindow::Center() {
  auto window = GetWindow();
  if (!window) {
    return;
  }
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
}

void BaseFlutterWindow::Close() {
  auto window = GetWindow();
  if (!window) {
    return;
  }
  gtk_window_close(GTK_WINDOW(window));
}

void BaseFlutterWindow::StartDragging() {
  auto window = GetWindow();
  if (!window) {
    return;
  }
  auto screen = gtk_window_get_screen(window);
  auto display = gdk_screen_get_display(screen);
  auto seat = gdk_display_get_default_seat(display);
  auto device = gdk_seat_get_pointer(seat);

  gint root_x, root_y;
  gdk_device_get_position(device, nullptr, &root_x, &root_y);
  guint32 timestamp = (guint32)g_get_monotonic_time();

  gtk_window_begin_move_drag(window, 1, root_x, root_y, timestamp);
}

bool BaseFlutterWindow::IsMaximized() { return this->maximized; }

void BaseFlutterWindow::Maximize() {
  auto window = GetWindow();
  if (!window) {
    return;
  }
  gtk_window_maximize(window);
  this->maximized = true;
}

void BaseFlutterWindow::Unmaximize() {
  auto window = GetWindow();
  if (!window) {
    return;
  }
  gtk_window_unmaximize(window);
  this->maximized = false;
}

void BaseFlutterWindow::Minimize() {
  auto window = GetWindow();
  if (!window) {
    return;
  }
  gtk_window_iconify(window);
}

void BaseFlutterWindow::ShowTitlebar(bool show) {
  auto window = GetWindow();
  if (!window) {
    return;
  }
  gtk_window_set_decorated(window, show);
}