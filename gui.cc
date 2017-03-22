/*
To manually compile this (those are backticks not quotes):
  g++ -Wall -g gui.cc -o resetmsmice-gui `pkg-config gtk+-2.0 --cflags --libs`
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
#include <gtk/gtk.h>
#include <sstream>
#include <iostream>
#include <libusb-1.0/libusb.h>

static const char * about_text = 
    "This package fixes scroll wheel issues with certain Wireless Microsoft mice in X.org (includes KDE & Gnome applications), where the vertical wheel scrolls abnormally fast. Only needed if you dual boot between Microsoft Windows and some linux distribution.\n\n"
    "Known to fix the vertical scroll wheel issue with the following models (and others related):\n"
    "    Microsoft Wireless Mouse 1000\n"
    "    Microsoft Wireless Optical Desktop 3000\n"
    "    Microsoft Wireless Mobile Mouse 3500\n"
    "    Microsoft Wireless Mobile Mouse 4000\n"
    "    Microsoft Comfort Mouse 4500\n"
    "    Microsoft Wireless Mouse 5000\n\n"
    "This program basically just resets a setting in the mouse through usb communications and then exits.";

class ExecSave {
protected:
    int m_pipefd[2];
    pid_t m_pid;
    void (*m_append_text)(const std::string&, gpointer);
    std::string m_output;
    bool m_quit_on_newline;
    int m_exit_status, m_wait_status;
    gpointer m_data;
    GThread *m_thread;
    GMutex m_output_mutex;
    bool m_output_updated, m_abort, m_all_done, m_read_thread_done;
public:
    ExecSave() { m_pid = 0; m_pipefd[0] = 0; m_pipefd[1] = 0; m_exit_status = 0; m_output_updated = false; m_all_done = true; m_read_thread_done = true; }
    signed char run(std::string& exe, std::string& output, bool quit_on_newline, void (*)(const std::string&, gpointer) = NULL, gpointer data = NULL);
    bool find_it(const char *exe_name, std::string& location);
    bool is_running() { return (m_all_done == true ? false : true); }
    bool is_done() { return m_all_done; }
    static gpointer read_thread(gpointer);
    static gboolean append_text_timer(gpointer);
};


class ResetWindow {
protected:
    GtkBuilder *m_builder;
    GtkWidget *m_window;
    GtkLabel *m_status_label;
    GtkTextBuffer *m_buffer;
    bool m_resized;
    ExecSave exec_save;
public:
    ResetWindow();
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
    static void destroy(GtkObject*, gpointer);
    static gboolean delete_event(GtkWidget*, gpointer);
    static void append_terminal_text_callback(const std::string&, gpointer);
};


signed char ExecSave::run(std::string& exe, std::string& output, bool quit_on_newline, void (*append_text)(const std::string&, gpointer), gpointer data)
{
    std::vector<std::string> str_args;
    std::vector<const char*> ptr_args;
    std::string root_cmd;
    std::string::size_type start, end, size;
    int retval;
    signed char retval8;
    int count = 0;
   
    if (m_all_done == false) {
        return -1;
    }
    m_all_done = false;
    m_output = output;
    m_wait_status = 0;
    m_exit_status = 0;
    m_append_text = append_text;
    m_quit_on_newline = quit_on_newline;
    m_data = data;
    
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
        ptr_args.push_back(str_args[count].c_str());
        start += size + 1;
    }
    ptr_args.push_back(NULL);
 
    std::cout << "Executing: ";
    char* const* ptr_args2 = (char* const*) &ptr_args.front();
    for (int i = 0; ptr_args2[i] != NULL; i++) {
        std::cout << ptr_args2[i] << " ";
    }
    std::cout << std::endl;
    
    if (pipe(m_pipefd) == -1) {
        m_output += strerror(errno);
        if (append_text) append_text(m_output, data);
        return -1;
    }

    m_pid = fork();
    if (m_pid == 0) {
        // in child
        close(m_pipefd[0]);
        dup2(m_pipefd[1], STDOUT_FILENO);
        dup2(m_pipefd[1], STDERR_FILENO);
        close(m_pipefd[1]); // in child
        retval = execvp(root_cmd.c_str(), (char* const*) &ptr_args.front());
        if (retval == -1) perror("Arg!");
        _exit(retval);
    } else if (m_pid == -1) {
        m_output += strerror(errno);
        std::cerr << m_output;
        if (append_text) append_text(m_output, data);
        return -1;
    }
    // Now in parent
    close(m_pipefd[1]); // close the write end of the pipe in the parent

    g_mutex_init(&m_output_mutex);
    m_abort = false;
    if ((m_thread = g_thread_new("read_thread", read_thread, this)) == NULL) {
        m_output += "Fatal Error: failed to create thread!";
        std::cerr << m_output;
        if (append_text) append_text(m_output, data);
        m_wait_status = waitpid(m_pid, &m_exit_status, 0);
        close(m_pipefd[0]);
    } else {
        if (append_text) {
            g_timeout_add(200, append_text_timer, this);
        } 
        gtk_main();
        if (m_read_thread_done == false) {
            m_abort = true;
            g_thread_join(m_thread);
            g_mutex_clear(&m_output_mutex);
            _exit(-1);
        }
    }
    g_mutex_clear(&m_output_mutex);
    if (m_output_updated && append_text) append_text(m_output, data);

    if (m_wait_status != -1 && WIFEXITED(m_exit_status)) { // child terminated normally
        retval8 = WEXITSTATUS(m_exit_status);
    } else {
        m_output += strerror(errno);
        if (append_text) append_text(m_output, data);
        retval8 = -1;
    }
    output = m_output;
    std::cerr << "Execution complete, return value: " << static_cast<int>(retval8) << std::endl;
    m_all_done = true;
    return retval8;
} 


gpointer ExecSave::read_thread(gpointer data)
{
    ExecSave *exec = static_cast<ExecSave*>(data);
    char key = 0;

    exec->m_read_thread_done = false;
    
    while (read(exec->m_pipefd[0], &key, 1) > 0 && exec->m_abort == false && (exec->m_quit_on_newline == false || (key != 0x0D && key != 0x0A))) {
        std::cout << key;
        g_mutex_lock(&exec->m_output_mutex);
        exec->m_output += key;
        exec->m_output_updated = true;
        g_mutex_unlock(&exec->m_output_mutex);
    }
    if (exec->m_quit_on_newline && (key == 0x0D || key == 0x0A)) {
        std::cout << std::endl;
    }
    exec->m_wait_status = waitpid(exec->m_pid, &exec->m_exit_status, 0);
    close(exec->m_pipefd[0]);
    exec->m_read_thread_done = true;
    gtk_main_quit();
    return NULL;
}


gboolean ExecSave::append_text_timer(gpointer data)
{
    std::string output;
    ExecSave* exec = static_cast<ExecSave*>(data);
    if (exec == NULL || exec->m_all_done == true || exec->m_append_text == NULL) return FALSE;

    if (exec->m_read_thread_done == false) {
        g_mutex_lock(&exec->m_output_mutex);
    }
    if (exec->m_output_updated) {
        output = exec->m_output.c_str();
        exec->m_append_text(output, exec->m_data);
        exec->m_output.clear();
        exec->m_output_updated = false;
    }
    if (exec->m_read_thread_done == false) {
        g_mutex_unlock(&exec->m_output_mutex);
    }
    return (exec->m_read_thread_done == true ? FALSE : TRUE);
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


ResetWindow::ResetWindow()
{ 
    m_resized = false; 
    m_buffer = NULL;
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
    GtkWidget *text_view, *vbox1, *scroll_window;
    GtkTextIter start, end;
    gint width, height;

    if (m_resized && m_buffer) { 
        return true; 
    } else if (m_resized) {
        return false;
    }

    m_resized = true;

    gtk_window_get_size( GTK_WINDOW(m_window), &width, &height);
    gtk_window_resize( GTK_WINDOW(m_window), width, height+220);
    text_view = GTK_WIDGET( gtk_text_view_new());       
    gtk_text_view_set_editable( GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode( GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    gtk_text_view_set_cursor_visible( GTK_TEXT_VIEW(text_view), TRUE);
    gtk_widget_set_size_request(text_view, 600, 220);
    scroll_window = GTK_WIDGET( gtk_scrolled_window_new(NULL, NULL) );
    vbox1 = GTK_WIDGET( gtk_builder_get_object(m_builder, "vbox1"));
    if (vbox1 == NULL) {
        std::cerr << "Cannot find vbox1 in window!" << std::endl;
        return false;
    }
    gtk_container_add(GTK_CONTAINER(scroll_window), text_view);
    gtk_widget_set_size_request(scroll_window, 620, 220);
    gtk_box_pack_start( GTK_BOX(vbox1), scroll_window, TRUE, TRUE, 0);
    gtk_widget_show(scroll_window);
    gtk_widget_show(text_view);
    m_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    if (m_buffer) {
        /* Tag with font monospace and tag name "font". */
        gtk_text_buffer_create_tag(m_buffer, "monospace", "family", "monospace", NULL);
        gtk_text_buffer_get_iter_at_offset(m_buffer, &start, 0);
        gtk_text_buffer_get_iter_at_offset(m_buffer, &end, -1);
        gtk_text_buffer_apply_tag_by_name(m_buffer, "monospace", &start, &end);
        return true;
    } else {
        std::cerr << "Fatal Error: No terminal text view buffer! resetmsmice-gui will not work!" << std::endl;
        return false;
    }
}

void ResetWindow::clear_terminal()
{
    if (m_buffer) {
        gtk_text_buffer_set_text(m_buffer, "", -1);
    }
}

void ResetWindow::set_terminal_text(const std::string& text)
{
    if (m_buffer) {
        GtkTextIter end;
        gtk_text_buffer_set_text(m_buffer, "", -1);
        gtk_text_buffer_get_iter_at_offset(m_buffer, &end, -1);
        gtk_text_buffer_insert_with_tags_by_name(m_buffer, &end, text.c_str(), -1, "monospace", NULL);
    }
}

void ResetWindow::append_terminal_text(const std::string& text)
{
    if (m_buffer) {
        GtkTextIter end;
        gtk_text_buffer_get_iter_at_offset(m_buffer, &end, -1);
        gtk_text_buffer_insert_with_tags_by_name(m_buffer, &end, text.c_str(), -1, "monospace", NULL);
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
    std::string pkexec_cmd, enable_boot_path, output;
    std::stringstream err_msg;
    char self_location[256];

    create_terminal_view();
    clear_terminal();

    if (exec_save.find_it("pkexec", pkexec_cmd) == false) { 
        err_msg << "Error: Cannot find pkexec binary in PATH. Please make sure policy kit is installed." << std::endl;
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

    pkexec_cmd += " ";
    pkexec_cmd += enable_boot_path;
    if (enable == true) {
        pkexec_cmd += " --enable --quiet";
    } else {
        pkexec_cmd += " --disable";
    }
    if (exec_save.run(pkexec_cmd, output, false, &ResetWindow::append_terminal_text_callback, this) == 0) {
        append_terminal_text("Done.");
        get_status_text();
    } else {
        err_msg << std::endl << "Error running: " << pkexec_cmd << std::endl;
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
    if (resetwindow->exec_save.is_running() == false) {
        resetwindow->reset_now();
    }
}


void ResetWindow::reset_now()
{
    char self_location[256];
    std::string resetmsmice_cmd, output;
    std::stringstream err_msg;
    std::string pkexec_cmd;

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

    if (exec_save.find_it("pkexec", pkexec_cmd) == false) {
        err_msg << "Error: Cannot find pkexec binary in PATH. Please make sure policy kit is installed." << std::endl;
        std::cerr << err_msg.str();
        append_terminal_text(err_msg.str());
        return;
    }

    pkexec_cmd += " ";
    pkexec_cmd += resetmsmice_cmd;

    exec_save.run(pkexec_cmd, output, false, &ResetWindow::append_terminal_text_callback, this);
}


GtkWidget* ResetWindow::create()
{
    char self_location[256];
    struct stat info;
    const char *ui_locations[] = { "/usr/share/resetmsmice/resetmsmice.ui", "/usr/local/share/resetmsmice/resetmsmice.ui" };
    int ui_location_index = 0;
    bool found = false;
    GtkWidget *enable_button, *disable_button, *reset_now_button;
    GtkLabel *about_label;
    GError *error = NULL;
    
    if (readlink("/proc/self/exe", self_location, 255) > 0) {
        struct stat info;
        if (strstr(self_location, "/local/") == NULL) {
            ui_location_index = 0;
        } else {
            ui_location_index = 1;
        }
        if (stat(ui_locations[ui_location_index], &info) == 0) {
            found = true;
        }
    }
 
    if (found == false && stat(ui_locations[0], &info)==0) {
        found = true;
    }
    if (found == false && stat(ui_locations[1], &info)==0) {
        found = true;
        ui_location_index = 1;
    }
    if (found == false) {
        std::cerr << "resetmsmice-gui fatal error: cannot find resetmsmice.ui in /usr/share/resetmsmice or /usr/local/share/resetmsmice" << std::endl;
        return NULL;
    }

    m_builder = gtk_builder_new();
    if (gtk_builder_add_from_file(m_builder, ui_locations[ui_location_index], &error)==0) {
        g_warning("%s", error->message);
        return NULL;
    }
    m_window = GTK_WIDGET( gtk_builder_get_object(m_builder, "window1") );
    if (m_window == NULL) {
        std::cerr << "failed to build dialog." << std::endl;
        return NULL;
    }
    enable_button = GTK_WIDGET( gtk_builder_get_object(m_builder, "enable_button") );
    disable_button = GTK_WIDGET( gtk_builder_get_object(m_builder, "disable_button") );
    reset_now_button = GTK_WIDGET( gtk_builder_get_object(m_builder, "reset_now_button") );
    m_status_label = GTK_LABEL( gtk_builder_get_object(m_builder, "status_text") );
    about_label = GTK_LABEL( gtk_builder_get_object(m_builder, "about_text") );
    if (about_label == NULL) {
        std::cerr << "Cannot find about label.";
    } else {
        gtk_label_set_text(about_label, about_text);
    }
    if (m_status_label == NULL) std::cerr << "Cannot find status label element.";
    if (reset_now_button == NULL) std::cerr << "Cannot find reset now button.";

    gtk_window_set_title(GTK_WINDOW(m_window), "Reset Microsoft Mice");

    gtk_window_set_resizable(GTK_WINDOW(m_window), TRUE);

    gtk_builder_connect_signals(m_builder, NULL);

    g_signal_connect(G_OBJECT(m_window), "delete-event",
                     G_CALLBACK(&ResetWindow::delete_event), NULL);

    g_signal_connect(G_OBJECT(m_window), "destroy", 
                     G_CALLBACK(&ResetWindow::destroy), NULL);
                 
    g_signal_connect(G_OBJECT(enable_button), "clicked",
                     G_CALLBACK(&ResetWindow::enable_clicked), this);
           
    g_signal_connect(G_OBJECT(disable_button), "clicked",
                     G_CALLBACK(&ResetWindow::disable_clicked), this);
       
    g_signal_connect(G_OBJECT(reset_now_button), "clicked",
                     G_CALLBACK(&ResetWindow::reset_now_clicked), this);
       
    gtk_widget_show(m_window);
                   
    get_status_text();

    return m_window;
}


void ResetWindow::destroy(GtkObject *object, gpointer user_data)
{
    gtk_main_quit();
}


gboolean ResetWindow::delete_event(GtkWidget*, gpointer)
{
    return FALSE;
}


ResetWindow::~ResetWindow() 
{
    g_object_unref(G_OBJECT(m_builder));
}


int main(int argc, char **argv)
{
    GtkWidget *pDialog;
    ResetWindow resetwindow;

    gtk_init(&argc, &argv);

    pDialog = resetwindow.create();
    if (pDialog) {
        gtk_main();
        return 0;
    }
    return 1;
}

