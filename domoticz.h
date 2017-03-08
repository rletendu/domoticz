#ifndef domoticz_h
#define domoticz_h

#include "Arduino.h"

#define DOMOTICZ_WIFI           1
#define DOMOTICZ_ETHERNET       2

#include "../config_domoticz.h"

#ifndef WIFI_TIMEOUT_MAX
#define WIFI_TIMEOUT_MAX      50
#endif

#ifndef DOMO_BUFF_MAX
#define DOMO_BUFF_MAX       1700
#endif

#ifndef DOMOTICZ_VBAT_MIN
#define DOMOTICZ_VBAT_MIN   2000
#endif

#ifndef DOMOTICZ_VBAT_MAX
#define DOMOTICZ_VBAT_MAX   3600
#endif

// Define where debug output will be printed.
#ifndef DEBUG_DOMO_PRINTER
#define DEBUG_DOMO_PRINTER Serial
#endif

// Setup debug printing macros.
#ifdef DOMOTICZ_DEBUG
#define DEBUG_DOMO_PRINT(...) { DEBUG_DOMO_PRINTER.print(__VA_ARGS__); }
#define DEBUG_DOMO_PRINTLN(...) { DEBUG_DOMO_PRINTER.println(__VA_ARGS__); }
#else
#define DEBUG_DOMO_PRINT(...) {}
#define DEBUG_DOMO_PRINTLN(...) {}
#endif


class Domoticz
{
  public:
    Domoticz();
    bool begin(void);
#if DOMOTICZ_INTERFACE==DOMOTICZ_WIFI
    bool begin(char *ssid, char *passw,char *server,char *port, char *domo_user, char *domo_passwd);
#elif DOMOTICZ_INTERFACE==DOMOTICZ_ETHERNET
    bool begin(char *server,char *port, char *domo_user, char *domo_passwd);
#endif
    bool stop(void);

    bool get_variable(int idx, char* var);

    bool get_servertime(char *servertime);
    bool get_sunrise(char *sunrise);
    bool get_sunset(char *sunset);
    bool send_log_message(char *message);

    bool update_temperature(int idx, const char* temp);
    bool update_temperature(int idx, float temp);
    bool get_temperature(int idx, float *temp, char *name);

    bool update_luminosity(int idx, const char* lux);
    bool update_luminosity(int idx, int lux);
    bool get_luminosity(int idx, int *lum, char* name);

    bool udpate_temp_hum(int idx, const char* temp, const char* hum);
    bool udpate_temp_hum(int idx, float temp, float hum);
    bool get_temp_hum(int idx, float *temp, uint8_t *hum, char *name);

    bool udpate_temp_hum_baro(int idx, const char* temp, const char* hum, const char* baro);
    bool get_temp_hum_baro(int idx, float *temp, uint8_t *hum, uint16_t *baro, char *name);

    bool update_voltage(int idx, const char* voltage);
    bool update_voltage(int idx, float voltage);
    bool get_voltage(int idx, float *voltage, char* name);

    bool update_wind(int idx, const char* bearing, const char* speed_10ms);
    bool update_barometer(int idx, const char* pressure);

    bool update_switch(int idx, bool state);
    bool get_switch_status(int idx, char *status, char *name);

    bool get_device_data(int idx, char *data, char *name);

    float vbat(void);
    int vbat_percentage(void);
    int rssi(void);
    int rssi_12level(void);

  private:
    bool _communicate(void);
    bool _update_sensor(int idx, int nvalue, int n, ...);
    bool _get_device_status(int idx);
    void _dbg_connect_info(void);

    char _buff[DOMO_BUFF_MAX];
#if DOMOTICZ_INTERFACE==DOMOTICZ_WIFI
    char _wifi_ssid[32];
    char _wifi_pass[32];
#endif
    char _domo_server[32];
    char _domo_port[5];
    char _domo_user[32];
    char _domo_pass[32];


};


//  Select a default interface if unspecified
#ifndef DOMOTICZ_INTERFACE
  #warning No DOMOTICZ_INTERFACE defined, using default from architecture
  #ifdef ARDUINO_ARCH_ESP8266
    #define DOMOTICZ_INTERFACE DOMOTICZ_WIFI
  #elif ARDUINO_ARCH_AVR
    #define DOMOTICZ_INTERFACE DOMOTICZ_ETHERNET
  #else
    #error No default DOMOTICZ_INTERFACE for this architecture!
  #endif
#endif

// Very Scrappy way to add modules in arduino subfolder ...
// But the only one found using ARDUINO IDE
#ifdef ARDUINO
  #include "domoticz.cpp"
  #if DOMOTICZ_INTERFACE==DOMOTICZ_ETHERNET
    #include "communicate_ethernet.c"
  #elif DOMOTICZ_INTERFACE==DOMOTICZ_WIFI
    #include "communicate_ethernet.c"
  #else
    #error No DOMOTICZ_INTERFACE Specified
  #endif
#endif

#endif
