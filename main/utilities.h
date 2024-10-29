#define SOCKET_BUFFER_SIZE 1024
#define MUX_COUNT 10

typedef struct {
    bool sock_connected;
    size_t sock_available;
    uint32_t _timeout;
    char buffer[SOCKET_BUFFER_SIZE];
    size_t buffer_head;
    size_t buffer_tail;
    size_t buffer_size;
} socket_t;

uint32_t get_time_ms(void);
bool uart_bytes_available(int uart_num);
void delay_ms(uint32_t ms);
void socket_buffer_put(socket_t* socket, char c);