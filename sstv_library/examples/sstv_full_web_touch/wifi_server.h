// Copyright (c) Francesco Capuzzi 2025 IS0JSV
// MIT License

#ifndef __WIFI_SERVER_H__
#define __WIFI_SERVER_H__

#include <WiFi.h>
#include <WiFiServer.h>

extern uint16_t count_bitmaps(Dir& root);

class wifi_server {
 

  const char* ssid = "Undixedda noa";
  const char* password = "Miagolina25!";

  WiFiServer server;

  bool connected = false;

  void sendError(WiFiClient& client, uint16_t error, const char* text);
  void sendHttpOK(WiFiClient& client, const char* type);
  bool isImage(String name);
  void sendImage(WiFiClient& client, String filename);
  void sendGallery(WiFiClient& client, int page, bool tx);
  void getImage(WiFiClient& client);

public:
  wifi_server();
  void connect();
  void reconnectWiFiAndClient();
  void connectToWiFi();
  void disconnectWiFi();
  void poll_wifi_client();

};
#endif