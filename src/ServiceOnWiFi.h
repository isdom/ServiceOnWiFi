
#pragma once

#include <Arduino.h>
#include <FS.h>

void startService(String serviceName, uint16_t port);

String getClientRequest(unsigned long timeout);

void startClient();

String findService(String serviceName);

void sendReqToService(String serviceIp, String req);

boolean loadWifiFromFS(fs::FS &fs, const String &filename, String &outssid, String &outpasswd);

String lookupTag(fs::FS &fs, const String &filename, const String &textIncludeTag);

String loadfile(fs::FS &fs, const String &filename);
