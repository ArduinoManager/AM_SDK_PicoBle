#include "AM_SDManager.h"

#include "AM_SDK_PicoBle.h"

#include "f_util.h"
#include "ff.h"

#ifdef DEBUG_SD
#define SD_DEBUG_printf printf
#else
#define SD_DEBUG_printf
#endif

SDManager::SDManager(AMController *pico)
{
    this->pico = pico;
}

bool SDManager::endsWith(const char *base, const char *str)
{
    int blen = strlen(base);
    int slen = strlen(str);
    if (slen <= blen)
    {
        return (0 == strcmp(base + blen - slen, str));
    }
    return false;
}

int SDManager::dir(char *last_file_sent)
{
    FATFS fs;
    FRESULT fr;
    DIR dir;
    FILINFO fno;
    FRESULT res;

    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("SD not mounted - error: %s (%d)\n", FRESULT_str(fr), fr);
        return 0;
    }

    SD_DEBUG_printf("SD mounted\n");

    fr = f_findfirst(&dir, &fno, "/", "*");
    if (FR_OK != fr)
    {
        SD_DEBUG_printf("f_open(%s) error: %s (%d)\n", "/", FRESULT_str(fr), fr);
        return 0;
    }

    if (strlen(last_file_sent) > 0)
    {
        while (fr == FR_OK && fno.fname[0])
        {
            if (fno.fname[0] != '.' && endsWith(fno.fname, ".txt"))
            {
                if (strcmp(fno.fname, last_file_sent) == 0)
                {
                    break;
                }
                SD_DEBUG_printf("\tAlready Sent %s\n", fno.fname);
            }
            fr = f_findnext(&dir, &fno); /* Search for next item */
        }
    }

    while (fr == FR_OK && fno.fname[0])
    {
        if (fno.fname[0] != '.' && endsWith(fno.fname, ".txt"))
        {
            SD_DEBUG_printf("Dir - Sending %s\n", fno.fname);
            if (!pico->can_send_message())
            {
                SD_DEBUG_printf("File cannot be sent [Last Sent: %s]\n", last_file_sent);
                return -1;
            }
            strcpy(last_file_sent, fno.fname);
            pico->write_message_immediate("SD", fno.fname);
        }
        fr = f_findnext(&dir, &fno); /* Search for next item */
    }

    if (!pico->can_send_message()) {
        return -1;
    }

    SD_DEBUG_printf("Dir - Sending end of list\n");
    pico->notifiy_message("SD", "$EFL$");

    f_closedir(&dir);
    f_unmount("");

    return 0;
}

int SDManager::transmit_file(char *filename,  int *already_read_bytes)
{
    FATFS fs;
    FRESULT fr;
    DIR dir;
    FILINFO fno;
    FRESULT res;
    FIL fil;
    char buffer[64];

    SD_DEBUG_printf("Sending file %s\n", filename);

    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Device not mounted - error: %s (%d)\n", FRESULT_str(fr), fr);
        return 0;
    }

    fr = f_open(&fil, filename, FA_OPEN_EXISTING | FA_READ);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Error opening file %s - error: %s (%d)\n", filename, FRESULT_str(fr), fr);
        return 0;
    }

    if (*already_read_bytes == 0) {
        pico->write_message_immediate("SD", "$C$");
    }
    else {
        f_lseek(&fil, *already_read_bytes);
    }

    //int chunk = 0;
    while (!f_eof(&fil))
    {
        uint size;
        fr = f_read(&fil, &buffer, sizeof(buffer), &size);
        if (fr != FR_OK)
        {
            f_close(&fil);
            f_unmount("");

            return 0;
        }
        SD_DEBUG_printf("\tBuffer %s\n", buffer);

        if (!pico->can_send_message())
        {
            DEBUG_printf("File %s not yet completed\n", filename);
            f_close(&fil);
            f_unmount("");
            return -1;
        }

        pico->notify_buffer(buffer, size);
        *already_read_bytes += size;
    }
    pico->write_message_immediate("SD", "$E$");

    SD_DEBUG_printf("\nFile %s completed\n", filename);

    f_close(&fil);
    f_unmount("");

    return 0;
}

// Is this used somewhere?
bool SDManager::append(char *filename, uint8_t *byte, unsigned int size)
{
    FRESULT fr;
    FIL fil;

    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (fr != FR_OK)
    {
        return false;
    }

    unsigned int written_bytes;

    fr = f_write(&fil, byte, size, &written_bytes);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("f_write error: %s (%d)\n", FRESULT_str(fr), fr);
        return false;
    }

    f_close(&fil);

    return (written_bytes == size);
}

void SDManager::sd_log_labels(const char *variable, const char *label1, const char *label2, const char *label3, const char *label4, const char *label5)
{
    FATFS fs;
    FRESULT fr;
    DIR dir;
    FILINFO fno;
    FRESULT res;
    FIL fil;

    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Device not mounted - error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }

    char filename[64];
    strcpy(filename, "/");
    strcat(filename, variable);
    strcat(filename, ".txt");

    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Error opening file %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }

    if (f_size(&fil) > 0)
    {
        SD_DEBUG_printf("No Labels required for %s\n", filename);
        f_close(&fil);
        return;
    }

    f_printf(&fil, "-;%s;", label1);

    if (label2 != NULL)
    {
        f_printf(&fil, "%s;", label2);
    }
    else
    {
        f_printf(&fil, "-;");
    }

    if (label3 != NULL)
    {
        f_printf(&fil, "%s;", label3);
    }
    else
    {
        f_printf(&fil, "-;");
    }

    if (label4 != NULL)
    {
        f_printf(&fil, "%s;", label4);
    }
    else
    {
        f_printf(&fil, "-;");
    }

    if (label5 != NULL)
    {
        f_printf(&fil, "%s\n", label5);
    }
    else
    {
        f_printf(&fil, "-\n");
    }

    f_close(&fil);

    f_unmount("");
}

void SDManager::log_value(const char *variable, unsigned long time, float v1)
{
    log_values(variable, time, &v1, NULL, NULL, NULL, NULL);
}

void SDManager::log_value(const char *variable, unsigned long time, float v1, float v2)
{
    log_values(variable, time, &v1, &v2, NULL, NULL, NULL);
}

void SDManager::log_value(const char *variable, unsigned long time, float v1, float v2, float v3)
{
    log_values(variable, time, &v1, &v2, &v3, NULL, NULL);
}

void SDManager::log_value(const char *variable, unsigned long time, float v1, float v2, float v3, float v4)
{
    log_values(variable, time, &v1, &v2, &v3, &v4, NULL);
}

void SDManager::log_value(const char *variable, unsigned long time, float v1, float v2, float v3, float v4, float v5)
{
    log_values(variable, time, &v1, &v2, &v3, &v4, &v5);
}

void SDManager::log_values(const char *variable, unsigned long time, float *v1, float *v2, float *v3, float *v4, float *v5)
{
    FATFS fs;
    FRESULT fr;
    DIR dir;
    FILINFO fno;
    FRESULT res;
    FIL fil;

    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Device not mounted - error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }

    char filename[64];
    strcpy(filename, "/");
    strcat(filename, variable);
    strcat(filename, ".txt");

    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Error opening file %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }

    f_printf(&fil, "%lu;%f;", time, *v1);

    if (v2 != NULL)
    {
        f_printf(&fil, "%f;", *v2);
    }
    else
    {
        f_printf(&fil, "-;");
    }

    if (v3 != NULL)
    {
        f_printf(&fil, "%f;", *v3);
    }
    else
    {
        f_printf(&fil, "-;");
    }

    if (v4 != NULL)
    {
        f_printf(&fil, "%f;", *v4);
    }
    else
    {
        f_printf(&fil, "-;");
    }

    if (v5 != NULL)
    {
        f_printf(&fil, "%f\n", *v5);
    }
    else
    {
        f_printf(&fil, "-\n");
    }

    f_close(&fil);

    f_unmount("");
}

FSIZE_t SDManager::sd_log_size(const char *variable)
{
    FATFS fs;
    FRESULT fr;
    DIR dir;
    FILINFO fno;
    FRESULT res;
    FIL fil;

    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Device not mounted - error: %s (%d)\n", FRESULT_str(fr), fr);
        return 0;
    }

    char filename[64];
    strcpy(filename, "/");
    strcat(filename, variable);
    strcat(filename, ".txt");

    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Error opening file %s (%d)\n", FRESULT_str(fr), fr);
        return 0;
    }
    FSIZE_t size = f_size(&fil);

    f_close(&fil);
    f_unmount("");

    return size;
}

void SDManager::sd_purge_data(const char *variable)
{
    FATFS fs;
    FRESULT fr;
    DIR dir;
    FILINFO fno;
    FRESULT res;
    FIL fil;

    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Device not mounted - error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }

    char filename[64];
    strcpy(filename, "/");
    strcat(filename, variable);
    strcat(filename, ".txt");

    fr = f_unlink(filename);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Error deleting: %s - %s (%d)\n", filename, FRESULT_str(fr), fr);
    }

    f_unmount("");
}

void SDManager::sd_purge_data_keeping_labels(const char *variable)
{
    FATFS fs;
    FRESULT fr;
    DIR dir;
    FILINFO fno;
    FRESULT res;
    FIL fil;

    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Device not mounted - error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }

    char filename[64];
    strcpy(filename, "/");
    strcat(filename, variable);
    strcat(filename, ".txt");

    fr = f_open(&fil, filename, FA_OPEN_EXISTING | FA_READ | FA_WRITE);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Error opening file : %s (%d)\n", FRESULT_str(fr), fr);
        f_unmount("");
        return;
    }

    char line[128];

    // Reads the first line which contains the labels
    f_gets(line, 128, &fil);
    SD_DEBUG_printf("%s\n", line);

    // Truncate the file at current position
    fr = f_truncate(&fil);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Error truncating file : %s (%d)\n", FRESULT_str(fr), fr);
        f_close(&fil);
        f_unmount("");
        return;
    }

    f_close(&fil);
    f_unmount("");
}

int SDManager::sd_send_log_data(const char *value, int *already_read_bytes)
{
    DEBUG_printf("Sending Logging file for variable: %s\n", value);

    FATFS fs;
    FRESULT fr;
    DIR dir;
    FILINFO fno;
    FRESULT res;
    FIL fil;

    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Device not mounted - error: %s (%d)\n", FRESULT_str(fr), fr);
        pico->write_message_immediate(value, "");
        return 0;
    }

    char filename[64];
    strcpy(filename, "/");
    strcat(filename, value);
    strcat(filename, ".txt");

    SD_DEBUG_printf("Sending File %s\n", filename);

    fr = f_open(&fil, filename, FA_OPEN_EXISTING | FA_READ);
    if (fr != FR_OK)
    {
        SD_DEBUG_printf("Error opening file : %s (%d)\n", FRESULT_str(fr), fr);
        pico->write_message_immediate(value, "");
        f_unmount("");
        return 0;
    }

    if (*already_read_bytes > 0)
    {
        f_lseek(&fil, *already_read_bytes);
    }

    char line[128];
    while (!f_eof(&fil))
    {
        f_gets(line, 128, &fil);
        *already_read_bytes += strlen(line);
        DEBUG_printf("%s\n", line);

        if (!pico->can_send_message())
        {
            DEBUG_printf("File %s not yet completed\n", filename);
            f_close(&fil);
            f_unmount("");
            return -1;
        }

        pico->notifiy_message(value, line);
    }

    pico->write_message_immediate(value, "");
    f_close(&fil);
    f_unmount("");

    SD_DEBUG_printf("File %s sent\n", filename);

    return 0;
}