#ifndef __EXEC_SAVE_H
#define __EXEC_SAVE_H

class ExecSave {
protected:
  void (*m_append_text)(const std::string&, gpointer);
  std::string *m_output;
  bool m_quit_on_newline;
  bool m_newline_found;
  gchar **m_argv;
  GIOChannel *m_ch_stdin, *m_ch_stdout, *m_ch_stderr;
  GPid m_child_pid;
  GMainLoop *m_loop;
  int m_exit_status;
  gpointer m_data;
  bool m_abort, m_all_done, m_running;
public:
  ExecSave(); 
  ~ExecSave() { }

  signed char run(std::string& exe, std::string& output, bool quit_on_newline, void (*)(const std::string&, gpointer) = NULL, gpointer data = NULL);
  bool find_it(const char *exe_name, std::string& location);
  bool is_running() { return (m_all_done == true ? false : true); }
  bool is_done() { return m_all_done; }
  void key_press_event(const char *utf8_str, int size);
  static gboolean channel_read(GIOChannel *channel, GIOCondition cond, ExecSave* exec);
  static gboolean spawn_func(ExecSave *exec);
  static void child_watch(GPid pid, gint status, ExecSave *exec);
  void abort();
  void sigkill();
  bool is_quit() { return m_abort; }
};

#endif
