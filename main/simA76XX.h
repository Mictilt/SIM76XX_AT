// Modem and board pin configurations
#define TAG "MODEM"
#define MODEM_BAUDRATE 115200
#define MODEM_DTR_PIN 25
#define MODEM_TX_PIN 26
#define MODEM_RX_PIN 27
#define BOARD_PWRKEY_PIN 4
#define BOARD_POWERON_PIN 12
#define MODEM_RESET_PIN 5
#define MODEM_RESET_LEVEL 1

#define UART_NUM UART_NUM_1

void uart_init();
void modem_power_on();
void modem_reset();
void send_at_command(const char *command);
void receive_response(char *buffer, int buf_len, int timeout_ms);
void sim_unlock_simcom(const char *pin);
void check_sim_status();
void check_registration_status();
void factory_reset();
void power_off();
void sleep_mode();
void wake_up();
void set_phone_functionality(int fun, int rst);
void set_network_mode(int mode);
void disable_network();
void gprs_connect(char *apn, char *user, char *pass);
void gprs_disconnect();
bool is_gprs_connected();
void get_sim_info();
void call_hangup();
void get_network_info();
void send_sms(const char *number, const char *message);
void enable_gps_impl(int8_t power_en_pin, uint8_t enable_level);
void disable_gps_impl(int8_t power_en_pin, uint8_t disable_level);
bool is_enable_gps_impl(void);
void enable_agps_impl(void);
void get_gps_raw_impl(char *buffer, size_t buffer_size);
bool get_gps_impl(uint8_t *status, float *lat, float *lon, float *speed, float *alt,
                  int *vsat, int *usat, float *accuracy,
                  int *year, int *month, int *day, int *hour,
                  int *minute, int *second);
bool set_gps_baud_impl(uint32_t baud);
bool set_gps_mode_impl(uint8_t mode);
bool set_gps_output_rate_impl(uint8_t rate_hz);
void enable_nmea_impl(void);
void disable_nmea_impl(void);
void config_nmea_sentence_impl(bool CGA, bool GLL, bool GSA, bool GSV,
                               bool RMC, bool VTG, bool ZDA, bool ANT);
bool modem_connect(const char *host, uint16_t port, uint8_t mux, bool ssl, int timeout_s);
int16_t modem_send(const void *buff, size_t len, uint8_t mux);
size_t modem_read(size_t size, uint8_t mux);
bool modem_get_connected(uint8_t mux);
size_t modem_get_available(uint8_t mux);
void enable_debug();
void disable_debug();
void init_simcom();