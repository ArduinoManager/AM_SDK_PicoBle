#ifndef AM_PICOBLE_H
#define AM_PICOBLE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "btstack_defines.h"
#include "ble/att_db.h"
#include "ble/att_server.h"
#include "btstack_util.h"
#include "bluetooth_gatt.h"
#include "btstack_debug.h"

#include "AM_Alarms.h"
#include "AM_SDManager.h"

#ifdef DEBUG
#define DEBUG_printf printf
#else
#define DEBUG_printf
#endif

#define VARIABLELEN 14
#define VALUELEN 14

#define BUF_SIZE 2048

// Create a struct for managing this service
typedef struct
{
    // Connection handle for service
    hci_con_handle_t con_handle;

    // Characteristic D information
    char *characteristic_d_value;
    uint16_t characteristic_d_client_configuration;
    char *characteristic_d_user_description;

    // Characteristic D handles
    uint16_t characteristic_d_handle;
    uint16_t characteristic_d_client_configuration_handle;
    uint16_t characteristic_d_user_description_handle;

    btstack_context_callback_registration_t callback_d;

} custom_service_t;

class SDManager;

class AMController
{
private:
    void (*doWork)(void);                                         // Pointer to the function where to put code in place of loop()
    void (*doSync)();                                             // Pointer to the function where widgets are synchronized
    void (*processIncomingMessages)(char *variable, char *value); // Pointer to the function where incoming messages are processed
    void (*processOutgoingMessages)(void);                        // Pointer to the function where outgoing messages are processed
    void (*deviceConnected)(void);                                // Pointer to the function called when a device connects to Arduino
    void (*deviceDisconnected)(void);                             // Pointer to the function called when a device disconnects from Arduino
    void (*processAlarms)(char *alarm);                           // Pointer to the function called when an alarm is fired

    bool is_device_connected;
    bool is_sync_completed;

    char characteristic_d_rx[100];
    btstack_packet_callback_registration_t hci_event_callback_registration;

    SDManager *sd_manager;

    Alarms alarms;
    struct repeating_timer alarms_checks_timer;
    static bool alarm_timer_callback(__unused struct repeating_timer *t);

    //
    char log_file_to_send[128];
    int log_file_read_bytes;

public:
    void init(
        void (*doWork)(void),
        void (*doSync)(void),
        void (*processIncomingMessages)(char *variable, char *value),
        void (*processOutgoingMessages)(void),
        void (*deviceConnected)(void),
        void (*deviceDisconnected)(void),
        void (*processAlarms)(char *alarm));

    void write_message(const char *variable, int value);
    void write_message(const char *variable, long value);
    void write_message(const char *variable, unsigned long value);
    void write_message(const char *variable, float value);
    void write_message(const char *variable, const char *value);
    void write_message_immediate(const char *variable, const char *value);
    void notifiy_message(const char *variable, const char *value);
    void write_message_buffer(const char *value, uint size);
    int can_send_message();

    void write_message(const char *variable, float x, float y, float z);

    unsigned long now();

    void log(int msg);
    void log(long msg);
    void log(unsigned long msg);
    void log(float msg);
    void log(const char *msg);
    void logLn(int msg);
    void logLn(long msg);
    void logLn(unsigned long msg);
    void logLn(float msg);
    void logLn(const char *msg);

    void log_labels(const char *variable, const char *label1);
    void log_labels(const char *variable, const char *label1, const char *label2);
    void log_labels(const char *variable, const char *label1, const char *label2, const char *label3);
    void log_labels(const char *variable, const char *label1, const char *label2, const char *label3, const char *label4);
    void log_labels(const char *variable, const char *label1, const char *label2, const char *label3, const char *label4, const char *label5);

    void log_value(const char *variable, unsigned long time, float v1);
    void log_value(const char *variable, unsigned long time, float v1, float v2);
    void log_value(const char *variable, unsigned long time, float v1, float v2, float v3);
    void log_value(const char *variable, unsigned long time, float v1, float v2, float v3, float v4);
    void log_value(const char *variable, unsigned long time, float v1, float v2, float v3, float v4, float v5);

    unsigned long log_size(const char *variable);
    void log_purge_data(const char *variable);

    void gpio_temporary_put(uint pin, bool value, uint ms);

private:
    att_service_handler_t service_handler;
    void custom_service_server_init(char *d_ptr);

    static void static_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
    void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

    static uint16_t static_custom_service_read_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
    uint16_t custom_service_read_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);

    static int static_custom_service_write_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
    int custom_service_write_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);

    static void characteristic_d_callback(void *context);

    void process_received_buffer(char *buffer);

    int send_log_file(char *name);
};

#endif