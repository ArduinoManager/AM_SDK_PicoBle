#include "AM_SDK_PicoBle.h"

// #include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"
#include "pico/time.h"

#include "pico/util/datetime.h"
#include "hardware/powman.h"
#include "pico/aon_timer.h"

#include "gap_configuration.h"

// Global instance for forwarding
AMController *global_instance = nullptr;

char characteristic_d[] = "Tx/Rx";
static custom_service_t service_object;

// Flag for general discoverability
#define APP_AD_FLAGS 0x06

static uint8_t adv_data[] = {
    // Flags: general discoverable
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
    // Name: "AM"
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'A', 'M',
    // Custom Service UUID: CC7D10AA-B9DC-4435-8D1F-C6F3781B5D4B
    0x11,                                                             // Length: 1 byte for type + 16 bytes for UUID
    BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS, // Type
    0x4B, 0x5D, 0x1B, 0x78, 0xF3, 0xC6, 0x1F, 0x8D,
    0x35, 0x44, 0xDC, 0xB9, 0xAA, 0x10, 0x7D, 0xCC // UUID in little-endian
};

static const uint8_t adv_data_len = sizeof(adv_data);

void AMController::init(
    void (*doWork)(void),
    void (*doSync)(void),
    void (*processIncomingMessages)(char *variable, char *value),
    void (*processOutgoingMessages)(void),
    void (*deviceConnected)(void),
    void (*deviceDisconnected)(void),
    void (*processAlarms)(char *alarm))
{
    DEBUG_printf("Library Initialization\n");

    this->doWork = doWork;
    this->doSync = doSync;
    this->processIncomingMessages = processIncomingMessages;
    this->processOutgoingMessages = processOutgoingMessages;
    this->deviceConnected = deviceConnected;
    this->deviceDisconnected = deviceDisconnected;
    this->processAlarms = processAlarms;

    is_device_connected = false;
    is_sync_completed = false;

    l2cap_init();
    sm_init();

    // Bluetooth stack setup

    att_server_init(profile_data, NULL, NULL);

    // Instantiate our custom service handler
    custom_service_server_init(characteristic_d_rx);

    // inform about BTstack state
    hci_event_callback_registration.callback = &AMController::static_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for ATT event
    att_server_register_packet_handler(static_packet_handler);

    // turn on bluetooth!
    hci_power_control(HCI_POWER_ON);

    // ----------- Setup Time -----------

    struct tm d;
    time_t epoch = 1733829472; // Tuesday, December 10, 2024 11:32:23 AM
    memcpy(&d, gmtime(&epoch), sizeof(struct tm));
    aon_timer_start_calendar(&d);

    // -----------------------------------
    sleep_ms(100); // Wait a while before configuring the SD Card

    sd_manager = new SDManager(this);

    FATFS fs;
    FRESULT fr = f_mount(&fs, "", 1);
    if (FR_OK == fr)
    {
        DEBUG_printf("SD Card mounted!\n");
        if (processAlarms != NULL)
        {
            // Initialize Alarms
            alarms.init_alarms();
            add_repeating_timer_ms(ALARMS_CHECKS_PERIOD, this->alarm_timer_callback, this, &alarms_checks_timer);
        }
    }
    else
    {
        printf("Error: SD not mounted!\n");
    }

    f_unmount("");

    while (true)
    {
        doWork();

        if (is_device_connected & is_sync_completed)
        {
            processOutgoingMessages();
        }

#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for Wi-Fi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(2000);
#endif
    }
}

void AMController::custom_service_server_init(char *d_ptr)
{

    // Pointer to our service object
    custom_service_t *instance = &service_object;

    // Assign characteristic value

    instance->characteristic_d_value = d_ptr;

    instance->characteristic_d_user_description = characteristic_d;

    instance->characteristic_d_handle = ATT_CHARACTERISTIC_0000FF14_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE;
    instance->characteristic_d_client_configuration_handle = ATT_CHARACTERISTIC_0000FF14_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE;
    instance->characteristic_d_user_description_handle = ATT_CHARACTERISTIC_0000FF14_0000_1000_8000_00805F9B34FB_01_USER_DESCRIPTION_HANDLE;

    global_instance = this; // Set the current instance globally

    // Service start and end handles (modeled off heartrate example)
    service_handler.start_handle = 0;
    service_handler.end_handle = 0xFFFF;
    service_handler.read_callback = &AMController::static_custom_service_read_callback;
    service_handler.write_callback = &AMController::static_custom_service_write_callback;

    // Register the service handler
    att_server_register_service_handler(&service_handler);
}

uint16_t AMController::static_custom_service_read_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    // Forward to the instance-specific method
    if (global_instance)
    {
        return global_instance->custom_service_read_callback(con_handle, attribute_handle, offset, buffer, buffer_size);
    }
    return 0; // Or an appropriate error code
}

uint16_t AMController::custom_service_read_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{

    // Characteristic D
    if (attribute_handle == service_object.characteristic_d_handle)
    {
        return att_read_callback_handle_blob(reinterpret_cast<uint8_t *>(service_object.characteristic_d_value), strlen(service_object.characteristic_d_value), offset, buffer, buffer_size);
    }
    if (attribute_handle == service_object.characteristic_d_client_configuration_handle)
    {
        return att_read_callback_handle_little_endian_16(service_object.characteristic_d_client_configuration, offset, buffer, buffer_size);
    }
    if (attribute_handle == service_object.characteristic_d_user_description_handle)
    {
        return att_read_callback_handle_blob(reinterpret_cast<uint8_t *>(service_object.characteristic_d_user_description), strlen(service_object.characteristic_d_user_description), offset, buffer, buffer_size);
    }

    return 0;
}

int AMController::static_custom_service_write_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    if (global_instance)
    {
        return global_instance->custom_service_write_callback(con_handle, attribute_handle, transaction_mode, offset, buffer, buffer_size);
    }
    return 0; // Or an appropriate error code
}

int AMController::custom_service_write_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    UNUSED(transaction_mode);
    UNUSED(offset);
    UNUSED(buffer_size);

    // Enable/disable notificatins
    if (attribute_handle == service_object.characteristic_d_client_configuration_handle)
    {
        service_object.characteristic_d_client_configuration = little_endian_read_16(buffer, 0);
        service_object.con_handle = con_handle;
    }

    // Write characteristic value
    if (attribute_handle == service_object.characteristic_d_handle)
    {
        custom_service_t *instance = &service_object;
        buffer[buffer_size] = 0;
        memset(service_object.characteristic_d_value, 0, sizeof(service_object.characteristic_d_value));
        memcpy(service_object.characteristic_d_value, buffer, buffer_size);

        // Null-terminate the string
        service_object.characteristic_d_value[buffer_size] = 0;

        DEBUG_printf("R >%s<\n", service_object.characteristic_d_value);
        process_received_buffer(service_object.characteristic_d_value);

        // If client has enabled notifications, register a callback
        if (instance->characteristic_d_client_configuration)
        {
            instance->callback_d.callback = &characteristic_d_callback;
            instance->callback_d.context = (void *)instance;
            att_server_register_can_send_now_callback(&instance->callback_d, instance->con_handle);
        }
        // Alert the application of a bluetooth RX
        // PT_SEM_SAFE_SIGNAL(pt, &BLUETOOTH_READY);
    }

    return 0;
}

void AMController::characteristic_d_callback(void *context)
{
    // Associate the void pointer input with our custom service object
    custom_service_t *instance = (custom_service_t *)context;
    // Send a notification
    att_server_notify(instance->con_handle, instance->characteristic_d_handle, reinterpret_cast<uint8_t *>(instance->characteristic_d_value), strlen(instance->characteristic_d_value));
}

void AMController::static_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (global_instance)
    {
        return global_instance->packet_handler(packet_type, channel, packet, size);
    }
}

void AMController::packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET)
        return;

    // Retrieve event type from HCI packet
    uint8_t event_type = hci_event_packet_get_type(packet);

    // Switch on event type . . .
    switch (event_type)
    {
    // Setup GAP advertisement
    case BTSTACK_EVENT_STATE:
    {
        if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING)
            return;

        gap_local_bd_addr(local_addr);
        printf("\nBTstack up and running on %s.\n", bd_addr_to_str(local_addr));
        // setup advertisements
        uint16_t adv_int_min = 800;
        uint16_t adv_int_max = 800;
        uint8_t adv_type = 0;
        bd_addr_t null_addr;
        memset(null_addr, 0, 6);
        gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
        assert(adv_data_len <= 31); // ble limitation
        gap_advertisements_set_data(adv_data_len, (uint8_t *)adv_data);
        gap_advertisements_enable(1);
        break;
    }

    case ATT_EVENT_CONNECTED:
        DEBUG_printf("ATT_EVENT_CONNECTED\n");
        is_device_connected = true;
        if (deviceConnected != NULL)
        {
            deviceConnected();
        }
        break;

    case ATT_EVENT_DISCONNECTED:
        DEBUG_printf("ATT_EVENT_DISCONNECTED\n");
        is_device_connected = false;
        if (deviceDisconnected != NULL)
        {
            deviceDisconnected();
        }
        break;

        // Disconnected from a client
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        // printf("HCI_EVENT_DISCONNECTION_COMPLETE \n");
        break;

    // Ready to send ATT
    case ATT_EVENT_CAN_SEND_NOW:
        DEBUG_printf("ATT_EVENT_CAN_SEND_NOW\n");
        break;

    default:
        break;
    }
}

void AMController::process_received_buffer(char *buffer)
{
    DEBUG_printf("Buffer >>>%s<<<\n", buffer);

    char *pHash;

    pHash = strtok(buffer, "#");
    while (pHash != NULL)
    {
        DEBUG_printf("\t>%s<\n", pHash);
        char buffer1[strlen(pHash) + 1];
        strcpy(buffer1, pHash);

        char variable[VARIABLELEN];
        char value[VALUELEN];

        char *pEqual = strchr(pHash, '=');
        if (pEqual != NULL)
        {
            strncpy(variable, pHash, MIN(VARIABLELEN, pEqual - pHash));
            variable[pEqual - pHash] = '\0';
            strncpy(value, pEqual + 1, VALUELEN);

            // DEBUG_printf("\t\tvariable %s - value: %s\n", variable, value);

            if (strcmp(value, "Start") > 0 && strcmp(variable, "Sync") == 0)
            {
                // Process sync messages for the variable in value field
                doSync();
                is_sync_completed = true;
            }
            else if (strcmp(variable, "$Time$") == 0)
            {
                struct tm d;
                time_t epoch = atoll(value);
                memcpy(&d, gmtime(&epoch), sizeof(struct tm));
                aon_timer_start_calendar(&d);

                struct tm d1;
                aon_timer_get_time_calendar(&d1);
                DEBUG_printf("%s", asctime(&d1));
            }
            else if (
                (strcmp(variable, "$AlarmId$") == 0 || strcmp(variable, "$AlarmT$") == 0 || strcmp(variable, "$AlarmR$") == 0) &&
                strlen(value) > 0)
            {
                alarms.process_alarm_request(variable, value);
            }
            else if ((strcmp(variable, "SD") == 0 || strcmp(variable, "$SDDL$") == 0) && strlen(value) > 0)
            {
                sd_manager->process_sd_request(variable, value);
            }
            else if (strcmp(variable, "$SDLogData$") == 0 && strlen(value) > 0)
            {
                sd_manager->sd_send_log_data(value);
            }
            else if (strcmp(variable, "$SDLogPurge$") == 0 && strlen(value) > 0)
            {
                sd_manager->process_sd_request(variable, value);
            }
            else
            {
                this->processIncomingMessages(variable, value);
            }
        }

        pHash = strtok(NULL, "#");
    }

    DEBUG_printf("processBuffer completed \n");
}

bool AMController::alarm_timer_callback(__unused struct repeating_timer *t)
{
    AMController *p = (AMController *)t->user_data;

    p->alarms.check_fire_alarms(p->processAlarms);

    return true;
}

void AMController::write_message(const char *variable, int value)
{
    if (strlen(variable) > VARIABLELEN)
    {
        DEBUG_printf("Message Discarded\n");
        return;
    }

    // Pointer to our service object
    custom_service_t *instance = &service_object;

    // Update field value
    sprintf(instance->characteristic_d_value, "%s=%d#", variable, value);

    // Are notifications enabled? If so, register a callback
    if (instance->characteristic_d_client_configuration)
    {
        instance->callback_d.callback = &characteristic_d_callback;
        instance->callback_d.context = (void *)instance;
        att_server_register_can_send_now_callback(&instance->callback_d, instance->con_handle);
    }
}

void AMController::write_message(const char *variable, long value)
{
    if (strlen(variable) > VARIABLELEN)
    {
        DEBUG_printf("Message Discarded\n");
        return;
    }

    // Pointer to our service object
    custom_service_t *instance = &service_object;

    // Update field value
    sprintf(instance->characteristic_d_value, "%s=%ld#", variable, value);

    // Are notifications enabled? If so, register a callback
    if (instance->characteristic_d_client_configuration)
    {
        instance->callback_d.callback = &characteristic_d_callback;
        instance->callback_d.context = (void *)instance;
        att_server_register_can_send_now_callback(&instance->callback_d, instance->con_handle);
    }
}

void AMController::write_message(const char *variable, unsigned long value)
{
    if (strlen(variable) > VARIABLELEN)
    {
        DEBUG_printf("Message Discarded\n");
        return;
    }

    // Pointer to our service object
    custom_service_t *instance = &service_object;

    // Update field value
    sprintf(instance->characteristic_d_value, "%s=%lu#", variable, value);

    // Are notifications enabled? If so, register a callback
    if (instance->characteristic_d_client_configuration)
    {
        instance->callback_d.callback = &characteristic_d_callback;
        instance->callback_d.context = (void *)instance;
        att_server_register_can_send_now_callback(&instance->callback_d, instance->con_handle);
    }
}

void AMController::write_message(const char *variable, float value)
{
    if (strlen(variable) > VARIABLELEN)
    {
        DEBUG_printf("Message Discarded\n");
        return;
    }

    // Pointer to our service object
    custom_service_t *instance = &service_object;

    // Update field value
    sprintf(instance->characteristic_d_value, "%s=%.5f#", variable, value);

    // Are notifications enabled? If so, register a callback
    if (instance->characteristic_d_client_configuration)
    {
        instance->callback_d.callback = &characteristic_d_callback;
        instance->callback_d.context = (void *)instance;
        att_server_register_can_send_now_callback(&instance->callback_d, instance->con_handle);
    }
}

void AMController::write_message(const char *variable, const char *value)
{
    if (strlen(variable) > VARIABLELEN)
    {
        DEBUG_printf("Message Discarded\n");
        return;
    }

    // Pointer to our service object
    custom_service_t *instance = &service_object;

    // Update field value
    sprintf(instance->characteristic_d_value, "%s=%s#", variable, value);

    // Are notifications enabled? If so, register a callback
    if (instance->characteristic_d_client_configuration)
    {
        instance->callback_d.callback = &characteristic_d_callback;
        instance->callback_d.context = (void *)instance;
        att_server_register_can_send_now_callback(&instance->callback_d, instance->con_handle);
    }
}

void AMController::write_message_immediate(const char *variable, const char *value)
{
    if (strlen(variable) > VARIABLELEN)
    {
        DEBUG_printf("Message Discarded\n");
        return;
    }

    // Pointer to our service object
    custom_service_t *instance = &service_object;

    // Update field value
    sprintf(instance->characteristic_d_value, "%s=%s#", variable, value);

    // Are notifications enabled? If so, register a callback
    if (instance->characteristic_d_client_configuration)
    {
        instance->callback_d.callback = &characteristic_d_callback;
        instance->callback_d.context = (void *)instance;
        att_server_register_can_send_now_callback(&instance->callback_d, instance->con_handle);
    }
}

void AMController::write_message_buffer(const char *value, uint size)
{
    // Pointer to our service object
    custom_service_t *instance = &service_object;

    // Update field value
    sprintf(instance->characteristic_d_value, "%s", value);

    // Are notifications enabled? If so, register a callback
    if (instance->characteristic_d_client_configuration)
    {
        instance->callback_d.callback = &characteristic_d_callback;
        instance->callback_d.context = (void *)instance;
        att_server_register_can_send_now_callback(&instance->callback_d, instance->con_handle);
    }
}

void AMController::write_message(const char *variable, float x, float y, float z)
{
    if (strlen(variable) > VARIABLELEN)
    {
        DEBUG_printf("Message Discarded\n");
        return;
    }

    // Pointer to our service object
    custom_service_t *instance = &service_object;

    // Update field value
    sprintf(instance->characteristic_d_value, "%s=%.2f:%.2f:%.2f#", variable, x, y, z);

    // Are notifications enabled? If so, register a callback
    if (instance->characteristic_d_client_configuration)
    {
        instance->callback_d.callback = &characteristic_d_callback;
        instance->callback_d.context = (void *)instance;
        att_server_register_can_send_now_callback(&instance->callback_d, instance->con_handle);
    }
}

unsigned long AMController::now()
{
    time_t now = time(NULL);
    return now;
}

void AMController::log(int msg)
{
    write_message("$D$", msg);
}
void AMController::log(long msg)
{
    write_message("$D$", msg);
}

void AMController::log(unsigned long msg)
{
    write_message("$D$", msg);
}

void AMController::log(float msg)
{
    write_message("$D$", msg);
}

void AMController::log(const char *msg)
{
    write_message("$D$", msg);
}

void AMController::logLn(int msg)
{
    write_message("$DLN$", msg);
}

void AMController::logLn(long msg)
{
    write_message("$DLN$", msg);
}

void AMController::logLn(unsigned long msg)
{
    write_message("$DLN$", msg);
}

void AMController::logLn(float msg)
{
    write_message("$DLN$", msg);
}

void AMController::logLn(const char *msg)
{
    write_message("$DLN$", msg);
}

void AMController::log_labels(const char *variable, const char *label1)
{
    sd_manager->sd_log_labels(variable, label1, NULL, NULL, NULL, NULL);
}

void AMController::log_labels(const char *variable, const char *label1, const char *label2)
{
    sd_manager->sd_log_labels(variable, label1, label2, NULL, NULL, NULL);
}

void AMController::log_labels(const char *variable, const char *label1, const char *label2, const char *label3)
{
    sd_manager->sd_log_labels(variable, label1, label2, label3, NULL, NULL);
}

void AMController::log_labels(const char *variable, const char *label1, const char *label2, const char *label3, const char *label4)
{
    sd_manager->sd_log_labels(variable, label1, label2, label3, label4, NULL);
}

void AMController::log_labels(const char *variable, const char *label1, const char *label2, const char *label3, const char *label4, const char *label5)
{
    sd_manager->sd_log_labels(variable, label1, label2, label3, label4, label5);
}

void AMController::log_value(const char *variable, unsigned long time, float v1)
{
    sd_manager->log_value(variable, time, v1);
}

void AMController::log_value(const char *variable, unsigned long time, float v1, float v2)
{
    sd_manager->log_value(variable, time, v1, v2);
}

void AMController::log_value(const char *variable, unsigned long time, float v1, float v2, float v3)
{
    sd_manager->log_value(variable, time, v1, v2, v3);
}

void AMController::log_value(const char *variable, unsigned long time, float v1, float v2, float v3, float v4)
{
    sd_manager->log_value(variable, time, v1, v2, v3, v4);
}

void AMController::log_value(const char *variable, unsigned long time, float v1, float v2, float v3, float v4, float v5)
{
    sd_manager->log_value(variable, time, v1, v2, v3, v4, v5);
}

unsigned long AMController::log_size(const char *variable)
{
    return sd_manager->sd_log_size(variable);
}

void AMController::log_purge_data(const char *variable)
{
    sd_manager->sd_purge_data(variable);
}

void AMController::gpio_temporary_put(uint pin, bool value, uint ms)
{
    bool previousValue = gpio_get(pin);

    gpio_put(pin, value);
    sleep_ms(ms);
    gpio_put(pin, previousValue);
}