#include "Arduino.h"
#include "domoticz.h"

#ifdef ARDUINO_ARCH_AVR
  #include <SPI.h>
  #ifndef USE_UIPETHERNET
    #include <Ethernet.h>
    #include <utility/w5100.h>
  #else
    #include <UIPEthernet.h>
  #endif
  #ifndef ETHERNET_MAC
    #define ETHERNET_MAC { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  #endif
  byte mac[] = ETHERNET_MAC;
  EthernetClient ethernet_client;
#endif

#ifndef ETHERNET_CONNECTION_RETRY
#define ETHERNET_CONNECTION_RETRY 5
#endif

int _b64_encode(const unsigned char* aInput, int aInputLen, unsigned char* aOutput, int aOutputLen);

bool Domoticz::begin(char *server,char *port, char *domo_user, char *domo_passwd )
{
  int i;
  for (i = 0; i <= strlen(server); i++) {
    _domo_server[i] = server[i];
  }
  for (i = 0; i <= strlen(port); i++) {
    _domo_port[i] = port[i];
  }
  for (i = 0; i <= strlen(domo_user); i++) {
    _domo_user[i] = domo_user[i];
  }
  for (i = 0; i <= strlen(domo_passwd); i++) {
    _domo_pass[i] = domo_passwd[i];
  }
  _dbg_connect_info();
  DEBUG_DOMO_PRINTLN(F("-Connecting Ethernet "));
  if (Ethernet.begin(mac) == 0) {
    DEBUG_DOMO_PRINTLN(F("-Failed to DHCP"));
    return false;
  }
  DEBUG_DOMO_PRINT("IP:");DEBUG_DOMO_PRINTLN(Ethernet.localIP());
  return true;
}

bool Domoticz::_communicate(void)
{
  int i,j;
  bool json_answer = false;
  unsigned char input[3];
  unsigned char output[5];
  bool status_line = true;
  const char* http_ok = "HTTP/1.1 200 OK";

  DEBUG_DOMO_PRINT(F("-Connecting Server "));
  DEBUG_DOMO_PRINT(_domo_server); DEBUG_DOMO_PRINT(":");
  DEBUG_DOMO_PRINTLN(_domo_port);
  #ifndef USE_UIPETHERNET
  W5100.setRetransmissionCount(2); // 8 is the default ?
  #endif
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
  // Send Http GET Request
  DEBUG_DOMO_PRINTLN(F("-Sending GET Request"));
  i = ethernet_client.println("GET " + String(_buff) + " HTTP/1.1");

  // Send BasicAuth header if needed
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
          if ( (inputOffset == 3) || (i == userLen+passwordLen) ) {
              _b64_encode(input, inputOffset, output, 4);
              output[4] = '\0';
              ethernet_client.print((char*)output);
              inputOffset = 0;
          }
      }
      ethernet_client.println();
  }
  // Empty line to finish request
  i= ethernet_client.println();

  // Read Back Answer
  i = 0;
  j = 0;

  while(ethernet_client.connected()) {
    if (ethernet_client.available()) {
      char c = ethernet_client.read();
      //DEBUG_DOMO_PRINT(c);
      if (status_line) {
        if (c =='\r') {
          status_line = false;
        } else if ( c != http_ok[j] ) {
              DEBUG_DOMO_PRINTLN(F("-Incorrect HTTP Status: "));
              ethernet_client.stop();
              return false;
        }
        j++;
      }
      if (c=='{') {
          json_answer = true;
      }
      if (json_answer) {
        _buff[i] = c;
        i++;
      }
    }
  }
  _buff[i] = 0;

  ethernet_client.stop();
  delay(100);

  DEBUG_DOMO_PRINT(F("-GET Data:\n"));DEBUG_DOMO_PRINTLN(_buff);
  if (json_answer) {
    return true;
  } else {
    return false;
  }
}


int _b64_encode(const unsigned char* aInput, int aInputLen, unsigned char* aOutput, int aOutputLen)
{
    if (aOutputLen < (aInputLen*8)/6) {
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
        int i;
        for (i = 0; i < aInputLen/3; i++) {
            _b64_encode(&aInput[i*3], 3, &aOutput[i*4], 4);
        } if (aInputLen % 3 > 0) {
            _b64_encode(&aInput[i*3], aInputLen % 3, &aOutput[i*4], aOutputLen - (i*4));
        }
    }
    return ((aInputLen+2)/3)*4;
}
