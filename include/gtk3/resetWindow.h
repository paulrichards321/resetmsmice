#ifndef __RESETWINDOW_H
#define __RESETWINDOW_H

class ResetWindow {
protected:
  GApplication *m_app;
  GtkBuilder *m_builder;
  GtkWidget *m_window;
  GtkLabel *m_status_label;
  GtkTextBuffer *m_buffer;
  GtkTextView *m_text_view;
  GtkGrid *m_grid;
  bool m_resized;
  ExecSave exec_save;
  std::string m_newline;
public:
  ResetWindow(GApplication *app);
  ~ResetWindow();
  GtkWidget* create();
  void reset_now();
  bool create_terminal_view();
  void clear_terminal(); 
  void get_status_text();
  void set_terminal_text(const std::string&);
  void append_terminal_text(const std::string&);
  void set_status_text(const std::string&);
  void enable_boot(bool);
  // Gtk Callback functions
  static void enable_clicked(GtkWidget *widget, gpointer data);
  static void disable_clicked(GtkWidget *widget, gpointer data);
  static void reset_now_clicked(GtkWidget *widget, gpointer data);
  static void destroy(GObject*, gpointer);
  static gboolean delete_event(GtkWidget*, gpointer);
  static void append_terminal_text_callback(const std::string&, gpointer);
  static gboolean key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data);
//  static gboolean button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
  //static gboolean enable_clicked(GtkWidget *widget, GdkEventButton *event, gpointer data);
};

#endif
