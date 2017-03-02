#include "Arduino.h"
#include "domoticz.h"

#if DOMOTICZ_INTERFACE==DOMOTICZ_WIFI
  #ifdef ARDUINO_ARCH_ESP8266
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
    #else
      #error "Wifi interface for AVR not yet implemented"
    #endif
#elif DOMOTICZ_INTERFACE==DOMOTICZ_ETHERNET
  #ifdef ARDUINO_ARCH_AVR
    #include <SPI.h>
    #include <Ethernet.h>
    #include <ArduinoHttpClient.h>
    byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
    EthernetClient ethernet_client;
    HttpClient client(ethernet_client,DOMOTICZ_SERVER,DOMOTICZ_PORT);
  #endif
#else
  #error NO DOMOTICZ_INTERFACE selected
#endif

#include <ArduinoJson.h>

#ifndef DOMOTICZ_SERVER
#error DOMOTICZ_SERVER is not defined
#endif

#ifndef DOMOTICZ_PORT
#error DOMOTICZ_PORT is not defined
#endif

#ifdef DOMOTICZ_SEND_VBAT
  ADC_MODE(ADC_VCC);
#endif

Domoticz::Domoticz(void)
{
#if DOMOTICZ_INTERFACE==DOMOTICZ_WIFI
  WiFi.disconnect(false);
  WiFi.mode(WIFI_STA);
  _wifi_ssid[0] = 0;
  _wifi_pass[0] = 0;
#elif DOMOTICZ_INTERFACE==DOMOTICZ_ETHERNET
  _domo_server[0] = 0;
  _domo_port[0] = 0;
  _domo_user[0] = 0;
  _domo_pass[0] = 0;
#endif
}


bool Domoticz::begin(void)
{
#if DOMOTICZ_INTERFACE==DOMOTICZ_WIFI
  return begin(MYSSID, PASSWD, DOMOTICZ_SERVER, DOMOTICZ_PORT, "", "");
#elif DOMOTICZ_INTERFACE==DOMOTICZ_ETHERNET
  return begin(DOMOTICZ_SERVER, DOMOTICZ_PORT, "", "");
#endif
}


#if DOMOTICZ_INTERFACE==DOMOTICZ_WIFI
bool Domoticz::begin(char *ssid, char *passwd,char *server,char *port, char *domo_user, char *domo_passwd )
{
  char wifi_timeout = 0;
  int i;

  for (i = 0; i <= strlen(ssid); i++) {
    _wifi_ssid[i] = ssid[i];
  }
  for (i = 0; i <= strlen(passwd); i++) {
    _wifi_pass[i] = passwd[i];
  }

  for (i = 0; i <= strlen(server); i++) {
    _domo_server[i] = server[i];
  }
  for (i = 0; i <= strlen(port); i++) {
    _domo_port[i] = port[i];
  }
  if (strlen(domo_user)) {
    for (i = 0; i <= strlen(domo_user); i++) {
      _domo_user[i] = domo_user[i];
    }
    for (i = 0; i <= strlen(domo_passwd); i++) {
      _domo_pass[i] = domo_passwd[i];
    }
  }

  DEBUG_DOMO_PRINT("-Connecting Wifi "); DEBUG_DOMO_PRINTLN(MYSSID);
  WiFi.disconnect(false);
  WiFi.mode(WIFI_STA);
  DEBUG_DOMO_PRINT("MAC: "); DEBUG_DOMO_PRINTLN(WiFi.macAddress().c_str());
  WiFi.begin(MYSSID, PASSWD);
  while (WiFi.status() != WL_CONNECTED) {
    if (++wifi_timeout > WIFI_TIMEOUT_MAX) {
      DEBUG_DOMO_PRINTLN("-TIMEOUT!");
      DEBUG_DOMO_PRINT("-Wifi Status:");DEBUG_DOMO_PRINTLN(WiFi.status());
      return false;
    }
    delay(500);
  }
  DEBUG_DOMO_PRINT("IP:");DEBUG_DOMO_PRINTLN(WiFi.localIP());
  DEBUG_DOMO_PRINT("Link:");DEBUG_DOMO_PRINT(WiFi.RSSI());DEBUG_DOMO_PRINTLN("dBm");
  return true;
}
#endif

#if DOMOTICZ_INTERFACE==DOMOTICZ_ETHERNET
bool Domoticz::begin(char *server,char *port, char *domo_user, char *domo_passwd )
{
  int i;
  for (i = 0; i <= strlen(server); i++) {
    _domo_server[i] = server[i];
  }
  for (i = 0; i <= strlen(port); i++) {
    _domo_port[i] = port[i];
  }
  if (strlen(domo_user)) {
    for (i = 0; i <= strlen(domo_user); i++) {
      _domo_user[i] = domo_user[i];
    }
    for (i = 0; i <= strlen(domo_passwd); i++) {
      _domo_pass[i] = domo_passwd[i];
    }
  }
  DEBUG_DOMO_PRINTLN(F("-Connecting Ethernet "));
  if (Ethernet.begin(mac) == 0) {
    DEBUG_DOMO_PRINTLN(F("Failed to configure Ethernet using DHCP"));
    while(1);
  }
  DEBUG_DOMO_PRINT("IP:");DEBUG_DOMO_PRINTLN(Ethernet.localIP());
  return true;
}
#endif

bool Domoticz::send_log_message(char *message)
{
  int i;
  String str = "/json.htm?type=command&param=addlogmessage&message=";
  // Be sure to remove spaces in string -> replaced by '_'
  for (i = 0; i < strlen(message); i++) {
    if (message[i] == ' ') {
      message[i] = '_';
    }
  }
  str += String(message);
  str.toCharArray(_buff, DOMO_BUFF_MAX);
  if (_exchange()) {
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
  if (_exchange()) {
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
  if (_exchange()) {
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
  if (_exchange()) {
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
  if (_exchange()) {
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

bool Domoticz::_update_sensor(int idx, int nvalue, int n, ...)
{
  int i;
  int vbat;
  int quality;
  int dbm;

  String str = "/json.htm?type=command&param=udevice&idx=" + String(idx) + "&nvalue=";
  str +=nvalue;
  str += "&svalue=";
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

#ifdef DOMOTICZ_SEND_VBAT
  str += "&battery=";
  str += vbat_percentage();
#endif

#ifdef DOMOTICZ_SEND_RSSI
  str += "&rssi=";
  str += rssi_12level();
#endif

  str.toCharArray(_buff, DOMO_BUFF_MAX);
  if (_exchange()) {
    return true;
  } else {
    return false;
  }
}

bool Domoticz::update_temperature(int idx, const char* temp)
{
  return _update_sensor(idx, 0, 1, temp);
}

bool Domoticz::update_temperature(int idx, float temp)
{
  char str_temp[10];
  dtostrf(temp, 3, 1, str_temp);
  return _update_sensor(idx, 0, 1, str_temp);
}

bool Domoticz::get_temperature(int idx, float *temp, char *name)
{
  uint8_t dummy_hum;
  return get_temp_hum(idx, temp, &dummy_hum, name);
}


bool Domoticz::update_luminosity(int idx, const char* lux)
{
  return _update_sensor(idx, 0, 1, lux);
}

bool Domoticz::update_luminosity(int idx, int lux)
{
  char str_lum[10];
  snprintf(str_lum, sizeof(str_lum), "%d", lux);
  return _update_sensor(idx, 0, 1, str_lum);
}

bool Domoticz::update_voltage(int idx, const char* volt)
{
  return _update_sensor(idx, 0, 1, volt);
}

bool Domoticz::update_voltage(int idx, float volt)
{
  char str_volt[10];
  dtostrf(volt, 3, 1, str_volt);
  return _update_sensor(idx, 0, 1, str_volt);
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
  return _update_sensor(idx, 0, 3, temp, hum, "0");
}

bool Domoticz::udpate_temp_hum(int idx, float temp, float hum)
{
  char str_temp[10];
  char str_hum[10];
  dtostrf(temp, 3, 1, str_temp);
  dtostrf(hum, 3, 1, str_hum);
  return _update_sensor(idx, 0, 3, str_temp, str_hum, "0");
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
  return _update_sensor(idx, 0, 2, pressure, "0");
}

bool Domoticz::udpate_temp_hum_baro(int idx, const char* temp, const char* hum, const char* baro)
{
  return _update_sensor(idx, 0, 5, temp, hum, "0", baro, "0");
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
  return _update_sensor(idx, 0, 6, bearing, arr[(val % 16)], speed_10ms, speed_10ms, "0", "0");
}

bool Domoticz::update_switch(int idx, bool state)
{
  if (state) {
    return _update_sensor(idx, 1, 1, "1");
  } else {
    return _update_sensor(idx, 0, 1, "0");
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

bool Domoticz::_get_device_status(int idx)
{
  String str = "/json.htm?type=devices&rid=" + String(idx);
  str.toCharArray(_buff, DOMO_BUFF_MAX);
  return _exchange();
}

float Domoticz::vbat(void)
{
#ifdef ARDUINO_ARCH_ESP8266
  return ESP.getVcc()/1000.0;
#else
  return 5.0;
#endif
}

int Domoticz::vbat_percentage(void)
{
#ifdef ARDUINO_ARCH_ESP8266
  return map(ESP.getVcc(), DOMOTICZ_VBAT_MIN, DOMOTICZ_VBAT_MAX, 0, 100);
#else
  return 100;
#endif

}

int Domoticz::rssi(void)
{
#if DOMOTICZ_INTERFACE==DOMOTICZ_WIFI
  return WiFi.RSSI();
#else
  return 12;
#endif
}

int Domoticz::rssi_12level(void)
{
  int dbm;
  int quality;
#if DOMOTICZ_INTERFACE==DOMOTICZ_WIFI
  dbm = WiFi.RSSI();
#else
  dbm = 0;
#endif
 // dBm to Quality:
  if(dbm <= -100) {
    quality = 0;
  } else if(dbm >= -50) {
    quality = 12;
  } else {
    quality = 2 * (dbm + 100);
    quality = map(quality, 0 , 100, 0, 12);
  }
  return quality;
}

#if DOMOTICZ_INTERFACE==DOMOTICZ_WIFI
bool Domoticz::_exchange(void)
{
  int i;
  HTTPClient http;
  String url = "http://"+String(DOMOTICZ_SERVER)+":"+String(DOMOTICZ_PORT)+(String)_buff;
  DEBUG_DOMO_PRINT("- Domo  url: ");DEBUG_DOMO_PRINTLN(url);
  http.begin(url);
#ifdef DOMOTICZ_USER
  http.setAuthorization(DOMOTICZ_USER,DOMOTICZ_PASSWD);
#endif
  int httpCode = http.GET();
  if(httpCode !=  HTTP_CODE_OK) {
    DEBUG_DOMO_PRINT("[HTTP] GET failed code: ");DEBUG_DOMO_PRINTLN(httpCode);
    http.end();
    return false;
  }
  String str = http.getString();
  http.end();

  str = str.substring(str.indexOf('{'));
  for (i = 0; i < sizeof(_buff); i++) { _buff[i] = 0; ESP.wdtFeed(); }
  str.toCharArray(_buff, DOMO_BUFF_MAX);
  DEBUG_DOMO_PRINT("_buff content:"); DEBUG_DOMO_PRINTLN(strlen(_buff)); DEBUG_DOMO_PRINTLN(_buff);
  return true;
}
#endif


#if DOMOTICZ_INTERFACE==DOMOTICZ_ETHERNET
bool Domoticz::_exchange(void)
  int i;
  DEBUG_DOMO_PRINT(F("Get:"));DEBUG_DOMO_PRINTLN(_buff);
  client.stop();
  // client.setHttpResponseTimeout(10);
  //ethernet_client.stop();
  //client.setHttpResponseTimeout(100);
  //client.beginRequest();
  client.get(_buff);
  ////client.sendBasicAuth("username", "password"); // send the username and password for authentication
  //client.endRequest();
  i = client.responseStatusCode();
  DEBUG_DOMO_PRINT(F("GetStatus:"));DEBUG_DOMO_PRINTLN(i);
  if (i != 200 ) {
    client.stop();
    return false;
  }

  String str = client.responseBody();
  str = str.substring(str.indexOf('{'));
  for (i = 0; i < sizeof(_buff); i++) { _buff[i] = 0; }
  str.toCharArray(_buff, DOMO_BUFF_MAX);
  DEBUG_DOMO_PRINT("GetData:");DEBUG_DOMO_PRINTLN(_buff);
  delay(100);
  client.stop();
  return true;
}
#endif
/*
  String script = "/json.htm?";
  String type_command = "type=command";
  String param_udevice = "&param=udevice";
  String idx_SetTemp = "&idx=3";
  String nvalue_0 = "&nvalue=0";
  String svalue = "&svalue=";
  String dataInput = "";
  static float temperature = -10.2;
#if 1
  domoticz_client.connect(domoticz_server, 8080);
  // Set temp
  dataInput = script + type_command + param_udevice + idx_SetTemp + nvalue_0 + svalue + temperature + "&battery=89";
  Serial.println(dataInput);
  Serial.println();
  domoticz_client.println("GET " + dataInput + " HTTP/1.1");
  //domoticz_client.println("Host: 10.171.140.73");
  //domoticz_client.println("User-Agent: Mozilla/5.0");
  //domoticz_client.println("Connection: close");
  domoticz_client.println();
  domoticz_client.stop();
  delay(10000);
  temperature += 1;
#endif
*/
