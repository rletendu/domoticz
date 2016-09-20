
#ifndef domoticz_h
#define domoticz_h

#include "Arduino.h"
#include "config.h"
#include <ESP8266WiFi.h>

#define WIFI_TIMEOUT_MAX 50
#define DOMO_BUFF_MAX 1000
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
    bool update_luminosity(int idx, const char* lux);
    bool udpate_temp_hum(int idx, const char* temp, const char* hum);
    bool udpate_temp_hum_baro(int idx, const char* temp, const char* hum, const char* baro);
    bool update_voltage(int idx, const char* voltage);
    bool update_wind(int idx, const char* bearing, const char* speed_10ms);
    bool update_barometer(int idx, const char* pressure);
    bool update_switch(int idx, bool state);


  private:
    bool exchange(void);
    bool _update_sensor(int idx, int n, ...);

    WiFiClient _client;
    char _buff[DOMO_BUFF_MAX];

};

#endif
