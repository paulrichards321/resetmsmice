/*
To manually compile this (those are backticks not quotes):
  g++ -Wall -g gui.cc -o resetmsmice-gui `pkg-config gtk+-3.0 --cflags --libs`
Then execute it using:
  ./resetmsmice-gui
*/

#include <cstring>
#include <cerrno>
#include <vector>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <wait.h>
#include <sstream>
#include <iostream>
#include <libusb-1.0/libusb.h>
#include <gtk/gtk.h>
#include "execSave.h"
#include "resetWindow.h"

static const char * about_text = 
  "This package fixes scroll wheel issues with certain Wireless Microsoft mice in X.org, where the vertical wheel scrolls abnormally fast. Only needed if you dual boot between Microsoft Windows and a linux distribution.\n\n"
  "Known to fix the vertical scroll wheel issue with the following models (and others related):\n"
  "    Microsoft Wireless Mouse 1000\n"
  "    Microsoft Wireless Optical Desktop 3000\n"
  "    Microsoft Wireless Mobile Mouse 3500\n"
  "    Microsoft Wireless Mobile Mouse 4000\n"
  "    Microsoft Comfort Mouse 4500\n"
  "    Microsoft Wireless Mouse 5000\n"
  "    Microsoft Sculpt Mouse\n\n"
  "This program basically just resets a setting in the mouse through usb communications and then exits.";

static const char * auth_text =
  "You may need to provide authentication to disable or enable resetmsmice on bootup.";

ResetWindow::ResetWindow(GApplication *app)
{ 
  std::stringstream ss;

  ss << std::endl;
  m_newline = ss.str();

  m_resized = false; 
  m_buffer = NULL;
  m_app = app;
  m_text_view = 0;
}


void ResetWindow::set_status_text(const std::string& text)
{
  gtk_label_set_text(m_status_label, text.c_str());
}


void ResetWindow::get_status_text()
{
  char self_location[256];
  std::string str_status = "Reset any Microsoft mice on bootup is currently: ";
  std::string enable_boot_path, output;
  std::stringstream err_msg; 

  if (readlink("/proc/self/exe", self_location, 255) > 0) {
    struct stat info;
    if (strstr(self_location, "/local/") == NULL) {
      enable_boot_path = "/usr/sbin/resetmsmice-enable-boot";
    } else {
      enable_boot_path = "/usr/local/sbin/resetmsmice-enable-boot";
    }
    if (stat(enable_boot_path.c_str(), &info) != 0) {
      enable_boot_path = "";
    }
  }
  if (enable_boot_path.length() == 0 && exec_save.find_it("resetmsmice-enable-boot", enable_boot_path) == false) {
    err_msg << std::endl << "Error: Cannot find resetmsmice-enable-boot script. Please make sure this script is installed in your PATH." << std::endl;
    std::cerr << err_msg.str();
    create_terminal_view();
    append_terminal_text(err_msg.str());
    str_status += "UNKNOWN ";
    set_status_text(str_status);
    return;
  }
  enable_boot_path += " --status";
  if (exec_save.run(enable_boot_path, output, true) == 0) {
    str_status += output;
    str_status += " ";
  } else {
    str_status += "UNKNOWN ";
    err_msg << std::endl << "Error getting status: " << strerror(errno) << std::endl;
    std::cerr << err_msg.str();
    create_terminal_view();
    append_terminal_text(err_msg.str());
  }
  set_status_text(str_status);
  return;
}    


bool ResetWindow::create_terminal_view()
{
  GtkWidget *text_view, *scroll_window;
  gint width, height;

  if (m_resized && m_buffer) { 
    gtk_widget_grab_focus(GTK_WIDGET(m_text_view));
    return true; 
  } else if (m_resized) {
    return false;
  }

  m_resized = true;

  gtk_window_get_size( GTK_WINDOW(m_window), &width, &height);
  text_view = GTK_WIDGET( gtk_text_view_new());       
  m_text_view = GTK_TEXT_VIEW(text_view);
  gtk_text_view_set_input_purpose(m_text_view, GTK_INPUT_PURPOSE_PASSWORD);
  gtk_widget_set_can_focus(GTK_WIDGET(m_text_view), TRUE);
  gtk_widget_set_can_default(GTK_WIDGET(m_text_view), TRUE);
  g_signal_connect(G_OBJECT (text_view), "key_press_event", G_CALLBACK (ResetWindow::key_press_event), this);
  //gtk_text_view_set_editable( GTK_TEXT_VIEW(text_view), TRUE);
  //gtk_text_view_set_wrap_mode( GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
  gtk_text_view_set_cursor_visible( GTK_TEXT_VIEW(text_view), TRUE);
  scroll_window = GTK_WIDGET( gtk_scrolled_window_new(NULL, NULL) );
  gtk_container_add(GTK_CONTAINER(scroll_window), text_view);
  gtk_widget_set_size_request(text_view, width-20, 300);
  gtk_widget_set_size_request(scroll_window, width-20, 300);
  gtk_grid_attach(m_grid, GTK_WIDGET(scroll_window), 0, 6, 5, 1);
  gtk_widget_show_all(scroll_window);
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(text_view), TRUE);
  m_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  gtk_widget_grab_focus(GTK_WIDGET(m_text_view));
  if (m_buffer) {
    return true;
  }
  std::cerr << "Fatal Error: No terminal text view buffer! resetmsmice-gui will not work!" << std::endl;
  return false;
}


gboolean ResetWindow::key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  char utf8_str[32];
  gint size = 0;
  if (data == NULL) return FALSE;
  ResetWindow *resetwindow = static_cast<ResetWindow*>(data);
  if (event->keyval == GDK_KEY_KP_Enter || event->keyval == GDK_KEY_Return)
  {
    resetwindow->append_terminal_text(resetwindow->m_newline);
    resetwindow->exec_save.key_press_event(resetwindow->m_newline.c_str(), resetwindow->m_newline.size());
  }
  else if (!event->is_modifier)
  {
    /// We want to ignore irrelevant modifiers like ScrollLock
    GdkModifierType modifiers = gtk_accelerator_get_default_mod_mask();
    if (event->keyval == GDK_KEY_c
      && (event->state & modifiers) == GDK_CONTROL_MASK)
    {
      std::string warning = "Ctrl-C hit, terminated process. ";
      resetwindow->append_terminal_text(resetwindow->m_newline);
      resetwindow->append_terminal_text(warning);
      resetwindow->exec_save.sigkill();
      return TRUE;
    }
    guint32 uni_char = gdk_keyval_to_unicode(event->keyval);
    size = g_unichar_to_utf8(uni_char, utf8_str);
    if (size < 0)
    {
      size = 0;
    }
    else if (size < 32)
    {
      utf8_str[size] = 0;
    }
    else
    {
      utf8_str[31] = 0;
      size = 31;
    }
    resetwindow->exec_save.key_press_event((const char*) utf8_str, (int) size);
  }
  return TRUE;
}


void ResetWindow::clear_terminal()
{
  if (m_buffer) {
    gtk_text_buffer_set_text(m_buffer, "", -1);
    gtk_text_view_set_input_purpose(m_text_view, GTK_INPUT_PURPOSE_PASSWORD);
  }
}

void ResetWindow::set_terminal_text(const std::string& text)
{
  if (m_buffer) {
    GtkTextIter end;
    gtk_text_buffer_set_text(m_buffer, "", -1);
    gtk_text_buffer_get_iter_at_offset(m_buffer, &end, -1);
    gtk_text_buffer_insert_with_tags_by_name(m_buffer, &end, text.c_str(), -1, "monospace", NULL);
    gtk_text_view_set_input_purpose(m_text_view, GTK_INPUT_PURPOSE_PASSWORD);
  }
}

void ResetWindow::append_terminal_text(const std::string& text)
{
  if (m_buffer) {
     GtkTextIter end;
     gtk_text_buffer_get_iter_at_offset(m_buffer, &end, -1);
     gtk_text_buffer_insert_with_tags_by_name(m_buffer, &end, text.c_str(), -1, "monospace", NULL);
     gtk_text_view_set_input_purpose(m_text_view, GTK_INPUT_PURPOSE_PASSWORD);
  }
}


void ResetWindow::append_terminal_text_callback(const std::string& text, gpointer data)
{
  ResetWindow *resetwindow = static_cast<ResetWindow*>(data);
  if (resetwindow == NULL) return;
  resetwindow->append_terminal_text(text);
}


void ResetWindow::enable_boot(bool enable)
{
  std::string sudo_cmd, enable_boot_path, output;
  std::stringstream err_msg;
  char self_location[256];

  create_terminal_view();
  clear_terminal();

  if (exec_save.find_it("sudo", sudo_cmd) == false) { 
    err_msg << "Error: Cannot find sudo binary in PATH." << std::endl;
    std::cerr << err_msg.str();
    set_terminal_text(err_msg.str());
    return;
  }
    
  if (readlink("/proc/self/exe", self_location, 255) > 0) {
    struct stat info;
    if (strstr(self_location, "/local/") == NULL) {
      enable_boot_path = "/usr/sbin/resetmsmice-enable-boot";
    } else {
      enable_boot_path = "/usr/local/sbin/resetmsmice-enable-boot";
    }
    if (stat(enable_boot_path.c_str(), &info) != 0) {
      enable_boot_path = "";
    }
  }
  if (enable_boot_path.length() == 0 && exec_save.find_it("resetmsmice-enable-boot", enable_boot_path) == false) {
    err_msg << "Error: Cannot find resetmsmice-enable-boot script. Please make sure this script is installed in your PATH." << std::endl;
    std::cerr << err_msg.str();
	  set_terminal_text(err_msg.str());
    return;
  }

  sudo_cmd += " -S ";
  sudo_cmd += enable_boot_path;
  if (enable == true) {
    sudo_cmd += " --enable --quiet";
  } else {
    sudo_cmd += " --disable";
  }
  append_terminal_text(sudo_cmd);
  append_terminal_text(m_newline);
  int status = exec_save.run(sudo_cmd, output, false, &ResetWindow::append_terminal_text_callback, this);
  if (exec_save.is_quit()){
    return;
  }
  if (status == 0) {
    append_terminal_text("Done.");
    get_status_text();
  } else {
    err_msg << std::endl << "Error running: " << sudo_cmd << std::endl;
    err_msg << "Error message: " << strerror(errno) << std::endl;
    append_terminal_text(err_msg.str());
  }
  return;
}


void ResetWindow::enable_clicked(GtkWidget *widget, gpointer data)
{
  ResetWindow *resetwindow = static_cast<ResetWindow*>(data);
  if (resetwindow->exec_save.is_running() == false) {
    resetwindow->enable_boot(true);
  }
}


void ResetWindow::disable_clicked(GtkWidget *widget, gpointer data)
{
  ResetWindow *resetwindow = static_cast<ResetWindow*>(data);
  if (resetwindow->exec_save.is_running() == false) {
    resetwindow->enable_boot(false);
  }
}


void ResetWindow::reset_now_clicked(GtkWidget *widget, gpointer data)
{
  ResetWindow *resetwindow = static_cast<ResetWindow*>(data);
  if (resetwindow->exec_save.is_running() == false) 
  {
    resetwindow->reset_now();
  }
}


void ResetWindow::reset_now()
{
  char self_location[256];
  std::string resetmsmice_cmd, output;
  std::stringstream err_msg;
  std::string sudo_cmd;

  create_terminal_view();
  clear_terminal();
    
  if (readlink("/proc/self/exe", self_location, 255) > 0) {
    struct stat info;
    if (strstr(self_location, "/local/") == NULL) {
      resetmsmice_cmd = "/usr/bin/resetmsmice";
    } else {
      resetmsmice_cmd = "/usr/local/bin/resetmsmice";
    }
    if (stat(resetmsmice_cmd.c_str(), &info) != 0) {
      resetmsmice_cmd = "";
    }
  }
  if (resetmsmice_cmd.length() == 0 && exec_save.find_it("resetmsmice", resetmsmice_cmd) == false) {
    err_msg << "Error: Cannot find resetmsmice binary in PATH." << std::endl;
    std::cerr << err_msg.str();
    set_terminal_text(err_msg.str());
    return;
  }
  if (exec_save.run(resetmsmice_cmd, output, false, &ResetWindow::append_terminal_text_callback, this) != LIBUSB_ERROR_ACCESS) {
    return; // return on success or any other error messages besides permission issue
  }

  output.clear();
  err_msg << "Need root or administrator rights to run resetmsmice, please provide root password." << std::endl << std::endl;
  std::cerr << err_msg.str();
  set_terminal_text(err_msg.str());

  if (exec_save.find_it("sudo", sudo_cmd) == false) {
    err_msg << "Error: Cannot find sudo binary in PATH." << std::endl;
    std::cerr << err_msg.str();
    append_terminal_text(err_msg.str());
    return;
  }

  sudo_cmd += " -S ";
  sudo_cmd += resetmsmice_cmd;

  append_terminal_text(sudo_cmd);
  append_terminal_text(m_newline);
  exec_save.run(sudo_cmd, output, false, &ResetWindow::append_terminal_text_callback, this);
}


GtkWidget* ResetWindow::create()
{
  GtkWidget *enable_button, *disable_button, *reset_now_button;
  GtkLabel *about_label;
  GError *err = NULL;
  struct stat statbuf;
  const char *icon_path = "/usr/share/icons/hicolor/scalable/apps/resetmsmice.svg";
  
  m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_resizable(GTK_WINDOW(m_window), FALSE);
  
  if (stat(icon_path, &statbuf) == -1) {
    icon_path = "/usr/local/share/icons/hicolor/scalable/apps/resetmsmice.svg";
  }
  gtk_window_set_icon_from_file(GTK_WINDOW(m_window), icon_path, &err);
  gtk_application_add_window(GTK_APPLICATION(m_app), GTK_WINDOW(m_window));

  m_grid = GTK_GRID(gtk_grid_new());
  gtk_container_add(GTK_CONTAINER(m_window), GTK_WIDGET(m_grid));
  gtk_widget_set_margin_start(GTK_WIDGET(m_grid), 5);
  gtk_widget_set_margin_end(GTK_WIDGET(m_grid), 5);
  gtk_widget_set_margin_top(GTK_WIDGET(m_grid), 5);
  gtk_widget_set_margin_bottom(GTK_WIDGET(m_grid), 5);

  enable_button = gtk_button_new_with_label("Enable");
  gtk_widget_set_focus_on_click(enable_button, FALSE);

  disable_button = gtk_button_new_with_label("Disable");
  gtk_widget_set_focus_on_click(disable_button, FALSE);
  
  reset_now_button = gtk_button_new_with_label("Reset Now");
  gtk_widget_set_focus_on_click(reset_now_button, FALSE);

  m_status_label = GTK_LABEL(gtk_label_new(""));
  about_label = GTK_LABEL(gtk_label_new(about_text));

  GtkWidget *auth_label = gtk_label_new(auth_text);
  gtk_label_set_justify(GTK_LABEL(auth_label), GTK_JUSTIFY_LEFT);
  gtk_widget_set_halign(auth_label, GTK_ALIGN_START);
  
  gtk_window_set_title(GTK_WINDOW(m_window), "Reset Microsoft Mice");

  gtk_grid_set_column_homogeneous(m_grid, TRUE);
  gtk_grid_set_column_spacing(m_grid, 4);
  gtk_grid_set_row_homogeneous(m_grid, FALSE);
  gtk_grid_set_row_spacing(m_grid, 8);
  
  gtk_label_set_single_line_mode(GTK_LABEL(about_label), FALSE);
  gtk_label_set_line_wrap(GTK_LABEL(about_label), TRUE);
  gtk_label_set_line_wrap(GTK_LABEL(auth_label), TRUE);
  gtk_label_set_max_width_chars(GTK_LABEL(about_label), 80);
  gtk_label_set_max_width_chars(GTK_LABEL(auth_label), 80);
  
  gtk_grid_attach(m_grid, GTK_WIDGET(about_label), 0, 0, 5, 1);
  gtk_grid_attach(m_grid, GTK_WIDGET(m_status_label), 0, 1, 3, 1);
  gtk_grid_attach(m_grid, enable_button, 3, 1, 1, 1);
  gtk_grid_attach(m_grid, disable_button, 4, 1, 1, 1);
  gtk_grid_attach(m_grid, auth_label, 0, 3, 5, 1);
  gtk_grid_attach(m_grid, reset_now_button, 0, 5, 1, 1);

  g_signal_connect(G_OBJECT(m_window), "delete-event",
                   G_CALLBACK(&ResetWindow::delete_event), NULL);
  g_signal_connect(G_OBJECT(m_window), "destroy", 
                   G_CALLBACK(&ResetWindow::destroy), this);
  g_signal_connect_after(G_OBJECT(enable_button), "clicked",
                   G_CALLBACK(&ResetWindow::enable_clicked), this);
  g_signal_connect_after(G_OBJECT(disable_button), "clicked",
                   G_CALLBACK(&ResetWindow::disable_clicked), this);
  g_signal_connect_after(G_OBJECT(reset_now_button), "clicked",
                   G_CALLBACK(&ResetWindow::reset_now_clicked), this);
     
  gtk_widget_show_all(m_window);
                 
  get_status_text();

  return m_window;
}


void ResetWindow::destroy(GObject *object, gpointer user_data)
{
  if (user_data)
  {
    ResetWindow *resetwindow = (ResetWindow*) user_data;
    resetwindow->exec_save.abort();
    g_application_quit(G_APPLICATION(resetwindow->m_app));
  }
}


gboolean ResetWindow::delete_event(GtkWidget*, gpointer)
{
  return FALSE;
}


ResetWindow::~ResetWindow() 
{
  g_object_unref(G_OBJECT(m_builder));
}


static void activate(GtkApplication *app, gpointer user_data)
{
  ResetWindow *resetwindow = new ResetWindow(G_APPLICATION(app));
  resetwindow->create();
}

int main(int argc, char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new("com.github.paulrichards321.resetmsmice", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}

