#pragma once

#include <time.h>

/**
 * @file schedule.h
 * @brief This file defines structures for schedule task
 *
 */

typedef void(ScheduledFunc)(void);

struct Schedule {
  const char *name;
  time_t next_run;
  int interval_secs;
  ScheduledFunc *func;
};