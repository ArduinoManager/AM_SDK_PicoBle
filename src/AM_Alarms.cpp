#include "AM_Alarms.h"
#include <time.h>

#include "AM_SDK_PicoBle.h"

#ifdef DUMP_ALARMS
#define DUMPALARMS_printf printf
#else
#define DUMPALARMS_printf
#endif

const char *const filename = "alarms.txt";

AM_Alarm current_alarm;
AM_Alarm alarms[MAX_ALARMS];
int last_alarm_idx;

static int find_alarm(char *alarmId);
static void save_alarms();
static void delete_alarm_by_id(char *id);
static void delete_alarm_by_idx(int idx);
static void dumpAlarms();

void Alarms::init_alarms()
{
    last_alarm_idx = 0;

    // Load current alarms from file

    FRESULT fr;
    FIL fil;

    fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK)
    {
        DEBUG_printf("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
        return;
    }

    while (!f_eof(&fil))
    {
        AM_Alarm alarm;

        fr = f_read(&fil, &alarm, sizeof(AM_Alarm), NULL);
        if (fr != FR_OK)
        {
            DEBUG_printf("f_read error: %s (%d)\n", FRESULT_str(fr), fr);
        }
        else
        {
            alarms[last_alarm_idx] = alarm;
            last_alarm_idx++;
        }
    }

    fr = f_close(&fil);
    if (fr != FR_OK)
    {
        DEBUG_printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }

    dumpAlarms();
}

void Alarms::process_alarm_request(char *variable, char *value)
{
    DEBUG_printf("\t** processAlarm **\n");
    DEBUG_printf("\t\t** Variable %s - Value %s\n", variable, value);

    if (strcmp(variable, "$AlarmId$") == 0)
    {
        strcpy(current_alarm.id, value);
    }
    if (strcmp(variable, "$AlarmT$") == 0)
    {
        current_alarm.time = atol(value);
    }
    if (strcmp(variable, "$AlarmR$") == 0)
    {
        current_alarm.repeat = atoi(value);
        if (current_alarm.time != 0)
        {
            DEBUG_printf("\t\t\t Create-Update Alarm\n");
            int idx = find_alarm(current_alarm.id);
            if (idx == -1)
            {
                alarms[last_alarm_idx] = current_alarm;
                last_alarm_idx += 1;
            }
            else
            {
                alarms[idx] = current_alarm;
            }

            save_alarms();
        }
        else
        {
            DEBUG_printf("\t\t\t Delete Alarm\n");
            delete_alarm_by_id(current_alarm.id);
            save_alarms();
        }

        dumpAlarms();
    }
    DEBUG_printf("\t** processAlarm completed **\n");
}

void Alarms::check_fire_alarms(void (*fireAlarm)(char *))
{
    time_t now = time(NULL);

    DUMPALARMS_printf("**** Check Alarms *****\n");

    char buff[20];
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S GMT []", localtime(&now));

    DUMPALARMS_printf("Now time: %s",buff);
    DUMPALARMS_printf(" [%ju]\n", now);

    dumpAlarms();

    for (int i = 0; i < last_alarm_idx; i++)
    {
        if (now >= alarms[i].time)
        {
            fireAlarm(alarms[i].id);

            if (alarms[i].repeat)
            {
                alarms[i].time += 86400; // Scheduled again tomorrow
            }
            else
            {
                delete_alarm_by_idx(i);
            }

            save_alarms();
        }
    }
}

static void delete_alarm_by_id(char *id)
{
    int idx = find_alarm(id);
    if (idx != -1)
    {
        delete_alarm_by_idx(idx);
    }
}

static void delete_alarm_by_idx(int idx)
{
    for (int i = idx; i <= last_alarm_idx - 1; i++)
    {
        alarms[i] = alarms[i + 1];
    }
    last_alarm_idx -= 1;
}

static int find_alarm(char *alarmId)
{
    for (int i = 0; i < MAX_ALARMS; i++)
    {
        if (strcmp(alarms[i].id, alarmId) == 0)
        {
            return i;
        }
    }

    return -1;
}

static void save_alarms()
{
    FATFS fs;
    FIL fil;

    FRESULT fr = f_mount(&fs, "", 1);
    if (fr != FR_OK)
    {
        DEBUG_printf("Device not mounted - error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }

    fr = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
    {
        DEBUG_printf("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    }

    for (int i = 0; i < last_alarm_idx; i++)
    {
        //     if (f_DEBUG_printf(&fil, "%s,%ld,%d\n", "cazzo", 0, 0) < 0) {
        //         DEBUG_printf("f_DEBUG_printf failed\n");
        //     }

        UINT bytes;

        fr = f_write(&fil, &(alarms[i]), sizeof(AM_Alarm), &bytes);
        if (fr != FR_OK)
        {
            DEBUG_printf("f_write error: %s (%d)\n", FRESULT_str(fr), fr);
        }
    }

    fr = f_close(&fil);
    if (FR_OK != fr)
    {
        DEBUG_printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }

    f_unmount("");
}

static void dumpAlarms()
{
    DUMPALARMS_printf("--- Alarms ----\n");

    for (int i = 0; i < last_alarm_idx; i++)
    {
        char buff[20];
        time_t alarm_time = alarms[i].time;
        strftime(buff, 20, "%Y-%m-%d %H:%M:%S GMT []", localtime(&alarm_time));
        DUMPALARMS_printf("%d: %s - %s [%lu] repeat: %s\n", i, alarms[i].id, buff, alarms[i].time, alarms[i].repeat ? "yes" : "no");
    }

    DUMPALARMS_printf("---------------\n");
}