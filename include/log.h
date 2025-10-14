#pragma once

#include <stdio.h>
#include <stdarg.h>

/**
 * @file log.h
 * @brief Helper functions for elegant message logging to the terminal
 * 
 * 
 */

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

/**
 * @brief Changes the log timestamp color for the calling thread only
 * 
 * @param color The new color to use
 */
void log_set_thread_color(enum LogColor color);

/**
 * @brief Logs a new message
 * 
 * @param level The log message level
 * @param fmt The log format (printf formatting)
 * @param ... Arguments for the format string
 */
void log_msg(enum LogLevel level, const char* fmt, ...);
