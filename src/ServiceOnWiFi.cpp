
#include "ServiceOnWiFi.h"

#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <HTTPClient.h>

static WiFiServer *gServer = NULL;

void startService(String serviceName, uint16_t port) {
  gServer = new WiFiServer(port);
  gServer->begin();
  
  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin("esp32")) {
      Serial.println("Error setting up MDNS responder!");
      while(1);
  }
  Serial.println("mDNS responder started");
  
  MDNS.addService(serviceName, "tcp", port);
}

String getClientRequest(unsigned long timeout) {
  // Check if a client has connected
  WiFiClient client = gServer->available();
  if (!client) {
      delay(timeout);
      return "";
  }

  // Wait for data from client to become available
  while (client.connected() && !client.available()){
    delay(1);
  }

  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');

  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
      return "";
  }
  req = req.substring(addr_start + 1, addr_end);

  client.print("HTTP/1.1 200 OK\r\n\r\n");

  client.stop();

  return req;
}

void startClient() {
  if (!MDNS.begin("ESP32_Client")) {
    while(1){
      delay(1000);
    }
  }
}

String findService(String serviceName){
  // Serial.printf("Browsing for service _%s._%s.local. ... ", service, proto);
  int n = MDNS.queryService(serviceName, "tcp");
  if (n == 0) {
      Serial.println("no services found");
  } else {
      Serial.print(n);
      Serial.println(" service(s) found");
      for (int i = 0; i < n; ++i) {
          // Print details for each service found
          Serial.print("  ");
          Serial.print(i + 1);
          Serial.print(": ");
          Serial.print(MDNS.hostname(i));
          Serial.print(" (");
          Serial.print(MDNS.IP(i));
          Serial.print(":");
          Serial.print(MDNS.port(i));
          Serial.println(")");
      }
  }
  Serial.println();
  if (n >= 1) {
    return MDNS.IP(0).toString();
  } else {
    return "";
  }
}

// in ms, 2 seconds
#define CONNECT_TIMEOUT 1000 * 2

// in ms, 2 seconds
#define READ_TIMEOUT  1000 * 2

void sendReqToService(String serviceIp, String req) {
  HTTPClient http;

  http.setConnectTimeout(CONNECT_TIMEOUT);
  http.setTimeout(READ_TIMEOUT);
  
  Serial.print("[HTTP] begin...\n");
  http.begin(String("http://") + serviceIp + "/" + req);

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  // HTTP header has been send and Server response header has been handled
  Serial.printf("[HTTP] POST... code: %d\n", httpCode);

  http.end();
}

boolean loadWifiFromFS(fs::FS &fs, const String &filename, String &outssid, String &outpasswd) {
  File file = fs.open(filename.c_str());
  if(!file){
      Serial.println("Failed to open file for setup wifi");
      return false;
  }

  int configNumber = 0;
  String ssid = "";
  String line = "";
  while(file.available()) {
    char c = file.read();
    if (c != '\n') {
      if (c != '\r') { // 兼容 Windows 环境换行符
        line += c;
        Serial.print(c);
      }
    } else {
      if (ssid.length() == 0) {
        ssid = line;
        Serial.print("ssid:");
        Serial.println(ssid);
      } else {
        Serial.print("pwd:");
        Serial.println(line);
        
        // ssid has been set, so the second line is passwd, just add pair to wifi
        outssid = ssid;
        outpasswd = line;
        file.close();
        return true;
//        configNumber++;
//        // reset ssid
//        ssid = "";
      }
      line = "";
    }
  }
  file.close();
  if (ssid.length() > 0 && line.length() > 0) {
    Serial.print("pwd:");
    Serial.println(line);
    
    // ssid has been set, so the second line is passwd, just add pair to wifi
    outssid = ssid;
    outpasswd = line;
    return true;
  }
  return false;
}

String lookupTag(fs::FS &fs, const String &filename, const String &textIncludeTag) {
  File file = fs.open(filename.c_str());
  if (!file) {
    Serial.println("Failed to open file for reading");
    return "";
  }

  String line = "";
  while(file.available()) {
    char c = file.read();
    if (c != '\n') {
      line += c;
    } else {
      int idx = line.indexOf('=');
      if (idx >= 0) {
        String tag = line.substring(0, idx);
        String id = line.substring(idx + 1);
        if (textIncludeTag.indexOf(tag) >= 0) {
          file.close();
          return id;
        }
      }
      line = "";
    }
  }
  file.close();
  return "";
}


String loadfile(fs::FS &fs, const String &filename) {
  File file = fs.open(filename.c_str());
  if (!file){
    Serial.println("Failed to open file for reading");
    return "";
  }

  String line = "";
  while(file.available()) {
    char c = file.read();
    if (c != '\n') {
      line += c;
    } else {
      break;
    }
  }
  file.close();
  return line;
}
