#pragma once
/**
 * @file schedule.h
 * @brief This file defines structures for schedule task
 *
 */
#include <time.h>

typedef void(ScheduledFunc)(void);

struct Schedule {
  const char *name;
  time_t next_run;
  int interval_secs;
  ScheduledFunc *func;
};