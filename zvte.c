#include <stdlib.h>
#include <sys/wait.h>

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <vte/vte.h>

#include "config.h"

static gboolean button_press_cb(VteTerminal *vte, GdkEventButton *event);
static void bell_cb(GtkWidget *vte);
static gboolean focus_cb(GtkWindow *window);

static void get_vte_padding(VteTerminal *vte, int *left, int *top, int *right, int *bottom);
static char *check_match(VteTerminal *vte, int event_x, int event_y);
static void setup(GtkWindow *window, VteTerminal *vte);

static gboolean button_press_cb(VteTerminal *vte, GdkEventButton *event)
{
    char *match = check_match(vte, (int)event->x, (int)event->y);
    if (!match)
        return FALSE;

    if (event->button == 1) {
        GError *gerror = NULL;
        if (!gtk_show_uri(NULL, match, event->time, &gerror)) {
            g_printerr ("Could not open URL: %s\n", gerror->message);
            g_error_free (gerror);
        }
    } else if(event->button == 3) {
        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clipboard, match, -1);
    }

    g_free(match);
    return TRUE;
}

static void bell_cb(GtkWidget *vte)
{
    gtk_window_set_urgency_hint(GTK_WINDOW(gtk_widget_get_toplevel(vte)), TRUE);
}

static gboolean focus_cb(GtkWindow *window)
{
    gtk_window_set_urgency_hint(window, FALSE);
    return FALSE;
}

static void get_vte_padding(VteTerminal *vte, int *left, int *top, int *right, int *bottom)
{
    GtkBorder border;
    gtk_style_context_get_padding(gtk_widget_get_style_context(GTK_WIDGET(vte)),
                                  gtk_widget_get_state_flags(GTK_WIDGET(vte)),
                                  &border);
    *left = border.left;
    *right = border.right;
    *top = border.top;
    *bottom = border.bottom;
}

static char *check_match(VteTerminal *vte, int event_x, int event_y)
{
    int tag, padding_left, padding_top, padding_right, padding_bottom;
    const long char_width = vte_terminal_get_char_width(vte);
    const long char_height = vte_terminal_get_char_height(vte);

    get_vte_padding(vte, &padding_left, &padding_top, &padding_right, &padding_bottom);
    return vte_terminal_match_check(vte,
                                    (event_x - padding_left) / char_width,
                                    (event_y - padding_top) / char_height,
                                    &tag);
}

static GdkRGBA *rgba_color(const gchar *spec)
{
    static GdkRGBA color = {0, 0, 0, 0};
    if (!gdk_rgba_parse(&color, spec)) {
        g_printerr("invalid color string: %s\n", spec);
    }
    return &color;
}

static void load_theme(GtkWindow *window, VteTerminal *vte)
{
    GdkRGBA palette[256];

    for (unsigned i = 0; i < 256; i++) {
        if (i < 16) {
            palette[i] = *rgba_color(COLORS[i]);
        } else if (i < 232) {
            const unsigned j = i - 16;
            const unsigned r = j / 36, g = (j / 6) % 6, b = j % 6;
            const unsigned red =   (r == 0) ? 0 : r * 40 + 55;
            const unsigned green = (g == 0) ? 0 : g * 40 + 55;
            const unsigned blue =  (b == 0) ? 0 : b * 40 + 55;
            palette[i].red   = (red | red << 8) / 65535.0;
            palette[i].green = (green | green << 8) / 65535.0;
            palette[i].blue  = (blue | blue << 8) / 65535.0;
        } else if (i < 256) {
            const unsigned shade = 8 + (i - 232) * 10;
            palette[i].red = palette[i].green = palette[i].blue = (shade | shade << 8) / 65535.0;
        }
    }

    vte_terminal_set_colors(vte, NULL, NULL, palette, 256);
    vte_terminal_set_color_foreground(vte, rgba_color(COLOR_FOREGROUND));
    vte_terminal_set_color_bold(vte, rgba_color(COLOR_FOREGROUND_BOLD));
    vte_terminal_set_color_background(vte, rgba_color(COLOR_BACKGROUND));
    gtk_widget_override_background_color(GTK_WIDGET(window), GTK_STATE_FLAG_NORMAL, rgba_color(COLOR_BACKGROUND));
    vte_terminal_set_color_cursor(vte, rgba_color(COLOR_CURSOR));
    vte_terminal_set_color_highlight(vte, rgba_color(COLOR_HIGHLIGHT));
}

static void set_font_from_string(VteTerminal *vte)
{
    PangoFontDescription *desc;
    desc = pango_font_description_from_string(FONT);
    vte_terminal_set_font(vte, desc);
    pango_font_description_free(desc);
}

static void setup(GtkWindow *window, VteTerminal *vte)
{
    vte_terminal_set_scroll_on_output(vte, SCROLL_ON_OUTPUT);
    vte_terminal_set_scroll_on_keystroke(vte, SCROLL_ON_KEYSTROKE);
    vte_terminal_set_audible_bell(vte, AUDIBLE_BELL);
    vte_terminal_set_mouse_autohide(vte, MOUSE_AUTOHIDE);
    vte_terminal_set_allow_bold(vte, ALLOW_BOLD);

    int tag = vte_terminal_match_add_gregex(vte,
        g_regex_new(url_regex,
                    G_REGEX_CASELESS,
                    G_REGEX_MATCH_NOTEMPTY,
                    NULL),
        (GRegexMatchFlags)0);
    vte_terminal_match_set_cursor_type(vte, tag, GDK_HAND2);
    set_font_from_string(vte);
    vte_terminal_set_scrollback_lines(vte, SCROLLBACK_LINES);
    vte_terminal_set_cursor_blink_mode(vte, CURSOR_BLINK);
    vte_terminal_set_cursor_shape(vte, CURSOR_SHAPE);
    gtk_window_set_icon_name(window, "terminal");

    load_theme(window, vte);
}

static void exit_with_status(VteTerminal *vte, int status)
{
    gtk_main_quit();
    exit(WIFEXITED(status) ? WEXITSTATUS(status) : EXIT_FAILURE);
}

static void exit_with_success(void)
{
    gtk_main_quit();
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    GError *error = NULL;
    const char *const term = "xterm-256color";
    char *directory = NULL;
    gboolean hold = FALSE;

    GOptionContext *context = g_option_context_new(NULL);
    char *role = NULL, *execute = NULL;
    const GOptionEntry entries[] = {
        {"exec", 'e', 0, G_OPTION_ARG_STRING, &execute, "Command to execute", "COMMAND"},
        {"role", 'r', 0, G_OPTION_ARG_STRING, &role, "The role to use", "ROLE"},
        {"directory", 'd', 0, G_OPTION_ARG_STRING, &directory, "Change to directory", "DIRECTORY"},
        {"hold", 0, 0, G_OPTION_ARG_NONE, &hold, "Remain open after child process exits", NULL},
        {NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL}
    };
    g_option_context_add_main_entries(context, entries, NULL);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));

    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_printerr("option parsing failed: %s\n", error->message);
        return EXIT_FAILURE;
    }

    if (directory) {
        if (chdir(directory) == -1) {
            perror("chdir");
            return EXIT_FAILURE;
        }
        g_free(directory);
    }

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    GtkWidget *panel_overlay = gtk_overlay_new();
    GtkWidget *hint_overlay = gtk_overlay_new();

    GtkWidget *vte_widget = vte_terminal_new();
    VteTerminal *vte = VTE_TERMINAL(vte_widget);

    if (role) {
        gtk_window_set_role(GTK_WINDOW(window), role);
        g_free(role);
    }

    char **command_argv;
    char *default_argv[2] = {NULL, NULL};

    if (execute) {
        int argcp;
        char **argvp;
        g_shell_parse_argv(execute, &argcp, &argvp, &error);
        if (error) {
            g_printerr("failed to parse command: %s\n", error->message);
            return EXIT_FAILURE;
        }
        command_argv = argvp;
    } else {
        default_argv[0] = vte_get_user_shell();
        command_argv = default_argv;
    }

    setup(GTK_WINDOW(window), vte);

    GdkRGBA transparent = {0, 0, 0, 0};

    gtk_widget_override_background_color(hint_overlay, GTK_STATE_FLAG_NORMAL, &transparent);

    gtk_container_add(GTK_CONTAINER(panel_overlay), hint_overlay);
    gtk_container_add(GTK_CONTAINER(hint_overlay), vte_widget);
    gtk_container_add(GTK_CONTAINER(window), panel_overlay);

    if (!hold) {
        g_signal_connect(vte, "child-exited", G_CALLBACK(exit_with_status), NULL);
    }
    g_signal_connect(window, "destroy", G_CALLBACK(exit_with_success), NULL);
    g_signal_connect(vte, "button-press-event", G_CALLBACK(button_press_cb), NULL);
    g_signal_connect(vte, "bell", G_CALLBACK(bell_cb), NULL);

    g_signal_connect(window, "focus-in-event",  G_CALLBACK(focus_cb), NULL);
    g_signal_connect(window, "focus-out-event", G_CALLBACK(focus_cb), NULL);

    g_object_bind_property (G_OBJECT (vte), "window-title",
                            G_OBJECT (window), "title",
                            G_BINDING_DEFAULT);

    gtk_widget_grab_focus(vte_widget);
    gtk_widget_show_all(window);

    GdkWindow *gdk_window = gtk_widget_get_window(window);
    if (!gdk_window) {
        g_printerr("no window\n");
        return EXIT_FAILURE;
    }
    char xid_s[20];
    snprintf(xid_s, sizeof xid_s, "%lu", GDK_WINDOW_XID(gdk_window));
    char **env = g_get_environ();
    env = g_environ_setenv(env, "WINDOWID", xid_s, TRUE);
    env = g_environ_setenv(env, "TERM", term, TRUE);
    env = g_environ_setenv(env, "VTE_VERSION", "3405", TRUE);

    GPid child_pid;
    if (vte_terminal_spawn_sync(vte, VTE_PTY_DEFAULT, NULL, command_argv, env,
            G_SPAWN_SEARCH_PATH, NULL, NULL, &child_pid, NULL, &error)) {
        vte_terminal_watch_child(vte, child_pid);
    } else {
        g_printerr("the command failed to run: %s\n", error->message);
        return EXIT_FAILURE;
    }

    int width, height, padding_left, padding_top, padding_right, padding_bottom;
    const long char_width = vte_terminal_get_char_width(vte);
    const long char_height = vte_terminal_get_char_height(vte);

    gtk_window_get_size(GTK_WINDOW(window), &width, &height);
    get_vte_padding(vte, &padding_left, &padding_top, &padding_right, &padding_bottom);
    vte_terminal_set_size(vte,
                          (width - padding_left - padding_right) / char_width,
                          (height - padding_top - padding_bottom) / char_height);

    g_strfreev(env);

    gtk_main();
    return EXIT_FAILURE; // child process did not cause termination
}
