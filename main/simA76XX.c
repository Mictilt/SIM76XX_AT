#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "utilities.h"
#include "simA76XX.h"

static char response[256];

// Array to store socket information for multiple connections
socket_t *sockets[MUX_COUNT] = {NULL};

void uart_init()
{
    const uart_config_t uart_config = {
        .baud_rate = MODEM_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, MODEM_TX_PIN, MODEM_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, 2048, 0, 0, NULL, 0));
}

void modem_power_on()
{
    gpio_set_level(BOARD_POWERON_PIN, 1);
    gpio_set_direction(BOARD_PWRKEY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BOARD_PWRKEY_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(BOARD_PWRKEY_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(BOARD_PWRKEY_PIN, 0);
}

void modem_reset()
{
    gpio_set_direction(MODEM_RESET_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
    vTaskDelay(pdMS_TO_TICKS(2600));
    gpio_set_level(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
}

void send_at_command(const char *command)
{
    uart_write_bytes(UART_NUM, command, strlen(command));
    uart_write_bytes(UART_NUM, "\r\n", 2); // Append CRLF
}

void receive_response(char *buffer, int buf_len, int timeout_ms)
{
    int len = uart_read_bytes(UART_NUM, (uint8_t *)buffer, buf_len, pdMS_TO_TICKS(timeout_ms));
    if (len > 0)
    {
        buffer[len] = '\0'; // Null-terminate the response
    }
    else
    {
        buffer[0] = '\0'; // Empty response
    }
}

void sim_unlock_simcom(const char *pin)
{
    char command[32];
    snprintf(command, sizeof(command), "AT+CPIN=\"%s\"", pin);
    send_at_command(command);
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "SIM unlocked successfully");
    }
    else
    {
        ESP_LOGW(TAG, "SIM unlock failed");
    }
}

void check_sim_status()
{

    send_at_command("AT+CPIN?");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "READY") != NULL)
    {
        ESP_LOGI(TAG, "SIM card is ready");
    }
    else if (strstr(response, "SIM PIN") != NULL)
    {
        ESP_LOGI(TAG, "SIM card is locked");
        sim_unlock_simcom("7623");
    }
    else
    {
        ESP_LOGI(TAG, "SIM card status unknown");
    }
}

void check_registration_status()
{
    send_at_command("AT+CREG?");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "0,1") != NULL)
    {
        ESP_LOGI(TAG, "Registered to home network");
    }
    else if (strstr(response, "0,5") != NULL)
    {
        ESP_LOGI(TAG, "Registered to roaming network");
    }
    else
    {
        ESP_LOGI(TAG, "Not registered");
    }
}

void factory_reset()
{
    send_at_command("AT&F");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Factory reset successful");
    }
    else
    {
        ESP_LOGW(TAG, "Factory reset failed");
    }
}

void power_off()
{
    send_at_command("AT+CPOF");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "NORMAL POWER DOWN") != NULL)
    {
        ESP_LOGI(TAG, "Modem powered off");
    }
    else
    {
        ESP_LOGW(TAG, "Failed to power off modem");
    }
}

void sleep_mode()
{
    send_at_command("AT+CSCLK=2");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Modem in sleep mode");
    }
    else
    {
        ESP_LOGW(TAG, "Failed to enter sleep mode");
    }
}

void wake_up()
{
    send_at_command("AT+CSCLK=0");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Modem wake up");
    }
    else
    {
        ESP_LOGW(TAG, "Failed to wake up modem");
    }
}

/*
<fun>
0 minimum functionality
1 full functionality, online mode
4 disable phone both transmit and receive RF circuits
5 Factory Test Mode (The 5 and 1 have the same function)
6 Reset
7 Offline Mode
<rst>
0 do not reset the ME before setting it to <fun> power level
1 reset the ME before setting it to <fun> power level. This value only
takes effect when <fun> equals 1.
*/
void set_phone_functionality(int fun, int rst)
{
    char command[32];
    snprintf(command, sizeof(command), "AT+CFUN=%d,%d", fun, rst);
    send_at_command(command);
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Phone functionality set to %d", fun);
    }
    else
    {
        ESP_LOGW(TAG, "Failed to set phone functionality");
    }
}
/*
<n> 0 disable network registration unsolicited result code.
1 enable network registration unsolicited result code +CREG: <stat>
.
2 enable network registration and location information unsolicited
result code +CREG: <stat>[,
<lac>
<ci>
,
,
<AcT>].
<stat>
0 not registered, ME is not currently searching a new operator to register to.
1 registered, home network.
2 not registered, but ME is currently searching a new operator to register to.
3 registration denied.
4 unknown.
5 registered, roaming.
6 registered for "SMS only", home network (applicable only whenE-UTRAN)
*/

void set_network_mode(int mode)
{
    char command[32];
    snprintf(command, sizeof(command), "AT+CNMP=%d", mode);
    send_at_command(command);
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Network mode set to %d", mode);
    }
    else
    {
        ESP_LOGW(TAG, "Failed to set network mode");
    }
}

void enable_network()
{
    send_at_command("AT+NETOPEN");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Network opened successfully");
    }
    else if (strstr(response, "Network is already opened") != NULL)
    {
        ESP_LOGI(TAG, "Network is already opened");
    }
    else
    {
        ESP_LOGW(TAG, "Network open failed");
    }
}

void disable_network()
{
    send_at_command("AT+NETCLOSE");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Network closed successfully");
    }
    else
    {
        ESP_LOGW(TAG, "Network close failed");
    }
}
/*
bool gprsConnectImpl(const char* apn, const char* user = NULL,
                       const char* pwd = NULL) {
    gprsDisconnect();  // Make sure we're not connected first

    // Define the PDP context

    // The CGDCONT commands set up the "external" PDP context

    // Set the external authentication
    if (user && strlen(user) > 0) {
      sendAT(GF("+CGAUTH=1,0,\""), user, GF("\",\""), pwd, '"');
      waitResponse();
    }

    // Define external PDP context 1
    sendAT(GF("+CGDCONT=1,\"IP\",\""), apn, '"', ",\"0.0.0.0\",0,0");
    waitResponse();

    // Configure TCP parameters

    // Select TCP/IP application mode (command mode)
    sendAT(GF("+CIPMODE=0"));
    waitResponse();

    // Set Sending Mode - send without waiting for peer TCP ACK
    sendAT(GF("+CIPSENDMODE=0"));
    waitResponse();

    // Configure socket parameters
    // AT+CIPCCFG= <NmRetry>, <DelayTm>, <Ack>, <errMode>, <HeaderType>,
    //            <AsyncMode>, <TimeoutVal>
    // NmRetry = number of retransmission to be made for an IP packet
    //         = 10 (default)
    // DelayTm = number of milliseconds to delay before outputting received data
    //          = 0 (default)
    // Ack = sets whether reporting a string "Send ok" = 0 (don't report)
    // errMode = mode of reporting error result code = 0 (numberic values)
    // HeaderType = which data header of receiving data in multi-client mode
    //            = 1 (+RECEIVE,<link num>,<data length>)
    // AsyncMode = sets mode of executing commands
    //           = 0 (synchronous command executing)
    // TimeoutVal = minimum retransmission timeout in milliseconds = 75000
    sendAT(GF("+CIPCCFG=10,0,0,0,1,0,75000"));
    if (waitResponse() != 1) {
      // printf("[%u]Configure socket parameters\n ",__LINE__);
      return false;
    }

    // Configure timeouts for opening and closing sockets
    // AT+CIPTIMEOUT=<netopen_timeout> <cipopen_timeout>, <cipsend_timeout>
    sendAT(GF("+CIPTIMEOUT="), 75000, ',', 15000, ',', 15000);
    waitResponse();

    // PDP context activate or deactivate
    sendAT("+CGACT=1,1");
    if (waitResponse(30000UL) != 1) {
      // printf("[%u]PDP context activate or deactivate\n ",__LINE__);
      return false;
    }

    // This activates and attaches to the external PDP context that is tied
    // to the embedded context for TCP/IP (ie AT+CGACT=1,1 and AT+CGATT=1)
    // Response may be an immediate OK_RESPONSE followed later by "+NETOPEN: 0".
    // We to ignore any immediate response and wait for the
    // URC to show it's really connected.
    sendAT(GF("+NETOPEN"));
    if (waitResponse(75000L, GF(GSM_NL "+NETOPEN: 0")) != 1) {
      // printf("[%u]NETOPEN\n ",__LINE__);
      return false;
    }

    return true;
  }
*/
void gprs_connect(char *apn, char *user, char *pwd)
{
    char command[64];
    disable_network();
    if (user && strlen(user) > 0)
    {
        char auth_command[64];
        snprintf(auth_command, sizeof(auth_command), "AT+CGAUTH=1,0,\"%s\",\"%s\"", user, pwd);
        send_at_command(auth_command);
        receive_response(response, sizeof(response), 1000);
        if (strstr(response, "OK") != NULL)
        {
            ESP_LOGI(TAG, "Authentication set successfully");
        }
        else
        {
            ESP_LOGW(TAG, "Failed to set authentication");
        }
    }

    snprintf(command, sizeof(command), "AT+CGDCONT=1,\"IP\",\"%s\",\",\",0,0", apn);
    send_at_command(command);
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "PDP context defined successfully");
    }
    else
    {
        ESP_LOGW(TAG, "Failed to define PDP context");
    }

    send_at_command("AT+CIPMODE=0");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "TCP mode set successfully");
    }
    else
    {
        ESP_LOGW(TAG, "Failed to set TCP mode");
    }

    send_at_command("AT+CIPSENDMODE=0");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Send mode set successfully");
    }
    else
    {
        ESP_LOGW(TAG, "Failed to set send mode");
    }

    send_at_command("AT+CIPCCFG=10,0,0,0,1,0,75000");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Socket parameters configured successfully");
    }
    else
    {
        ESP_LOGW(TAG, "Failed to configure socket parameters");
    }

    send_at_command("AT+CIPTIMEOUT=75000,15000,15000");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Timeouts configured successfully");
    }
    else
    {
        ESP_LOGW(TAG, "Failed to configure timeouts");
    }

    send_at_command("AT+CGACT=1,1");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "PDP context activated successfully");
    }
    else
    {
        ESP_LOGW(TAG, "Failed to activate PDP context");
    }

    send_at_command("AT+NETOPEN");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Network opened successfully");
    }
    else
    {
        ESP_LOGW(TAG, "Network open failed");
    }
}

void gprs_disconnect()
{
    send_at_command("AT+NETCLOSE");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Network closed successfully");
    }
    else
    {
        ESP_LOGW(TAG, "Network close failed");
    }
}

bool is_gprs_connected()
{
    send_at_command("AT+NETOPEN?");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Network is open");
        return true;
    }
    else
    {
        ESP_LOGW(TAG, "Network is closed");
        return false;
    }
}

void get_sim_info()
{
    send_at_command("AT+CCID");
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "SIM ICCID: %s", response);

    send_at_command("AT+CPBR=1");
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "Phonebook response: %s", response);
}

void call_hangup()
{
    send_at_command("AT+CHUP");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Call hangup successful");
    }
    else
    {
        ESP_LOGW(TAG, "Call hangup failed");
    }
}

void get_network_info()
{
    send_at_command("AT+CSQ"); // Signal quality
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "Signal quality response: %s", response);

    send_at_command("AT+COPS?"); // Operator info
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "Operator info response: %s", response);

    send_at_command("AT+CGPADDR=1"); // IP Address
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "IP Address response: %s", response);
}

void send_sms(const char *number, const char *message)
{
    char command[64];
    snprintf(command, sizeof(command), "AT+CMGS=\"%s\"", number);
    send_at_command(command);
    vTaskDelay(pdMS_TO_TICKS(100));
    send_at_command(message);
    vTaskDelay(pdMS_TO_TICKS(100));
    uart_write_bytes(UART_NUM, "\x1A", 1); // Ctrl+Z to send
}

void enable_gps_impl(int8_t power_en_pin, uint8_t enable_level)
{
    char command[64];

    if (power_en_pin != -1)
    {
        snprintf(command, sizeof(command), "AT+CGDRT=%d,1", power_en_pin);
        send_at_command(command);
        receive_response(response, sizeof(response), 1000);

        snprintf(command, sizeof(command), "AT+CGSETV=%d,%d", power_en_pin, enable_level);
        send_at_command(command);
        receive_response(response, sizeof(response), 1000);
    }

    send_at_command("AT+CGNSSPWR=1");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        printf("GPS enabled successfully\n");
    }
    else
    {
        printf("Failed to enable GPS\n");
    }
}

void disable_gps_impl(int8_t power_en_pin, uint8_t disable_level)
{
    char command[64];

    if (power_en_pin != -1)
    {
        snprintf(command, sizeof(command), "AT+CGSETV=%d,%d", power_en_pin, disable_level);
        send_at_command(command);
        receive_response(response, sizeof(response), 1000);

        snprintf(command, sizeof(command), "AT+CGDRT=%d,0", power_en_pin);
        send_at_command(command);
        receive_response(response, sizeof(response), 1000);
    }

    send_at_command("AT+CGNSSPWR=0");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        printf("GPS disabled successfully\n");
    }
    else
    {
        printf("Failed to disable GPS\n");
    }
}

bool is_enable_gps_impl(void)
{
    char *ptr;
    int status;

    send_at_command("AT+CGNSSPWR?");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL ||
        strstr(response, "+CGNSSPWR:") == NULL)
    {
        return false;
    }

    // Find the first number in response (GNSS_Power_status)
    ptr = strstr(response, "+CGNSSPWR:") + strlen("+CGNSSPWR:");
    status = atoi(ptr);
    return (status == 1);
}

void enable_agps_impl(void)
{
    char *ptr;
    int status;

    send_at_command("AT+CGNSSPWR?");
    receive_response(response, sizeof(response), 30000);
    if ( strstr(response, "OK") != NULL||
        strstr(response, "+CGNSSPWR:") == NULL)
    {
        printf("Failed to check GPS power status\n");
    }

    // Check if GPS is powered on
    ptr = strstr(response, "+CGNSSPWR:") + strlen("+CGNSSPWR:");
    status = atoi(ptr);

    if (status == 1)
    {
        send_at_command("AT+CAGPS");
        receive_response(response, sizeof(response), 30000);
        if ( strstr(response, "OK") != NULL ||
            strstr(response, "+AGPS:") == NULL)
        {
            printf("Failed to enable AGPS\n");
        }

        // Check for "success" in response
        if (strstr(response, "success") != NULL)
        {
            printf("AGPS enabled successfully\n");
        }
        else
        {
            printf("Failed to enable AGPS\n");
        }
    }
}

void get_gps_raw_impl(char *buffer, size_t buffer_size)
{
    char *start_ptr;

    send_at_command("AT+CGNSSINFO");
    if (strstr(response, "+CGNSSINFO:") == NULL)
    {
        buffer[0] = '\0';
        return;
    }

    // Extract the data after "+CGNSSINFO:"
    start_ptr = strstr(response, "+CGNSSINFO:") + strlen("+CGNSSINFO:");
    while (*start_ptr == ' ')
        start_ptr++; // Skip leading spaces

    // Copy the response to the provided buffer
    strncpy(buffer, start_ptr, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';

    // Trim trailing whitespace
    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\r' || buffer[len - 1] == '\n' || buffer[len - 1] == ' '))
    {
        buffer[--len] = '\0';
    }
}

bool get_gps_impl(uint8_t *status, float *lat, float *lon, float *speed, float *alt,
                  int *vsat, int *usat, float *accuracy,
                  int *year, int *month, int *day, int *hour,
                  int *minute, int *second)
{
    char response[512];
    char *ptr;
    uint8_t fix_mode;
    float ilat = 0, ilon = 0, ispeed = 0, ialt = 0, iaccuracy = 0, second_with_ss = 0;
    int ivsat = 0, iusat = 0, iyear = 0, imonth = 0, iday = 0, ihour = 0, imin = 0;
    char north, east;

    send_at_command("AT+CGNSSINFO");
    receive_response(response, sizeof(response), 1000);
    if ( strstr(response, "OK") != NULL ||
        strstr(response, "+CGNSSINFO:") == NULL)
    {
        return false;
    }

    ptr = strstr(response, "+CGNSSINFO:") + strlen("+CGNSSINFO:");
    while (*ptr == ' ')
        ptr++; // Skip leading spaces

    // Parse fix mode
    fix_mode = atoi(ptr);
    if (fix_mode != 1 && fix_mode != 2 && fix_mode != 3)
    {
        receive_response(response, sizeof(response), 1000); // Wait for final OK
        return false;
    }

    // Skip fields until latitude
    for (int i = 0; i < 4; i++)
    {
        ptr = strchr(ptr, ',');
        if (!ptr)
            return false;
        ptr++;
    }

    // Parse all fields
    ilat = strtof(ptr, &ptr);
    ptr = strchr(ptr, ',') + 1;
    north = *ptr;
    ptr = strchr(ptr, ',') + 1;

    ilon = strtof(ptr, &ptr);
    ptr = strchr(ptr, ',') + 1;
    east = *ptr;
    ptr = strchr(ptr, ',') + 1;

    // Parse date (ddmmyy)
    char date_str[7];
    strncpy(date_str, ptr, 6);
    date_str[6] = '\0';
    iday = atoi(date_str) / 10000;
    imonth = (atoi(date_str) / 100) % 100;
    iyear = atoi(date_str) % 100;
    ptr = strchr(ptr, ',') + 1;

    // Parse time (hhmmss.s)
    char time_str[9];
    strncpy(time_str, ptr, 8);
    time_str[8] = '\0';
    ihour = atoi(time_str) / 10000;
    imin = (atoi(time_str) / 100) % 100;
    second_with_ss = strtof(time_str + 4, NULL);
    ptr = strchr(ptr, ',') + 1;

    ialt = strtof(ptr, &ptr);
    ptr = strchr(ptr, ',') + 1;

    ispeed = strtof(ptr, &ptr);
    // Skip course over ground
    ptr = strchr(ptr, ',') + 1;
    // Skip report interval
    ptr = strchr(ptr, ',') + 1;

    iaccuracy = strtof(ptr, &ptr);

    // Assign values to output parameters if they're not NULL
    if (status)
        *status = fix_mode;
    if (lat)
        *lat = ilat * (north == 'N' ? 1 : -1);
    if (lon)
        *lon = ilon * (east == 'E' ? 1 : -1);
    if (speed)
        *speed = ispeed;
    if (alt)
        *alt = ialt;
    if (vsat)
        *vsat = ivsat;
    if (usat)
        *usat = iusat;
    if (accuracy)
        *accuracy = iaccuracy;

    if (iyear < 100)
        iyear += 2000;
    if (year)
        *year = iyear;
    if (month)
        *month = imonth;
    if (day)
        *day = iday;
    if (hour)
        *hour = ihour;
    if (minute)
        *minute = imin;
    if (second)
        *second = (int)second_with_ss;

    receive_response(response, sizeof(response), 1000); // Wait for final OK
    return true;
}

bool set_gps_baud_impl(uint32_t baud)
{
    char command[32];

    snprintf(command, sizeof(command), "AT+CGNSSIPR=%lu", baud);
    send_at_command(command);
    receive_response(response, sizeof(response), 1000);
    if(strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "GPS baud rate set to %lu", baud);
        return true;
    }
    else
    {
        ESP_LOGW(TAG, "Failed to set GPS baud rate");
        return false;
    }
}

bool set_gps_mode_impl(uint8_t mode)
{
    char command[32];

    snprintf(command, sizeof(command), "AT+CGNSSMODE=%u", mode);
    send_at_command(command);
    receive_response(response, sizeof(response), 1000);
    if(strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "GPS mode set to %u", mode);
        return true;
    }
    else
    {
        ESP_LOGW(TAG, "Failed to set GPS mode");
        return false;
    }
}

bool set_gps_output_rate_impl(uint8_t rate_hz)
{
    char command[32];

    snprintf(command, sizeof(command), "AT+CGPSNMEARATE=%u", rate_hz);
    send_at_command(command);
    receive_response(response, sizeof(response), 1000);
    if(strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "NMEA rate set to %u Hz", rate_hz);
        return true;
    }
    else
    {
        ESP_LOGW(TAG, "Failed to set NMEA rate");
        return false;
    }

}

void enable_nmea_impl(void)
{

    send_at_command("AT+CGNSSTST=1");
    receive_response(response, sizeof(response), 1000);

    send_at_command("AT+CGNSSPORTSWITCH=0,1");
    receive_response(response, sizeof(response), 1000);
    if(strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "NMEA enabled successfully");
    }
    else
    {
        ESP_LOGW(TAG, "Failed to enable NMEA");
    }
}

void disable_nmea_impl(void)
{

    send_at_command("AT+CGNSSTST=0");
    receive_response(response, sizeof(response), 1000);

    send_at_command("AT+CGNSSPORTSWITCH=1,0");
    receive_response(response, sizeof(response), 1000);
    if(strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "NMEA disabled successfully");
    }
    else
    {
        ESP_LOGW(TAG, "Failed to disable NMEA");
    }
}

void config_nmea_sentence_impl(bool CGA, bool GLL, bool GSA, bool GSV,
                               bool RMC, bool VTG, bool ZDA, bool ANT)
{
    char command[64];

    snprintf(command, sizeof(command), "AT+CGNSSNMEA=%u,%u,%u,%u,%u,%u,%u,0",
             CGA, GLL, GSA, GSV, RMC, VTG, ZDA);
    send_at_command(command);
    receive_response(response, sizeof(response), 1000);
}

bool modem_connect(const char *host, uint16_t port, uint8_t mux,
                   bool ssl, int timeout_s)
{
    char command[128];
    char response[128];
    uint8_t opened_mux, opened_result;
    char *ptr;
    uint32_t timeout_ms = ((uint32_t)timeout_s) * 1000;

    if (ssl)
    {
        ESP_LOGW(TAG, "SSL not yet supported on this module!");
    }

    // Enable manual data reception mode
    send_at_command("AT+CIPRXGET=1");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") == NULL)
    {
        return false;
    }

    // Create TCP connection
    snprintf(command, sizeof(command), "AT+CIPOPEN=%d,\"TCP\",\"%s\",%d",
             mux, host, port);
    send_at_command(command);

    // Wait for connection response
    receive_response(response, sizeof(response), timeout_ms);
    if (strstr(response, "OK") != NULL ||
        strstr(response, "+CIPOPEN:") == NULL)
    {
        return false;
    }

    // Parse response for mux and result
    ptr = strstr(response, "+CIPOPEN:") + strlen("+CIPOPEN:");
    opened_mux = atoi(ptr);
    ptr = strchr(ptr, ',') + 1;
    opened_result = atoi(ptr);

    return (opened_mux == mux && opened_result == 0);
}

int16_t modem_send(const void *buff, size_t len, uint8_t mux)
{
    char command[64];
    char response[128];
    int16_t sent_bytes = 0;
    char *ptr;

    // Send data length command
    snprintf(command, sizeof(command), "AT+CIPSEND=%d,%d", mux, (uint16_t)len);
    send_at_command(command);
    receive_response(response, sizeof(response), 1000);
    // Wait for prompt
    if (strstr(response, "OK") != NULL ||
        !strstr(response, ">"))
    {
        return 0;
    }

    // Send the actual data
    uart_write_bytes(UART_NUM, buff, len);
    uart_wait_tx_done(UART_NUM, 1000);

    receive_response(response, sizeof(response), 1000);
    // Get confirmation
    if (strstr(response, "OK") != NULL ||
        strstr(response, "+CIPSEND:") == NULL)
    {
        return 0;
    }

    // Parse sent bytes
    ptr = strstr(response, "+CIPSEND:") + strlen("+CIPSEND:");
    ptr = strchr(ptr, ',');     // Skip mux
    ptr = strchr(ptr + 1, ','); // Skip requested bytes
    sent_bytes = atoi(ptr + 1);

    return sent_bytes;
}

size_t modem_read(size_t size, uint8_t mux)
{
    char command[64];
    char response[1500]; // Larger buffer for data
    char *ptr;
    int16_t len_requested, len_confirmed;
    uint32_t start_time;

    if (!sockets[mux])
        return 0;

    snprintf(command, sizeof(command), "AT+CIPRXGET=3,%d,%d", mux, (uint16_t)size);
    send_at_command(command);
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL ||
        strstr(response, "+CIPRXGET:") == NULL)
    {
        return 0;
    }

    // Parse response for data lengths
    ptr = strstr(response, "+CIPRXGET:") + strlen("+CIPRXGET:");
    ptr = strchr(ptr, ',');     // Skip mode
    ptr = strchr(ptr + 1, ','); // Skip mux
    len_requested = atoi(ptr + 1);
    ptr = strchr(ptr + 1, ',');
    len_confirmed = atoi(ptr + 1);

    // Read the actual data
    for (int i = 0; i < len_requested; i++)
    {
        start_time = get_time_ms();
        
        while (!uart_bytes_available(UART_NUM) &&
               (get_time_ms() - start_time < sockets[mux]->_timeout))
        {
            delay_ms(1);
        }
        
        char c;
        uart_read_bytes(UART_NUM, &c, 1, pdMS_TO_TICKS(1000));
        socket_buffer_put(sockets[mux], c);
    }

    sockets[mux]->sock_available = len_confirmed;
    receive_response(response, sizeof(response), 1000); // Wait for OK
    return len_requested;
}
bool modem_get_connected(uint8_t mux)
{
    char response[128];
    char *ptr;
    int mux_state;

    if (!sockets[mux])
        return false;

    send_at_command("AT+CIPCLOSE?");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL ||
        strstr(response, "+CIPCLOSE:") == NULL)
    {
        return false;
    }

    // Parse connection states
    ptr = strstr(response, "+CIPCLOSE:") + strlen("+CIPCLOSE:");
    for (int muxNo = 0; muxNo < MUX_COUNT; muxNo++)
    {
        mux_state = atoi(ptr);
        if (sockets[muxNo])
        {
            sockets[muxNo]->sock_connected = mux_state;
        }
        ptr = strchr(ptr, ',');
        if (ptr)
            ptr++;
    }

    receive_response(response, sizeof(response), 1000); // Wait for OK
    return sockets[mux]->sock_connected;
}

size_t modem_get_available(uint8_t mux)
{
    char command[32];
    char response[64];
    size_t result = 0;
    char *ptr;

    if (!sockets[mux])
        return 0;

    snprintf(command, sizeof(command), "AT+CIPRXGET=4,%d", mux);
    send_at_command(command);
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL &&
        strstr(response, "+CIPRXGET:") != NULL)
    {
        ptr = strstr(response, "+CIPRXGET:") + strlen("+CIPRXGET:");
        ptr = strchr(ptr, ',');     // Skip mode
        ptr = strchr(ptr + 1, ','); // Skip mux
        result = atoi(ptr + 1);
        receive_response(response, sizeof(response), 1000);
    }

    if (!result)
    {
        sockets[mux]->sock_connected = modem_get_connected(mux);
    }
    return result;
}



void enable_debug()
{
    send_at_command("AT+CMEE=2");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Debug mode enabled");
    }
    else
    {
        ESP_LOGW(TAG, "Failed to enable debug mode");
    }
}

void disable_debug()
{
    send_at_command("AT+CMEE=0");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Debug mode disabled");
    }
    else
    {
        ESP_LOGW(TAG, "Failed to disable debug mode");
    }
}

void init_simcom()
{
    send_at_command("AT");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Modem responded at baudrate");
    }
    else
    {
        ESP_LOGW(TAG, "Modem not responding");
    }

    send_at_command("AT+IPR=115200");
    receive_response(response, sizeof(response), 1000);
    if (strstr(response, "OK") != NULL)
    {
        ESP_LOGI(TAG, "Baudrate set to 115200");
    }
    else
    {
        ESP_LOGW(TAG, "Failed to set baudrate");
    }

    send_at_command("AT+IPR?");
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "Current baudrate: %s", response);

    send_at_command("ATI");
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "Module Info: %s", response);

    send_at_command("AT+CGMI");
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "Manufacturer: %s", response);

    send_at_command("AT+CGMM");
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "Model: %s", response);

    send_at_command("AT+CGSN");
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "IMEI: %s", response);

    send_at_command("AT+CGMR");
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "Firmware version response: %s", response);

    send_at_command("AT+CTZR=0");
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "Timezone response: %s", response);

    send_at_command("AT+CTZU=1");
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "Automatic timezone update response: %s", response);

    check_sim_status();
}

