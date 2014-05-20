#include <stdlib.h>

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <vte/vte.h>

#include "config.h"

static void launch_browser(char *url);
static void window_title_cb(VteTerminal *vte);
static gboolean key_press_cb(VteTerminal *vte, GdkEventKey *event);
static gboolean button_press_cb(VteTerminal *vte, GdkEventButton *event);
static void beep_cb(GtkWidget *vte);
static gboolean focus_cb(GtkWindow *window);

static void get_vte_padding(VteTerminal *vte, int *left, int *top, int *right, int *bottom);
static char *check_match(VteTerminal *vte, int event_x, int event_y);
static void setup(GtkWindow *window, VteTerminal *vte);

static void launch_browser(char *url)
{
    char *browser = g_strdup(g_getenv("BROWSER"));;
    char *browser_cmd[3] = {browser, url, NULL};
    GError *error = NULL;

    if (!browser) {
        g_printerr("$BROWSER not set, can't open url\n");
        return;
    }

    g_spawn_async(NULL, browser_cmd, NULL, G_SPAWN_SEARCH_PATH,
                  NULL, NULL, NULL, &error);
    if (error) {
        g_printerr("error launching '%s': %s\n", browser, error->message);
        g_error_free(error);
    }
    g_free(browser);
}

static void window_title_cb(VteTerminal *vte)
{
    const char *const title = vte_terminal_get_window_title(vte);
    gtk_window_set_title(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(vte))),
                         title ? title : "zvte");
}

static gboolean key_press_cb(VteTerminal *vte, GdkEventKey *event)
{
    const char *text = NULL;
    const guint modifiers = event->state & gtk_accelerator_get_default_mod_mask();
    const guint keyval = gdk_keyval_to_lower(event->keyval);

    if (modifiers == (GDK_CONTROL_MASK|GDK_SHIFT_MASK)) {
        switch (keyval) {
            case GDK_KEY_c:
                vte_terminal_copy_clipboard(vte);
                return TRUE;
            case GDK_KEY_v:
                vte_terminal_paste_clipboard(vte);
                return TRUE;
        }
    } else if (modifiers == GDK_CONTROL_MASK) {
        switch (keyval) {
            /* alt+num -> alt+num */
            case GDK_KEY_1:
                text = "\0331"; break;
            case GDK_KEY_2:
                text = "\0332"; break;
            case GDK_KEY_3:
                text = "\0333"; break;
            case GDK_KEY_4:
                text = "\0334"; break;
            case GDK_KEY_5:
                text = "\0335"; break;
            case GDK_KEY_6:
                text = "\0336"; break;
            case GDK_KEY_7:
                text = "\0337"; break;
            case GDK_KEY_8:
                text = "\0338"; break;
            case GDK_KEY_9:
                text = "\0339"; break;
            case GDK_KEY_0:
                text = "\0330"; break;
        }
    } else if (modifiers == GDK_SHIFT_MASK) {
        switch (keyval) {
            case GDK_KEY_F1:
                text = "\033[23~"; break;
            case GDK_KEY_F2:
                text = "\033[24~"; break;
            case GDK_KEY_F3:
                text = "\033[25~"; break;
            case GDK_KEY_F4:
                text = "\033[26~"; break;
            case GDK_KEY_F5:
                text = "\033[28~"; break;
            case GDK_KEY_F6:
                text = "\033[29~"; break;
            case GDK_KEY_F7:
                text = "\033[31~"; break;
            case GDK_KEY_F8:
                text = "\033[32~"; break;
            case GDK_KEY_F9:
                text = "\033[33~"; break;
            case GDK_KEY_F10:
                text = "\033[34~"; break;
        }
    } else if (modifiers == GDK_MOD1_MASK) {
        switch (keyval) {
            case GDK_KEY_c:
                vte_terminal_copy_clipboard(vte);
                return TRUE;
            case GDK_KEY_v:
                vte_terminal_paste_clipboard(vte);
                return TRUE;
            case GDK_KEY_1:
                text = "\033[1;3P"; break;
            case GDK_KEY_2:
                text = "\033[1;3Q"; break;
            case GDK_KEY_3:
                text = "\033[1;3R"; break;
            case GDK_KEY_4:
                text = "\033[1;3S"; break;
            case GDK_KEY_5:
                text = "\033[15;3~"; break;
            case GDK_KEY_6:
                text = "\033[17;3~"; break;
            case GDK_KEY_7:
                text = "\033[18;3~"; break;
            case GDK_KEY_8:
                text = "\033[19;3~"; break;
            case GDK_KEY_9:
                text = "\033[20;3~"; break;
            case GDK_KEY_0:
                text = "\033[21;3~"; break;
        }
    }
    if (text) {
        vte_terminal_feed_child(vte, text, -1);
        return TRUE;
    }
    return FALSE;
}

static gboolean button_press_cb(VteTerminal *vte, GdkEventButton *event)
{
    char *match = check_match(vte, (int)event->x, (int)event->y);
    if (!match)
        return FALSE;

    if (event->button == 1) {
        launch_browser(match);
    } else if(event->button == 3) {
        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clipboard, match, -1);
    }

    g_free(match);
    return TRUE;
}

static void beep_cb(GtkWidget *vte)
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
    GtkBorder *border = NULL;
    gtk_widget_style_get(GTK_WIDGET(vte), "inner-border", &border, NULL);
    if (!border) {
        g_warning("VTE's inner-border property unavailable");
        *left = *top = *right = *bottom = 0;
    } else {
        *left = border->left;
        *right = border->right;
        *top = border->top;
        *bottom = border->bottom;
        gtk_border_free(border);
    }
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

    vte_terminal_set_colors_rgba(vte, NULL, NULL, palette, 256);
    vte_terminal_set_color_foreground_rgba(vte, rgba_color(COLOR_FOREGROUND));
    vte_terminal_set_color_bold_rgba(vte, rgba_color(COLOR_FOREGROUND_BOLD));
    vte_terminal_set_color_dim_rgba(vte, rgba_color(COLOR_FOREGROUND_DIM));
    vte_terminal_set_color_background_rgba(vte, rgba_color(COLOR_BACKGROUND));
    gtk_widget_override_background_color(GTK_WIDGET(window), GTK_STATE_FLAG_NORMAL, rgba_color(COLOR_BACKGROUND));
    vte_terminal_set_color_cursor_rgba(vte, rgba_color(COLOR_CURSOR));
    vte_terminal_set_color_highlight_rgba(vte, rgba_color(COLOR_HIGHLIGHT));
}

static void setup(GtkWindow *window, VteTerminal *vte)
{
    gtk_window_set_has_resize_grip(window, RESIZE_GRIP);
    vte_terminal_set_scroll_on_output(vte, SCROLL_ON_OUTPUT);
    vte_terminal_set_scroll_on_keystroke(vte, SCROLL_ON_KEYSTROKE);
    vte_terminal_set_audible_bell(vte, AUDIBLE_BELL);
    vte_terminal_set_visible_bell(vte, VISIBLE_BELL);
    vte_terminal_set_mouse_autohide(vte, MOUSE_AUTOHIDE);
    vte_terminal_set_allow_bold(vte, ALLOW_BOLD);

    int tag = vte_terminal_match_add_gregex(vte,
        g_regex_new(url_regex,
                    G_REGEX_CASELESS,
                    G_REGEX_MATCH_NOTEMPTY,
                    NULL),
        (GRegexMatchFlags)0);
    vte_terminal_match_set_cursor_type(vte, tag, GDK_HAND2);
    vte_terminal_set_font_from_string(vte, FONT);
    vte_terminal_set_word_chars(vte, WORD_CHARS);
    vte_terminal_set_scrollback_lines(vte, SCROLLBACK_LINES);
    vte_terminal_set_cursor_blink_mode(vte, CURSOR_BLINK);
    vte_terminal_set_cursor_shape(vte, CURSOR_SHAPE);
    gtk_window_set_icon_name(window, ICON_NAME);

    load_theme(window, vte);
}

static void exit_with_status(VteTerminal *vte)
{
    int status = vte_terminal_get_child_exit_status(vte);
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
    char *title = NULL;
    const GOptionEntry entries[] = {
        {"exec", 'e', 0, G_OPTION_ARG_STRING, &execute, "Command to execute", "COMMAND"},
        {"role", 'r', 0, G_OPTION_ARG_STRING, &role, "The role to use", "ROLE"},
        {"title", 't', 0, G_OPTION_ARG_STRING, &title, "Window title", "TITLE"},
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

    VtePty *pty = vte_terminal_pty_new(vte, VTE_PTY_DEFAULT, &error);

    if (!pty) {
        g_printerr("failed to create pty: %s\n", error->message);
        return EXIT_FAILURE;
    }

    setup(GTK_WINDOW(window), vte);

    vte_terminal_set_pty_object(vte, pty);
    vte_pty_set_term(pty, term);

    GdkRGBA transparent = {0, 0, 0, 0};

    gtk_widget_override_background_color(hint_overlay, GTK_STATE_FLAG_NORMAL, &transparent);

    gtk_container_add(GTK_CONTAINER(panel_overlay), hint_overlay);
    gtk_container_add(GTK_CONTAINER(hint_overlay), vte_widget);
    gtk_container_add(GTK_CONTAINER(window), panel_overlay);

    if (!hold) {
        g_signal_connect(vte, "child-exited", G_CALLBACK(exit_with_status), NULL);
    }
    g_signal_connect(window, "destroy", G_CALLBACK(exit_with_success), NULL);
    g_signal_connect(vte, "key-press-event", G_CALLBACK(key_press_cb), NULL);
    g_signal_connect(vte, "button-press-event", G_CALLBACK(button_press_cb), NULL);
    g_signal_connect(vte, "beep", G_CALLBACK(beep_cb), NULL);

    g_signal_connect(window, "focus-in-event",  G_CALLBACK(focus_cb), NULL);
    g_signal_connect(window, "focus-out-event", G_CALLBACK(focus_cb), NULL);

    if (title) {
        gtk_window_set_title(GTK_WINDOW(window), title);
        g_free(title);
    } else {
        g_signal_connect(vte, "window-title-changed", G_CALLBACK(window_title_cb), NULL);
        window_title_cb(vte);
    }

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

    GPid ppid;
    if (g_spawn_async(NULL, command_argv, env,
                      (GSpawnFlags)(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH),
                      (GSpawnChildSetupFunc)vte_pty_child_setup, pty,
                      &ppid, &error)) {
        vte_terminal_watch_child(vte, ppid);
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
