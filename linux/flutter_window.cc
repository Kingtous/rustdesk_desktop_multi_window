//
// Created by yangbin on 2022/1/11.
//

#include "flutter_window.h"

#include <iostream>

#include "include/desktop_multi_window/desktop_multi_window_plugin.h"
#include "desktop_multi_window_plugin_internal.h"

extern void fl_register_plugins(FlPluginRegistry *registry);

bool rustdesk_is_subwindow = false;

namespace
{

  WindowCreatedCallback _g_window_created_callback = nullptr;

}

FlutterWindow::FlutterWindow(
    int64_t id,
    const std::string &args,
    const std::shared_ptr<FlutterWindowCallback> &callback) : callback_(callback), id_(id)
{
  window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated(GTK_WINDOW(window_), FALSE);
  gtk_window_set_default_size(GTK_WINDOW(window_), 1280, 720);
  gtk_window_set_position(GTK_WINDOW(window_), GTK_WIN_POS_CENTER);
  // try setting icon for rustdesk, which uses the system cache 
  // mainly for the icon in appimage.
  GtkIconTheme* theme = gtk_icon_theme_get_default();
  gint icons[4] = {256, 128, 64, 32};
  for (int i = 0; i < 4; i++) {
    GdkPixbuf* icon = gtk_icon_theme_load_icon(theme, "rustdesk", icons[i], GTK_ICON_LOOKUP_NO_SVG, NULL);
    if (icon != nullptr) {
      gtk_window_set_icon(GTK_WINDOW(window_), icon);
    }
  }
  // set gtk header bar
  // fix for the frame of sub window exists after hide header bar on wayland
  GtkHeaderBar *header_bar = GTK_HEADER_BAR(gtk_header_bar_new());
  gtk_widget_show(GTK_WIDGET(header_bar));
  gtk_header_bar_set_title(header_bar, "");
  gtk_header_bar_set_show_close_button(header_bar, TRUE);
  gtk_window_set_titlebar(GTK_WINDOW(window_), GTK_WIDGET(header_bar));

  gtk_widget_show(GTK_WIDGET(window_));

  g_autoptr(FlDartProject)
      project = fl_dart_project_new();
  const char *entrypoint_args[] = {"multi_window", g_strdup_printf("%ld", id_), args.c_str(), nullptr};
  fl_dart_project_set_dart_entrypoint_arguments(project, const_cast<char **>(entrypoint_args));

  auto fl_view = fl_view_new(project);
  gtk_widget_show(GTK_WIDGET(fl_view));
  gtk_container_add(GTK_CONTAINER(window_), GTK_WIDGET(fl_view));

  if (_g_window_created_callback)
  {
    _g_window_created_callback(FL_PLUGIN_REGISTRY(fl_view));
  }
  // indicate to plugin injections using extern
  rustdesk_is_subwindow = true;
  fl_register_plugins(FL_PLUGIN_REGISTRY(fl_view));
  g_autoptr(FlPluginRegistrar)
      desktop_multi_window_registrar =
          fl_plugin_registry_get_registrar_for_plugin(FL_PLUGIN_REGISTRY(fl_view), "DesktopMultiWindowPlugin");
  desktop_multi_window_plugin_register_with_registrar_internal(desktop_multi_window_registrar);

  window_channel_ = WindowChannel::RegisterWithRegistrar(desktop_multi_window_registrar, id_);

  g_signal_connect(window_, "delete-event", G_CALLBACK(onWindowClose), this);
  g_signal_connect(window_, "window-state-event",
                   G_CALLBACK(onWindowStateChange), this);
  g_signal_connect(window_, "focus-in-event",
                   G_CALLBACK(onWindowFocus), this);
  g_signal_connect(window_, "focus-out-event",
                   G_CALLBACK(onWindowBlur), this);
  g_signal_connect(window_, "configure-event",
                   G_CALLBACK(onWindowMove), this);
  g_signal_connect(window_, "check-resize",
                   G_CALLBACK(onWindowResize), this);
  // enhance drag
  g_signal_connect(window_, "event-after", G_CALLBACK(onWindowEventAfter),
                   this);
  this->findEventBox(GTK_WIDGET(fl_view));
  this->pressedEmissionHook = g_signal_add_emission_hook(
      g_signal_lookup("button-press-event", GTK_TYPE_WIDGET), 0,
      onMousePressHook, this, NULL);

  gtk_widget_grab_focus(GTK_WIDGET(fl_view));
  gtk_widget_hide(GTK_WIDGET(window_));
}

WindowChannel *FlutterWindow::GetWindowChannel()
{
  return window_channel_.get();
}

int64_t FlutterWindow::GetId()
{
  return this->id_;
}

FlutterWindow::~FlutterWindow()
{
  g_signal_remove_emission_hook(g_signal_lookup("button-press-event", GTK_TYPE_WIDGET), this->pressedEmissionHook);
  if (this->window_)
  {
    gtk_widget_destroy(this->window_);
    this->window_ = nullptr;
  }
  if (this->window_channel_ != nullptr)
  {
    this->window_channel_.reset();
    this->window_channel_ = nullptr;
  }
};

void desktop_multi_window_plugin_set_window_created_callback(WindowCreatedCallback callback)
{
  _g_window_created_callback = callback;
}

void _emitEvent(const char *event_name, FlutterWindow *self)
{
  g_autoptr(FlValue) result_data = fl_value_new_map();
  fl_value_set_string_take(result_data, "eventName",
                           fl_value_new_string(event_name));
  fl_value_set_string_take(result_data, "windowId",
                           fl_value_new_int(self->GetId()));
  self->GetWindowChannel()->InvokeMethodSelfVoid("onEvent", result_data);
}

gboolean onWindowClose(GtkWidget *widget, GdkEvent *, gpointer arg)
{
  auto *self = static_cast<FlutterWindow *>(arg);
  _emitEvent("close", self);
  // destory hook
  if (!self->isPreventClose)
  {
    if (auto channel = self->GetWindowChannel())
    {
      auto args = fl_value_new_map();
      channel->InvokeMethodSelfVoid("onDestroy", args);
    }
    if (auto callback = self->callback_.lock())
    {
      callback->OnWindowClose(self->id_);
      callback->OnWindowDestroy(self->id_);
    }
  }
  return self->isPreventClose;
}

gboolean onWindowFocus(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  auto *self = static_cast<FlutterWindow *>(data);
  _emitEvent("focus", self);
  return false;
}

gboolean onWindowBlur(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  auto *self = static_cast<FlutterWindow *>(data);
  _emitEvent("blur", self);
  return false;
}

gboolean onWindowResize(GtkWidget *widget, gpointer data)
{
  auto *self = static_cast<FlutterWindow *>(data);
  _emitEvent("resize", self);
  return false;
}

gboolean onWindowMove(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  auto *self = static_cast<FlutterWindow *>(data);
  _emitEvent("move", self);
  return false;
}

gboolean onWindowStateChange(GtkWidget *widget,
                             GdkEventWindowState *event,
                             gpointer arg)
{
  auto *self = static_cast<FlutterWindow *>(arg);
  if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)
  {
    if (!self->maximized)
    {
      self->maximized = true;
      _emitEvent("maximize", self);
    }
  }
  if (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED)
  {
    if (!self->minimized)
    {
      self->minimized = true;
      _emitEvent("minimize", self);
    }
  }
  if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)
  {
    if (!self->fullscreen)
    {
      self->fullscreen = true;
      _emitEvent("enter-full-screen", self);
    }
  }
  if (self->maximized &&
      !(event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED))
  {
    self->maximized = false;
    _emitEvent("unmaximize", self);
  }
  if (self->minimized &&
      !(event->new_window_state & GDK_WINDOW_STATE_ICONIFIED))
  {
    self->minimized = false;
    _emitEvent("restore", self);
  }
  if (self->fullscreen &&
      !(event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN))
  {
    self->fullscreen = false;
    _emitEvent("leave-full-screen", self);
  }
  return false;
}