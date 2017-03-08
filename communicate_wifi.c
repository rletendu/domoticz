#include "Arduino.h"
#include "domoticz.h"

#ifdef ARDUINO_ARCH_ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #else
    #error "Wifi interface for AVR not yet implemented"
  #endif
#endif

bool Domoticz::begin(char *ssid, char *passwd,char *server,char *port, char *domo_user, char *domo_passwd )
{
  WiFi.disconnect(false);
  WiFi.mode(WIFI_STA);
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
  DEBUG_DOMO_PRINT(F("-Connecting Wifi ")); DEBUG_DOMO_PRINTLN(MYSSID);
  WiFi.disconnect(false);
  WiFi.mode(WIFI_STA);
  DEBUG_DOMO_PRINT(F("MAC: ")); DEBUG_DOMO_PRINTLN(WiFi.macAddress().c_str());
  WiFi.begin(MYSSID, PASSWD);
  while (WiFi.status() != WL_CONNECTED) {
    if (++wifi_timeout > WIFI_TIMEOUT_MAX) {
      DEBUG_DOMO_PRINTLN(F("-TIMEOUT!"));
      DEBUG_DOMO_PRINT(F("-Wifi Status:"));DEBUG_DOMO_PRINTLN(WiFi.status());
      return false;
    }
    delay(500);
  }
  DEBUG_DOMO_PRINT("IP:");DEBUG_DOMO_PRINTLN(WiFi.localIP());
  DEBUG_DOMO_PRINT("Link:");DEBUG_DOMO_PRINT(WiFi.RSSI());DEBUG_DOMO_PRINTLN("dBm");
  return true;
}

bool Domoticz::_communicate(void)
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
