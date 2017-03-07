#include "Arduino.h"
#include "domoticz.h"
/*
  Select a default interface if unspecified
*/
#ifndef DOMOTICZ_INTERFACE
  #warning No DOMOTICZ_INTERFACE defined, using default from architecture
  #ifdef ARDUINO_ARCH_ESP8266
    #define DOMOTICZ_INTERFACE DOMOTICZ_WIFI
  #elif ARDUINO_ARCH_AVR
    #define DOMOTICZ_INTERFACE DOMOTICZ_ETHERNET
  #else
    #error No defaultDOMOTICZ_INTERFACE for this architecture!
  #endif
#endif

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
    #include <utility/w5100.h>
    #ifndef ETHERNET_MAC
      #define ETHERNET_MAC { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
    #endif
    byte mac[] = ETHERNET_MAC;
    EthernetClient ethernet_client;
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

#ifndef ETHERNET_CONNECTION_RETRY
#define ETHERNET_CONNECTION_RETRY 5
#endif

int _b64_encode(const unsigned char* aInput, int aInputLen, unsigned char* aOutput, int aOutputLen);

Domoticz::Domoticz(void)
{
#if DOMOTICZ_INTERFACE==DOMOTICZ_WIFI
  WiFi.disconnect(false);
  WiFi.mode(WIFI_STA);
  _wifi_ssid[0] = 0;
  _wifi_pass[0] = 0;
#endif

  _domo_server[0] = 0;
  _domo_port[0] = 0;
  _domo_user[0] = 0;
  _domo_pass[0] = 0;
}


bool Domoticz::begin(void)
{
#if DOMOTICZ_INTERFACE==DOMOTICZ_WIFI
  return begin(MYSSID, PASSWD, DOMOTICZ_SERVER, DOMOTICZ_PORT, DOMOTICZ_USER, DOMOTICZ_PASSWD);
#elif DOMOTICZ_INTERFACE==DOMOTICZ_ETHERNET
  return begin(DOMOTICZ_SERVER, DOMOTICZ_PORT, DOMOTICZ_USER, DOMOTICZ_PASSWD);
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
  _dbg_connect_info();
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
  _dbg_connect_info();
  DEBUG_DOMO_PRINTLN(F("-Connecting Ethernet "));
  if (Ethernet.begin(mac) == 0) {
    DEBUG_DOMO_PRINTLN(F("Failed to configure Ethernet using DHCP"));
    while(1);
  }
  DEBUG_DOMO_PRINT("IP:");DEBUG_DOMO_PRINTLN(Ethernet.localIP());
  return true;
}
#endif

void Domoticz::_dbg_connect_info(void)
{
  DEBUG_DOMO_PRINT(F("-Using Server:Port "));
  DEBUG_DOMO_PRINT(_domo_server);DEBUG_DOMO_PRINT(F(":"));DEBUG_DOMO_PRINTLN(_domo_port);
  DEBUG_DOMO_PRINT(F("-Using User:Passwd "));
  DEBUG_DOMO_PRINT(_domo_user);DEBUG_DOMO_PRINT(F(":"));DEBUG_DOMO_PRINTLN(_domo_pass);
}

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
      DEBUG_DOMO_PRINTLN(F("-Error Parsing Json"));
      return false;
    } else {
      DEBUG_DOMO_PRINTLN(F("-Parsing OK"));
      if (root.containsKey("result")) {
        strcpy(var, root["result"][0]["Value"]);
      } else {
        DEBUG_DOMO_PRINTLN(F("-Value not found"));
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
      DEBUG_DOMO_PRINTLN(F("-Error Parsing Json"));
      return false;
    } else {
      DEBUG_DOMO_PRINTLN(F("-Parsing OK"));
      if (root.containsKey("ServerTime")) {
        strcpy(servertime, root["ServerTime"]);
      } else {
        DEBUG_DOMO_PRINTLN(F("-Value not found"));
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
      DEBUG_DOMO_PRINTLN(F("-Error Parsing Json"));
      return false;
    } else {
      DEBUG_DOMO_PRINTLN(F("-Parsing OK"));
      if (root.containsKey("Sunrise")) {
        strcpy(sunrise, root["Sunrise"]);
      } else {
        DEBUG_DOMO_PRINTLN(F("Value not found"));
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
      DEBUG_DOMO_PRINTLN(F("-Error Parsing Json"));
      return false;
    } else {
      DEBUG_DOMO_PRINTLN(F("-Parsing OK"));
      if (root.containsKey("Sunset")) {
        strcpy(sunset, root["Sunset"]);
      } else {
        DEBUG_DOMO_PRINTLN(F("-Value not found"));
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
  String url = "http://"+String(_domo_server)+":"+String(_domo_port)+(String)_buff;
  DEBUG_DOMO_PRINT("- Domo  url: ");DEBUG_DOMO_PRINTLN(url);
  http.begin(url);
  if (strlen(_domo_user)) {
    http.setAuthorization(_domo_user,_domo_pass);
  }
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
{
  int i,j;
  bool store = false;
  unsigned char input[3];
  unsigned char output[5]; // Leave space for a '\0'

  DEBUG_DOMO_PRINT(F("-Connecting Server "));
  DEBUG_DOMO_PRINT(_domo_server); DEBUG_DOMO_PRINT(":");
  DEBUG_DOMO_PRINTLN(_domo_port);
  W5100.setRetransmissionCount(2); // 8 is the default ?
  int userLen = strlen(_domo_user);
  int passwordLen = strlen(_domo_pass);
  int inputOffset = 0;

  ethernet_client.setTimeout(500);
  for (j=0;;j++) {
    i = ethernet_client.connect(_domo_server, atoi(_domo_port));
    if (i <= 0 ) {
      DEBUG_DOMO_PRINT(F("-Server Connect Fail: ")); DEBUG_DOMO_PRINTLN(i);
      if (j>ETHERNET_CONNECTION_RETRY)
      return false;
    } else {
      DEBUG_DOMO_PRINTLN(F("-Server Connect OK"));
      break;
    }
  }
  DEBUG_DOMO_PRINTLN(F("-Sending GET Request"));
  i = ethernet_client.println("GET " + String(_buff) + " HTTP/1.1");
  if (userLen) {
      DEBUG_DOMO_PRINTLN(F("-Sending BasicAuth Header"));
      ethernet_client.print("Authorization: Basic ");
      for (i = 0; i < (userLen+1+passwordLen); i++) {
          if (i < userLen) {
              input[inputOffset++] = _domo_user[i];
          }
          else if (i == userLen) {
              input[inputOffset++] = ':';
          } else {
              input[inputOffset++] = _domo_pass[i-(userLen+1)];
          }
          // See if we've got a chunk to encode
          if ( (inputOffset == 3) || (i == userLen+passwordLen) ) {
              _b64_encode(input, inputOffset, output, 4);
              output[4] = '\0';
              ethernet_client.print((char*)output);
              inputOffset = 0;
          }
      }
       ethernet_client.println();
  }
  i= ethernet_client.println();
  i = 0;
  while(ethernet_client.connected()) {
    if (ethernet_client.available()) {
      char c = ethernet_client.read();
      // DEBUG_DOMO_PRINT(c);
      if (c=='{') {
          store = true;
      }
      if (store) {
        _buff[i] = c;
        i++;
      }
    }
  }
  ethernet_client.stop();
  _buff[i] = 0;
  DEBUG_DOMO_PRINT(F("-GET Data:\n"));DEBUG_DOMO_PRINTLN(_buff);
  if (store) {
    return true;
  } else {
    return false;
  }
}
#endif

int _b64_encode(const unsigned char* aInput, int aInputLen, unsigned char* aOutput, int aOutputLen)
{
    if (aOutputLen < (aInputLen*8)/6) {
        // FIXME Should we return an error here, or just the length
        return (aInputLen*8)/6;
    }
    const char* b64_dictionary = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (aInputLen == 3) {
        aOutput[0] = b64_dictionary[aInput[0] >> 2];
        aOutput[1] = b64_dictionary[(aInput[0] & 0x3)<<4|(aInput[1]>>4)];
        aOutput[2] = b64_dictionary[(aInput[1]&0x0F)<<2|(aInput[2]>>6)];
        aOutput[3] = b64_dictionary[aInput[2]&0x3F];
    } else if (aInputLen == 2) {
        aOutput[0] = b64_dictionary[aInput[0] >> 2];
        aOutput[1] = b64_dictionary[(aInput[0] & 0x3)<<4|(aInput[1]>>4)];
        aOutput[2] = b64_dictionary[(aInput[1]&0x0F)<<2];
        aOutput[3] = '=';
    } else if (aInputLen == 1) {
        aOutput[0] = b64_dictionary[aInput[0] >> 2];
        aOutput[1] = b64_dictionary[(aInput[0] & 0x3)<<4];
        aOutput[2] = '=';
        aOutput[3] = '=';
    } else {
        // Break the input into 3-byte chunks and process each of them
        int i;
        for (i = 0; i < aInputLen/3; i++) {
            _b64_encode(&aInput[i*3], 3, &aOutput[i*4], 4);
        } if (aInputLen % 3 > 0) {
             // It doesn't fit neatly into a 3-byte chunk, so process what's left
            _b64_encode(&aInput[i*3], aInputLen % 3, &aOutput[i*4], aOutputLen - (i*4));
        }
    }
    return ((aInputLen+2)/3)*4;
}
