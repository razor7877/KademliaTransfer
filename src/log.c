#include <pthread.h>
#include <time.h>

#include "log.h"

static const char* LEVEL_COLORS[] = {
    [LOG_INFO]  = "\x1b[32m",
    [LOG_WARN]  = "\x1b[33m",
    [LOG_ERROR] = "\x1b[31m",
    [LOG_DEBUG] = "\x1b[36m"
};

static const char* THREAD_COLORS[] = {
    [LOG_COLOR_DEFAULT] = "\x1b[0m",
    [LOG_COLOR_RED]     = "\x1b[31m",
    [LOG_COLOR_GREEN]   = "\x1b[32m",
    [LOG_COLOR_YELLOW]  = "\x1b[33m",
    [LOG_COLOR_BLUE]    = "\x1b[34m",
    [LOG_COLOR_MAGENTA] = "\x1b[35m",
    [LOG_COLOR_CYAN]    = "\x1b[36m",
    [LOG_COLOR_WHITE]   = "\x1b[37m"
};

/**
 * @brief Thread-local variable for thread-specific log colors
 * 
 */
static __thread const char* thread_color = "\x1b[0m";

void log_set_thread_color(enum LogColor color) {
    if (color >= LOG_COLOR_DEFAULT && color <= LOG_COLOR_WHITE)
        thread_color = THREAD_COLORS[color];
    else
        thread_color = THREAD_COLORS[LOG_COLOR_DEFAULT];
}

void log_msg(enum LogLevel level, const char* fmt, ...) {
    const char* reset = "\x1b[0m";

    // Time prefix
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char timebuf[20];
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", t);

    const char* level_str;
    switch (level) {
        case LOG_INFO:  level_str = "INFO";  break;
        case LOG_WARN:  level_str = "WARN";  break;
        case LOG_ERROR: level_str = "ERROR"; break;
        case LOG_DEBUG: level_str = "DEBUG"; break;
        default:        level_str = "LOG";   break;
    }

    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "%s[%s]%s %s[%s]%s ",
        LEVEL_COLORS[level], level_str, reset,
        thread_color, timebuf, reset);

    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    va_end(args);
}
