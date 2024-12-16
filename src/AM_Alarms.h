#ifndef AM_ALARM_H
#define AM_ALARM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"

#include "pico/time.h"
#include "f_util.h"
#include "ff.h"

#define ALARM_ID_SIZE 12
#define MAX_ALARMS 5
#define ALARMS_CHECKS_PERIOD 10000 // ms

typedef struct AM_Alarm
{
    char id[ALARM_ID_SIZE];
    unsigned long time;
    bool repeat;
} AM_Alarm;

class Alarms
{

public:
    void init_alarms();
    void process_alarm_request(char *variable, char *value);
    void check_fire_alarms(void (*processAlarms)(char *));
};

#endif