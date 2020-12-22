#include <cstring>
#include <cerrno>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <wait.h>
#include <sstream>
#include <iostream>
#include <gtk/gtk.h>
#include <glib.h>
#include "execSave.h"

signed char ExecSave::run(std::string& exe, std::string& output, bool quit_on_newline, void (*append_text)(const std::string&, gpointer), gpointer data)
{
  std::vector<std::string> str_args;
  std::string root_cmd;
  std::string::size_type start, end, size;
  int count = 0;
 
  if (m_all_done == false) {
    return -1;
  }
  m_running = false;
  m_child_pid = 0;
  m_all_done = false;
  m_output = &output;
  m_exit_status = -1;
  m_append_text = append_text;
  m_quit_on_newline = quit_on_newline;
  m_newline_found = false;
  m_data = data;
  m_ch_stdin = NULL;
  m_ch_stdout = NULL;
  m_ch_stderr = NULL;
  m_loop = NULL;
  for (start = 0, count = 0; start < exe.length(); count++) {
    end = exe.find(' ', start);
    if (end == std::string::npos) {
      size = exe.length() - start;
    } else {
      size = end - start;
    }
    if (count == 0) {
      root_cmd = exe.substr(start, size);
    } 
    str_args.push_back(exe.substr(start, size));
    start += size + 1;
  }
  gchar *argv[str_args.size()+1];
  std::cout << "Executing: ";
  
  unsigned int sub = 0;
  while (sub < str_args.size()) {
    argv[sub] = (gchar*) str_args[sub].c_str(); 
    std::cout << argv[sub] << " ";
    sub++;
  }
  argv[sub] = NULL;
  std::cout << std::endl << std::flush;
 
  m_argv = argv;
  m_loop = g_main_loop_new(NULL, TRUE);
  g_idle_add((GSourceFunc) spawn_func, this);
  g_main_loop_run(m_loop);

  m_loop = NULL;
  m_running = false;
  m_all_done = true;
  return m_exit_status;
} 


gboolean ExecSave::spawn_func(ExecSave *exec)
{
  gint fd_stdin=0, fd_stdout=1, fd_stderr=2;
  GError* spawn_error = NULL;

  gboolean success = g_spawn_async_with_pipes(NULL, exec->m_argv, NULL, (GSpawnFlags) (G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH), NULL, NULL, &exec->m_child_pid, &fd_stdin, &fd_stdout, &fd_stderr, &spawn_error);
  if (success) {
    g_child_watch_add(exec->m_child_pid, (GChildWatchFunc) child_watch, exec); 
  } else {
    if (spawn_error != NULL) {
      std::string err_msg = spawn_error->message;
      std::cerr << "Arg, program failure: '" << err_msg << "'";
      if (exec->m_append_text) {
        exec->m_append_text(err_msg, exec->m_data);
      }
    }
    g_main_loop_quit(exec->m_loop);
    exec->m_exit_status = -1;
    return FALSE;
  }
	exec->m_running = true;
  exec->m_abort = false;
	exec->m_ch_stdout = g_io_channel_unix_new(fd_stdout);
  exec->m_ch_stderr = g_io_channel_unix_new(fd_stderr);
  exec->m_ch_stdin = g_io_channel_unix_new(fd_stdin);
  g_io_channel_set_flags(exec->m_ch_stdout, (GIOFlags) (G_IO_FLAG_NONBLOCK | G_IO_FLAG_IS_READABLE), NULL);
  g_io_channel_set_flags(exec->m_ch_stderr, (GIOFlags) (G_IO_FLAG_NONBLOCK | G_IO_FLAG_IS_READABLE), NULL);
  g_io_channel_set_flags(exec->m_ch_stdin, (GIOFlags) (G_IO_FLAG_NONBLOCK | G_IO_FLAG_IS_WRITEABLE), NULL);
  g_io_channel_set_close_on_unref(exec->m_ch_stdout, TRUE);
  g_io_channel_set_close_on_unref(exec->m_ch_stderr, TRUE);
  g_io_channel_set_close_on_unref(exec->m_ch_stdin, TRUE);
  g_io_add_watch(exec->m_ch_stdout, (GIOCondition) (G_IO_IN | G_IO_HUP | G_IO_PRI), (GIOFunc) channel_read, exec);
  g_io_add_watch(exec->m_ch_stderr, (GIOCondition) (G_IO_IN | G_IO_HUP | G_IO_PRI), (GIOFunc) channel_read, exec);
  return FALSE;
}


gboolean ExecSave::channel_read(GIOChannel *channel, GIOCondition cond, ExecSave* exec)
{
	gsize bytes_read = 0;
  char buf[256];
  
	GIOStatus status = g_io_channel_read_chars(channel, (gchar*) buf, 255, &bytes_read, NULL);
  if (status == G_IO_STATUS_ERROR) 
  {
    return (cond == G_IO_HUP ? FALSE : TRUE);
  }
  buf[bytes_read] = 0;
  std::cout << buf;
  if (exec->m_newline_found && (exec->m_quit_on_newline || exec->m_append_text == NULL)) 
  {
    return (cond == G_IO_HUP ? FALSE : TRUE);
  }
  if (exec->m_quit_on_newline || exec->m_append_text == NULL)
  {
    char *str2 = buf;
    for (gsize i = 0; i < bytes_read; i++)
    {
      char key = *str2;
      if (key == 0x0A || key == 0x0D)
      {
        *str2 = 0;
        exec->m_newline_found = true;
      }
      str2++;
    }
  }
	std::string text = buf;
	*(exec->m_output) += text;
	if (exec->m_append_text)
	{
		exec->m_append_text(text, exec->m_data);
	}
  return (cond == G_IO_HUP ? FALSE : TRUE);
}


void ExecSave::child_watch(GPid pid, gint status, ExecSave *exec)
{
  g_spawn_close_pid(pid); 
  exec->m_exit_status = WEXITSTATUS(status);
  exec->m_running = false;
  g_io_channel_unref(exec->m_ch_stdin);
	g_io_channel_unref(exec->m_ch_stdout);
 	g_io_channel_unref(exec->m_ch_stderr);
  if (exec->m_loop) 
  {
    g_main_loop_quit(exec->m_loop);
    exec->m_loop = NULL;
  }
}


void ExecSave::abort()
{
  m_abort = true;
  sigkill();
  if (m_running && m_loop) 
  {
    g_main_loop_quit(m_loop);
    m_loop = NULL;
  }
}


void ExecSave::sigkill()
{
  if (m_running && m_child_pid != 0)
  {
    kill(m_child_pid, SIGKILL);
  }
}


void ExecSave::key_press_event(const char *utf8_str, int size)
{
  if (m_running)
  {
    std::cout << "Write into pipe: " << utf8_str << std::endl;
    gsize bytes_written = 0;
    GIOStatus status = g_io_channel_write_chars(m_ch_stdin, utf8_str, size, &bytes_written, NULL);
    if (status == G_IO_STATUS_NORMAL) 
    {
      g_io_channel_flush(m_ch_stdin, NULL);
    }
  }
}


bool ExecSave::find_it(const char *exe_name, std::string& location)
{
  std::string which = "which ";
  const char *paths[] = { "/usr/local/sbin/", "/usr/local/bin/", "/usr/sbin/",
                    "/usr/bin/", "/sbin/", "/bin/" };
  int i;
  struct stat info;

  which += exe_name;
  location.erase();
  if (run(which, location, true) == 0) {
    return true;
  }

  for (i = 0; i < 6; i++) {
    location = paths[i];
    location += exe_name;
    if (stat(location.c_str(), &info) == 0) {
      return true;
    }
  }
  return false;
}

ExecSave::ExecSave()
{ 
  m_loop = NULL; 
  m_abort = false; 
  m_exit_status = 0; 
  m_all_done = true; 
  m_running = false; 
  m_child_pid = 0;
}
