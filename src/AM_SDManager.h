#ifndef AM_SDMANAGER_H
#define AM_SDMANAGER_H

#include "AM_SDK_PicoBle.h"

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "ff.h"

class AMController;

class SDManager
{
public:
    SDManager(AMController *p);

    void process_sd_request(char *variable, char *value);

    // void deleteFile(char *filename);
    bool append(char *filename, uint8_t *byte, unsigned int size);

    void sd_log_labels(const char *variable, const char *label1, const char *label2, const char *label3, const char *label4, const char *label5);
    void log_value(const char *variable, unsigned long time, float v1);
    void log_value(const char *variable, unsigned long time, float v1, float v2);
    void log_value(const char *variable, unsigned long time, float v1, float v2, float v3);
    void log_value(const char *variable, unsigned long time, float v1, float v2, float v3, float v4);
    void log_value(const char *variable, unsigned long time, float v1, float v2, float v3, float v4, float v5);
    FSIZE_t sd_log_size(const char *variable);
    void sd_purge_data(const char *variable);
    void sd_purge_data_keeping_labels(const char *variable);
    int sd_send_log_data(const char *value, int *log_file_read_bytes);

    int dir(char *last_file_sent);
    
private:
    AMController *pico;

    bool endsWith(const char *base, const char *str); 
    bool transmit_file(char *filename);

    void log_values(const char *variable, unsigned long time, float *v1, float *v2, float *v3, float *v4, float *v5);
};

#endif