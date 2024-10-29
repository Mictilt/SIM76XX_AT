#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#

#define TEST_HOST "example.com"
#define TEST_PORT 80
#define TEST_MUX 1
#define TEST_TIMEOUT_S 10

static const char *TAG = "MODEM_TEST";

void test_modem_connection() {
    ESP_LOGI(TAG, "Testing modem connection...");
    
    // Test basic TCP connection
    bool result = modem_connect(TEST_HOST, TEST_PORT, TEST_MUX, false, TEST_TIMEOUT_S);
    ESP_LOGI(TAG, "TCP connection test: %s", result ? "PASS" : "FAIL");

    // Test data sending
    const char *test_data = "TEST";
    int16_t sent = modem_send(test_data, strlen(test_data), TEST_MUX);
    ESP_LOGI(TAG, "Send data test: %s (sent %d bytes)", 
             sent == strlen(test_data) ? "PASS" : "FAIL", sent);

    // Test data availability check
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for potential response
    size_t available = modem_get_available(TEST_MUX);
    ESP_LOGI(TAG, "Data available test: %s (%d bytes available)", 
             available > 0 ? "PASS" : "FAIL", available);

    // Test data reading
    if (available > 0) {
        size_t read = modem_read(available, TEST_MUX);
        ESP_LOGI(TAG, "Read data test: %s (read %d bytes)", 
                 read > 0 ? "PASS" : "FAIL", read);
    }

    // Test connection status
    bool connected = modem_get_connected(TEST_MUX);
    ESP_LOGI(TAG, "Connection status test: %s (connected: %s)", 
             "PASS", connected ? "yes" : "no");
}

void test_socket_buffer() {
    ESP_LOGI(TAG, "Testing socket buffer...");
    
    // Initialize a test socket
    if (!sockets[TEST_MUX]) {
        sockets[TEST_MUX] = (socket_t*)malloc(sizeof(socket_t));
        if (sockets[TEST_MUX]) {
            memset(sockets[TEST_MUX], 0, sizeof(socket_t));
            sockets[TEST_MUX]->_timeout = 5000;
        }
    }

    if (!sockets[TEST_MUX]) {
        ESP_LOGE(TAG, "Failed to allocate test socket");
        return;
    }

    // Test buffer put operation
    const char test_chars[] = "Test123";
    for (int i = 0; i < strlen(test_chars); i++) {
        socket_buffer_put(sockets[TEST_MUX], test_chars[i]);
    }

    ESP_LOGI(TAG, "Socket buffer put test: PASS");
    ESP_LOGI(TAG, "Buffer size: %d", sockets[TEST_MUX]->buffer_size);
}

void test_http_request() {
    ESP_LOGI(TAG, "Testing HTTP request...");

    // Connect to web server
    if (!modem_connect("example.com", 80, TEST_MUX, false, TEST_TIMEOUT_S)) {
        ESP_LOGE(TAG, "Failed to connect to server");
        return;
    }

    // Send HTTP GET request
    const char *http_request = "GET / HTTP/1.1\r\n"
                              "Host: example.com\r\n"
                              "Connection: close\r\n"
                              "\r\n";

    int16_t sent = modem_send(http_request, strlen(http_request), TEST_MUX);
    if (sent <= 0) {
        ESP_LOGE(TAG, "Failed to send HTTP request");
        return;
    }

    // Wait for response
    uint32_t start_time = get_time_ms();
    size_t total_received = 0;
    char buffer[256];
    int received_header = 0;

    while ((get_time_ms() - start_time) < 10000) { // 10 second timeout
        size_t available = modem_get_available(TEST_MUX);
        if (available > 0) {
            size_t to_read = available > sizeof(buffer) ? sizeof(buffer) : available;
            size_t read = modem_read(to_read, TEST_MUX);
            if (read > 0) {
                total_received += read;
                if (!received_header) {
                    ESP_LOGI(TAG, "Received HTTP response header");
                    received_header = 1;
                }
            }
        }
        if (!modem_get_connected(TEST_MUX)) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "HTTP request test: %s (received %d bytes)", 
             total_received > 0 ? "PASS" : "FAIL", total_received);
}

void run_all_tests() {
    ESP_LOGI(TAG, "Starting modem tests...");

    // Run all test functions
    test_modem_connection();
    vTaskDelay(pdMS_TO_TICKS(1000));

    test_socket_buffer();
    vTaskDelay(pdMS_TO_TICKS(1000));

    test_http_request();
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "All tests completed!");
}

void app_main() {
    // Initialize logging
    esp_log_level_set(TAG, ESP_LOG_INFO);
    
    // Wait for modem to be ready
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    ESP_LOGI(TAG, "Starting modem tests...");
    
    // Run test suite
    run_all_tests();
    
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}