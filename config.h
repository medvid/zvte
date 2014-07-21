#define RESIZE_GRIP             FALSE
#define SCROLL_ON_OUTPUT        FALSE
#define SCROLL_ON_KEYSTROKE     FALSE
#define AUDIBLE_BELL            FALSE
#define VISIBLE_BELL            FALSE
#define MOUSE_AUTOHIDE          FALSE
#define ALLOW_BOLD              TRUE
#define SCROLLBACK_LINES        0

#define FONT                    "Monospace 13"

// word characters used for word selection
#define WORD_CHARS              "-A-Za-z0-9,./?%&#:_=+@~"

// CURSOR_BLINK_{SYSTEM,ON,OFF}
#define CURSOR_BLINK             VTE_CURSOR_BLINK_SYSTEM

// CURSOR_SHAPE_{BLOCK,UNDERLINE,IBEAM}
#define CURSOR_SHAPE             VTE_CURSOR_SHAPE_BLOCK

// Colors
#if 0 // gjm colorscheme

#define COLOR_FOREGROUND        "#c5c5c5"
#define COLOR_FOREGROUND_BOLD   "#ffffff"
#define COLOR_FOREGROUND_DIM    "#888888"
#define COLOR_BACKGROUND        "#1c1c1c"
#define COLOR_CURSOR            "#cee318"
#define COLOR_HIGHLIGHT         "#2f2f2f"

static const char *COLORS[16] = {
    "#1c1c1c",
    "#ff005b",
    "#cee318",
    "#ffe755",
    "#048ac7",
    "#833c9f",
    "#0ac1cd",
    "#e5e5e5",
    "#666666",
    "#ff00a0",
    "#ccff00",
    "#ff9f00",
    "#48c6ff",
    "#be67e1",
    "#63e7f0",
    "#f3f3f3",
};

#else // hybrid colorscheme

#define COLOR_FOREGROUND        "#c5c8c6"
#define COLOR_FOREGROUND_BOLD   "#ffffff"
#define COLOR_FOREGROUND_DIM    "#888888"
#define COLOR_BACKGROUND        "#1d1f21"
#define COLOR_CURSOR            "#cee318"
#define COLOR_HIGHLIGHT         "#2f2f2f"

static const char *COLORS[16] = {
    "#282a2e",
    "#ac4142",
    "#90a959",
    "#de935d",
    "#6a9fb5",
    "#aa759f",
    "#75b5aa",
    "#707880",
    "#373b41",
    "#cc6666",
    "#b5bd68",
    "#f0c674",
    "#81a2be",
    "#b294bb",
    "#8abeb7",
    "#c5c8c6",
};

#endif

// Regular expression for URL matching
static const gchar url_regex[] = "(ftp|http)s?://[-a-zA-Z0-9.?$%&/=_~#.,:;+]*";
