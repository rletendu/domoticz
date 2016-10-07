
#ifndef domoticz_h
#define domoticz_h

#include "Arduino.h"
#include "../config_domoticz.h"
#include <ESP8266WiFi.h>

#define WIFI_TIMEOUT_MAX 50
#define DOMO_BUFF_MAX 1500
#define JSON_BUFF 600

// Uncomment to enable printing out nice debug messages.
//#define DOMOTICZ_DEBUG

// Define where debug output will be printed.
#define DEBUG_PRINTER Serial

// Setup debug printing macros.
#ifdef DOMOTICZ_DEBUG
#define DEBUG_PRINT(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
#define DEBUG_PRINTLN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#else
#define DEBUG_PRINT(...) {}
#define DEBUG_PRINTLN(...) {}
#endif

class Domoticz
{
  public:
    Domoticz();
    bool begin(void);
    bool stop(void);
    bool get_variable(int idx, char* var);
    bool get_servertime(char *servertime);
    bool get_sunrise(char *sunrise);
    bool get_sunset(char *sunset);
    bool send_log_message(char *message);

    bool update_temperature(int idx, const char* temp);
    bool get_temperature(int idx, float *temp, char *name);

    bool update_luminosity(int idx, const char* lux);
    bool get_luminosity(int idx, int lum, char* name);

    bool udpate_temp_hum(int idx, const char* temp, const char* hum);
    bool udpate_temp_hum(int idx, float temp, float hum);
    bool get_temp_hum(int idx, float *temp, uint8_t *hum, char *name);

    bool udpate_temp_hum_baro(int idx, const char* temp, const char* hum, const char* baro);
    bool get_temp_hum_baro(int idx, float *temp, uint8_t *hum, uint16_t *baro, char *name);

    bool update_voltage(int idx, const char* voltage);
    bool get_voltage(int idx, float voltage, char* name);

    bool update_wind(int idx, const char* bearing, const char* speed_10ms);
    bool update_barometer(int idx, const char* pressure);

    bool update_switch(int idx, bool state);
    bool get_switch_status(int idx, char *status, char *name);

    bool get_device_data(int idx, char *data, char *name);

  private:
    bool exchange(void);
    bool _update_sensor(int idx, int n, ...);
    bool _get_device_status(int idx);

    WiFiClient _client;
    char _buff[DOMO_BUFF_MAX];

};

#endif
