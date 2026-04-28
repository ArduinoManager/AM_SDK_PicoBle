/*

   AMController libraries, example sketches (“The Software”) and the related documentation (“The Documentation”) are supplied to you
   by the Author in consideration of your agreement to the following terms, and your use or installation of The Software and the use of The Documentation
   constitutes acceptance of these terms.
   If you do not agree with these terms, please do not use or install The Software.
   The Author grants you a personal, non-exclusive license, under author's copyrights in this original software, to use The Software.
   Except as expressly stated in this notice, no other rights or licenses, express or implied, are granted by the Author, including but not limited to any
   patent rights that may be infringed by your derivative works or by other works in which The Software may be incorporated.
   The Software and the Documentation are provided by the Author on an "AS IS" basis.  THE AUTHOR MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT
   LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE SOFTWARE OR ITS USE AND OPERATION
   ALONE OR IN COMBINATION WITH YOUR PRODUCTS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE,
   REPRODUCTION AND MODIFICATION OF THE SOFTWARE AND OR OF THE DOCUMENTATION, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
   STRICT LIABILITY OR OTHERWISE, EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   Author: Fabrizio Boco - fabboco@gmail.com

   Version: 1.1

   All rights reserved

*/
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

    // Characteristic information
    char *characteristic_d_value;
    uint16_t characteristic_d_client_configuration;
    char *characteristic_d_user_description;

    // Characteristic handles
    uint16_t characteristic_d_handle;
    uint16_t characteristic_d_client_configuration_handle;
    uint16_t characteristic_d_user_description_handle;

    btstack_context_callback_registration_t callback_d;

    char buffer[64];

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

    char file_to_send[128]; // Name of the log file to send
    int already_read_bytes;    // Log file bytes already sent

    bool send_log_file;     // Sending Log File
    bool send_dir;          // Sending SD file list
    bool send_file_content; // Sending file content

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
    void notify_buffer(const char *value, uint size);
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
    float to_voltage(uint16_t adc_value, float vref);
    uint16_t avg_adc_read(uint8_t samples);

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
};

#endif