#pragma once

#include <stdio.h>
#include <stdarg.h>

enum LogLevel {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_DEBUG
};

enum LogColor {
    LOG_COLOR_DEFAULT,
    LOG_COLOR_RED,
    LOG_COLOR_GREEN,
    LOG_COLOR_YELLOW,
    LOG_COLOR_BLUE,
    LOG_COLOR_MAGENTA,
    LOG_COLOR_CYAN,
    LOG_COLOR_WHITE
};

void log_set_thread_color(enum LogColor color);
void log_msg(enum LogLevel level, const char* fmt, ...);
