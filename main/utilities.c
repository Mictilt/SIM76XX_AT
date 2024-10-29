#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

uint32_t get_time_ms(void) {
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

bool uart_bytes_available(int uart_num) {
    size_t available;
    uart_get_buffered_data_len(uart_num, &available);
    return available > 0;
}

void delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void socket_buffer_put(socket_t* socket, char c) {
    if (!socket) return;
    
    // Store character in circular buffer
    socket->buffer[socket->buffer_head] = c;
    socket->buffer_head = (socket->buffer_head + 1) % SOCKET_BUFFER_SIZE;
    
    // Update buffer size
    if (socket->buffer_size < SOCKET_BUFFER_SIZE) {
        socket->buffer_size++;
    } else {
        // Buffer is full, move tail
        socket->buffer_tail = (socket->buffer_tail + 1) % SOCKET_BUFFER_SIZE;
    }
}