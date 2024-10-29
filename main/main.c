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

void app_main()
{
    // Initialize UART
    uart_init();

    // Configure GPIO pins
    gpio_set_direction(BOARD_POWERON_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(MODEM_RESET_PIN, GPIO_MODE_OUTPUT);

    // Power on and reset the modem
    modem_power_on();
    modem_reset();

    // Initial delay to allow the modem to start
    vTaskDelay(pdMS_TO_TICKS(5000));
    enable_debug();
    // Check SIM card and registration status
    ESP_LOGI(TAG, "Checking SIM card status...");
    check_sim_status();

    ESP_LOGI(TAG, "Checking network registration status...");
    check_registration_status();

    // Get network information
    ESP_LOGI(TAG, "Getting network information...");
    get_network_info();

    // Query IP address
    send_at_command("AT+CGPADDR=1");
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "IP Address response: %s", response);

    // Query network time
    send_at_command("AT+CCLK?");
    receive_response(response, sizeof(response), 1000);
    ESP_LOGI(TAG, "Network time response: %s", response);
}
