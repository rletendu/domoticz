#include "Arduino.h"
#include "domoticz.h"

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#ifndef DOMOTICZ_SERVER
#error DOMOTICZ_SERVER is not defined
#endif

#ifndef DOMOTICZ_PORT
#error DOMOTICZ_PORT is not defined
#endif
Domoticz::Domoticz(void)
{
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
}

bool Domoticz::begin(void)
{
  char wifi_timeout = 0;
  uint8_t mac[6];
  DEBUG_DOMO_PRINT("-Connecting Wifi "); DEBUG_DOMO_PRINTLN(MYSSID);
  WiFi.disconnect();
  
  WiFi.macAddress(mac);
  DEBUG_DOMO_PRINT("MAC: ");
  DEBUG_DOMO_PRINT(mac[5],HEX);
  DEBUG_DOMO_PRINT(":");
  DEBUG_DOMO_PRINT(mac[4],HEX);
  DEBUG_DOMO_PRINT(":");
  DEBUG_DOMO_PRINT(mac[3],HEX);
  DEBUG_DOMO_PRINT(":");
  DEBUG_DOMO_PRINT(mac[2],HEX);
  DEBUG_DOMO_PRINT(":");
  DEBUG_DOMO_PRINT(mac[1],HEX);
  DEBUG_DOMO_PRINT(":");
  DEBUG_DOMO_PRINTLN(mac[0],HEX);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(MYSSID, PASSWD);
  while (WiFi.status() != WL_CONNECTED) {
    if (++wifi_timeout > WIFI_TIMEOUT_MAX) {
      DEBUG_DOMO_PRINTLN("-TIMEOUT!");
      return false;
    }
    delay(500);
  }
  DEBUG_DOMO_PRINT("IP:");DEBUG_DOMO_PRINTLN(WiFi.localIP());
  DEBUG_DOMO_PRINT("Link:");DEBUG_DOMO_PRINT(WiFi.RSSI());DEBUG_DOMO_PRINTLN("dBm");
  return true;
}



bool Domoticz::send_log_message(char *message)
{
  int i;
  String str = "/json.htm?type=command&param=addlogmessage&message=";
  // Be sure to remove spaces in strinf -> replaced by '_'
  for (i = 0; i < strlen(message); i++) {
    if (message[i] == ' ') {
      message[i] = '_';
    }
  }
  str += String(message);
  str.toCharArray(_buff, DOMO_BUFF_MAX);
  if (exchange()) {
    return true;
  }
  else {
    return false;
  }
}

bool Domoticz::get_variable(int idx, char* var)
{
  String str = "/json.htm?type=command&param=getuservariable&idx=" + String(idx);
  str.toCharArray(_buff, DOMO_BUFF_MAX);
  if (exchange()) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_buff);
    if (!root.success()) {
      DEBUG_DOMO_PRINTLN("-Error Parsing Json");
      return false;
    } else {
      DEBUG_DOMO_PRINTLN("-Parsing OK");
      if (root.containsKey("result")) {
        strcpy(var, root["result"][0]["Value"]);
      } else {
        DEBUG_DOMO_PRINTLN("-Value not found");
        return false;
      }
      DEBUG_DOMO_PRINTLN(var);
      return true;
    }
  } else {
    return false;
  }
}

bool Domoticz::get_servertime(char* servertime)
{
  String str = "/json.htm?type=command&param=getSunRiseSet";
  str.toCharArray(_buff, DOMO_BUFF_MAX);
  if (exchange()) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_buff);
    if (!root.success()) {
      DEBUG_DOMO_PRINTLN("Error Parsing Json");
      return false;
    } else {
      DEBUG_DOMO_PRINTLN("Parsing OK");
      if (root.containsKey("ServerTime")) {
        strcpy(servertime, root["ServerTime"]);
      } else {
        DEBUG_DOMO_PRINTLN("Value not found");
        return false;
      }
      DEBUG_DOMO_PRINTLN(servertime);
      return true;
    }
  } else {
    return false;
  }
}

bool Domoticz::get_sunrise(char* sunrise)
{
  String str = "/json.htm?type=command&param=getSunRiseSet";
  str.toCharArray(_buff, DOMO_BUFF_MAX);
  if (exchange()) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_buff);
    if (!root.success()) {
      DEBUG_DOMO_PRINTLN("Error Parsing Json");
      return false;
    } else {
      DEBUG_DOMO_PRINTLN("Parsing OK");
      if (root.containsKey("Sunrise")) {
        strcpy(sunrise, root["Sunrise"]);
      } else {
        DEBUG_DOMO_PRINTLN("Value not found");
        return false;
      }
      DEBUG_DOMO_PRINTLN(sunrise);
      return true;
    }
  } else {
    return false;
  }
}

bool Domoticz::get_sunset(char* sunset)
{
  String str = "/json.htm?type=command&param=getSunRiseSet";
  str.toCharArray(_buff, DOMO_BUFF_MAX);
  if (exchange()) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_buff);
    if (!root.success()) {
      DEBUG_DOMO_PRINTLN("Error Parsing Json");
      return false;
    } else {
      DEBUG_DOMO_PRINTLN("Parsing OK");
      if (root.containsKey("Sunset")) {
        strcpy(sunset, root["Sunset"]);
      } else {
        DEBUG_DOMO_PRINTLN("Value not found");
        return false;
      }
      DEBUG_DOMO_PRINTLN(sunset);
      return true;
    }
  } else {
    return false;
  }
}

bool Domoticz::_update_sensor(int idx, int n, ...)
{
  int i;
  String str = "/json.htm?type=command&param=udevice&idx=" + String(idx) + "&nvalue=0&svalue=";
  char *z;

  if (n < 1) {
    return false;
  }
  va_list va;
  va_start (va, n);
  for (i = 0; i < n; i++)
  {
    z = va_arg (va, char *);
    str += z;
    if (i < (n - 1)) {
      str += ";";
    }
  }
  str.toCharArray(_buff, DOMO_BUFF_MAX);
  if (exchange()) {
    return true;

  } else {
    return false;
  }
}

bool Domoticz::update_temperature(int idx, const char* temp)
{
  return _update_sensor(idx, 1, temp);
}

bool Domoticz::update_temperature(int idx, float temp)
{
  char str_temp[10];
  dtostrf(temp, 3, 1, str_temp);
  return _update_sensor(idx, 1, str_temp);
}

bool Domoticz::get_temperature(int idx, float *temp, char *name)
{
  uint8_t dummy_hum;
  return get_temp_hum(idx, temp, &dummy_hum, name);
}


bool Domoticz::update_luminosity(int idx, const char* lux)
{
  return _update_sensor(idx, 1, lux);
}

bool Domoticz::update_luminosity(int idx, int lux)
{
  char str_lum[10];
  snprintf(str_lum, sizeof(str_lum), "%d", lux);
  return _update_sensor(idx, 1, str_lum);
}

bool Domoticz::update_voltage(int idx, const char* volt)
{
  return _update_sensor(idx, 1, volt);
}

bool Domoticz::update_voltage(int idx, float volt)
{
  char str_volt[10];
  dtostrf(volt, 3, 1, str_volt);
  return _update_sensor(idx, 1, str_volt);
}

bool Domoticz::get_voltage(int idx, float *voltage, char* name)
{
  if (_get_device_status(idx)) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_buff);
    if (!root.success()) {
      DEBUG_DOMO_PRINTLN("-Error Parsing Json");
      return false;
    } else {
      DEBUG_DOMO_PRINTLN("-Parsing OK");
      if (root.containsKey("result")) {
        const char * n = root["result"][0]["Name"];
        strcpy(name, n);
        float v = root["result"][0]["Voltage"];
        *voltage = v;
      } else {
        DEBUG_DOMO_PRINTLN("-Value not found");
        return false;
      }
      DEBUG_DOMO_PRINT("Name:");DEBUG_DOMO_PRINTLN(name);
      DEBUG_DOMO_PRINT("Voltage found:");DEBUG_DOMO_PRINTLN(*voltage);
      return true;
    }
  } else {
    return false;
  }
}

bool Domoticz::udpate_temp_hum(int idx, const char* temp, const char* hum)
{
  return _update_sensor(idx, 3, temp, hum, "0");
}

bool Domoticz::udpate_temp_hum(int idx, float temp, float hum)
{
  char str_temp[10];
  char str_hum[10];
  dtostrf(temp, 3, 1, str_temp);
  dtostrf(hum, 3, 1, str_hum);
  return _update_sensor(idx, 3, str_temp, str_hum, "0");
}

bool Domoticz::get_temp_hum(int idx, float *temp, uint8_t *hum, char *name)
{
  if (_get_device_status(idx)) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_buff);
    if (!root.success()) {
      DEBUG_DOMO_PRINTLN("-Error Parsing Json");
      return false;
    } else {
      DEBUG_DOMO_PRINTLN("-Parsing OK");
      if (root.containsKey("result")) {
        float temperature = root["result"][0]["Temp"];
        const char * n = root["result"][0]["Name"];
        strcpy(name, n);
        *temp = temperature;
        JsonObject & _ooo = root["result"][0];
        if (_ooo.containsKey("Humidity")) {
          uint8_t h = root["result"][0]["Humidity"];
          *hum = h;
        } else {
          *hum = 255;
        }
      } else {
        DEBUG_DOMO_PRINTLN("-Value not found");
        return false;
      }
      DEBUG_DOMO_PRINT("Name:");DEBUG_DOMO_PRINTLN(name);
      DEBUG_DOMO_PRINT("Temp found:");DEBUG_DOMO_PRINTLN(*temp);
      DEBUG_DOMO_PRINT("Hum found:");DEBUG_DOMO_PRINTLN(*hum);
      return true;
    }
  } else {
    return false;
  }
}

bool Domoticz::update_barometer(int idx, const char* pressure)
{
  return _update_sensor(idx, 2, pressure, "0");
}

bool Domoticz::udpate_temp_hum_baro(int idx, const char* temp, const char* hum, const char* baro)
{
  return _update_sensor(idx, 5, temp, hum, "0", baro, "0");
}

bool Domoticz::get_temp_hum_baro(int idx, float *temp, uint8_t *hum, uint16_t *baro, char *name)
{
  if (_get_device_status(idx)) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_buff);
    if (!root.success()) {
      DEBUG_DOMO_PRINTLN("-Error Parsing Json");
      return false;
    } else {
      DEBUG_DOMO_PRINTLN("-Parsing OK");
      if (root.containsKey("result")) {
        float temperature = root["result"][0]["Temp"];
        *temp = temperature;
        uint8_t h = root["result"][0]["Humidity"];
        *hum = h;
        uint16_t b = root["result"][0]["Barometer"];
        *baro = b;
        const char * n = root["result"][0]["Name"];
        strcpy(name, n);
      } else {
        DEBUG_DOMO_PRINTLN("-Value not found");
        return false;
      }
      DEBUG_DOMO_PRINT("Name:");DEBUG_DOMO_PRINTLN(name);
      DEBUG_DOMO_PRINT("Temp found:");DEBUG_DOMO_PRINTLN(*temp);
      DEBUG_DOMO_PRINT("Hum found:");DEBUG_DOMO_PRINTLN(*hum);
      return true;
    }
  } else {
    return false;
  }
}

bool Domoticz::update_wind(int idx, const char* bearing, const char* speed_10ms)
{
  int val = int((atoi(bearing) / 22.5) + .5);
  const char* arr[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  return _update_sensor(idx, 6, bearing, arr[(val % 16)], speed_10ms, speed_10ms, "0", "0");
}

bool Domoticz::update_switch(int idx, bool state)
{
  int i;
  String str = "/json.htm?type=command&param=udevice&idx=" + String(idx) + "&nvalue="; ;
  char *z;
  if (state) {
    str +="0&svalue=0";
  } else {
    str += "1&svalue=1";
  }
  str.toCharArray(_buff, DOMO_BUFF_MAX);
  if (exchange()) {
    return true;

  } else {
    return false;
  }
}

bool Domoticz::get_switch_status(int idx, char *status, char *name)
{
  if (_get_device_status(idx)) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_buff);
    if (!root.success()) {
      DEBUG_DOMO_PRINTLN("-Error Parsing Json");
      return false;
    } else {
      DEBUG_DOMO_PRINTLN("-Parsing OK");
      if (root.containsKey("result")) {
        const char * n = root["result"][0]["Name"];
        strcpy(name, n);
        const char * s = root["result"][0]["Status"];
        strcpy(status, s);
      } else {
        DEBUG_DOMO_PRINTLN("-Value not found");
        return false;
      }
      DEBUG_DOMO_PRINT("Name:");DEBUG_DOMO_PRINTLN(name);
      DEBUG_DOMO_PRINT("Status found:");DEBUG_DOMO_PRINTLN(status);
      return true;
    }
  } else {
    return false;
  }
}

bool Domoticz::get_device_data(int idx, char *data, char *name)
{
  if (_get_device_status(idx)) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_buff);
    if (!root.success()) {
      DEBUG_DOMO_PRINTLN("-Error Parsing Json");
      return false;
    } else {
      DEBUG_DOMO_PRINTLN("-Parsing OK");
      if (root.containsKey("result")) {
        const char * n = root["result"][0]["Name"];
        strcpy(name, n);
        const char * d = root["result"][0]["Data"];
        strcpy(data, d);
      } else {
        DEBUG_DOMO_PRINTLN("-Value not found");
        return false;
      }
      DEBUG_DOMO_PRINT("Name:");DEBUG_DOMO_PRINTLN(name);
      DEBUG_DOMO_PRINT("Data found:");DEBUG_DOMO_PRINTLN(data);
      return true;
    }
  } else {
    return false;
  }
}

bool Domoticz::exchange(void)
{
  int i;
  if (!_client.connect(DOMOTICZ_SERVER, DOMOTICZ_PORT)) {
    DEBUG_DOMO_PRINTLN("connection failed");
    return false;
  }
  _client.println("GET " + (String)_buff + " HTTP/1.1");
  DEBUG_DOMO_PRINT("Request Sent: ");
  DEBUG_DOMO_PRINTLN("GET " + (String)_buff + " HTTP/1.1");
  _client.println();
  String str = _client.readString();
  _client.stop();
  DEBUG_DOMO_PRINT("Response: ");
  DEBUG_DOMO_PRINTLN(str);
  if (str.indexOf("HTTP/1.1 200 OK") == -1) {
    DEBUG_DOMO_PRINTLN("Response NOK");
    return false;
  }
  str = str.substring(str.indexOf('{'));
  for (i = 0; i < sizeof(_buff); i++) { _buff[i] = 0; ESP.wdtFeed(); }
  str.toCharArray(_buff, DOMO_BUFF_MAX);
  DEBUG_DOMO_PRINT("_buff content:"); DEBUG_DOMO_PRINTLN(strlen(_buff)); DEBUG_DOMO_PRINTLN(_buff);
  return true;
}

bool Domoticz::_get_device_status(int idx)
{
  String str = "/json.htm?type=devices&rid=" + String(idx);
  str.toCharArray(_buff, DOMO_BUFF_MAX);
  return exchange();
}
