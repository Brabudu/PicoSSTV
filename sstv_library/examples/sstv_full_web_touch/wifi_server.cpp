// Copyright (c) Francesco Capuzzi 2025 IS0JSV
// MIT License

#include <WiFi.h>
#include <WiFiServer.h>
#include <SDFS.h>
#include "wifi_server.h"

wifi_server::wifi_server() {
  server = WiFiServer(80);
}

void wifi_server::connect() {
  if ((WiFi.status() == WL_CONNECTED) && (!connected)) {
    server.begin();
    connected = true;
  }
}

bool wifi_server::isImage(String name) {
  name.toLowerCase();
  return name.endsWith(".bmp");
}

void wifi_server::sendImage(WiFiClient& client, String filename) {
  FILE* file = fopen(filename.c_str(), "rb");
  if (file == NULL) {
    sendError(client, 404, "Not Found");
    return;
  }

  sendHttpOK(client, "image/bmp");

  int n;
  uint8_t buffer[1024];

  while ((n = fread(buffer, 1, sizeof(buffer) - 1, file)) > 0) {
    client.write(buffer, n);
  }
  fclose(file);
}

void wifi_server::sendError(WiFiClient& client, uint16_t error, const char* text) {
  client.print("HTTP/1.0 ");
  client.print(error);
  client.print(" ");
  client.println(text);
  client.println();
}

void wifi_server::sendHttpOK(WiFiClient& client, const char* type) {
  client.println("HTTP/1.0 200 OK");
  client.print("Content-Type: ");
  client.println(type);
  client.println("Connection: close");
  client.println();
}

void wifi_server::sendGallery(WiFiClient& client, int page, bool tx) {

  const char* folder;
  if (tx) folder = "/tx";
  else folder = "/";

  sendHttpOK(client, "text/html");

  client.println("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
  client.print("<title>");
  //client.print(settings.tx_callsign);
  if (tx) client.print(" TX");
  client.println(" SSTV gallery</title>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  client.println("<style>body{background: antiquewhite;}.foto{float:left;border:1px lightgray solid;padding: 5px;margin:10px;border-radius: 10px;background:white;height:322px;}img{margin:20px;width:320px;border:2px black solid}</style></head>");
  client.println("<body><h1>");
  if (tx) client.print("TX ");
  else client.print("RX ");
  client.print("Image gallery</h1><hr><h2>Page ");
  client.print(page);
  client.print("</h2><h2> Folder ");
  client.print(folder);
  client.println("</h2>");

  if (tx) {
    client.println("<br><a href='/'><button style='margin:5px;'>RX folder</button></a>");
  } else {
    client.println("<br><a href='?tx=0'><button style='margin:5px;'>TX folder</button></a>");
  }
  client.println("<div style='display: inline flow-root list-item;'>");

  Dir root = SDFS.openDir(folder);
  int num = count_bitmaps(root);
  root.rewind();
  int n = 0;
  int disp = 0;

  while ((root.next()) && (disp < 4)) {
    String name = root.fileName();
    ;
    if (isImage(name)) {
      if (n >= page * 4) {
        client.print("<div class='foto'>");
        client.print("<a href=?del=");
        client.print(folder);
        client.print("/");
        client.print(name);
        client.print("><button style='width:100%'>Delete ");
        client.print(name);
        client.print("</button></a></br>");
        client.print("<a href='img");
        client.print(folder);
        client.print("/");
        client.print(name);
        client.print("'><img src='img");
        client.print(folder);
        client.print("/");
        client.print(name);
        client.print("' /></a>");

        client.print("</div>");
        disp++;
      }
      n++;
    }
  }
  client.print("</div><hr>");

  for (int i = 0; i <= (num - 1) / 4; i++) {
    if (tx) client.println("<a href='/?tx=");
    else client.println("<a href='/?page=");
    client.print(i);
    client.println("'><button style='margin:5px;'>Page ");
    client.print(i);
    client.print("</button></a>");
  }

  if (tx) {
    client.println("<hr><form action='upload.php' method='post' enctype='multipart/form-data'>");
    client.println("<input type='file' name='fileToUpload' id='fileToUploa'>");
    client.println("<input type='submit' value='Upload Image' name='submit'></form>");
  }

  client.println("</body></html>");
}

void wifi_server::getImage(WiFiClient& client) {

  // Apri stream raw
  String boundary;
  String filename;

  // 1️⃣ Leggi header HTTP e trova boundary
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line.startsWith("Content-Type: multipart/form-data")) {
      int b = line.indexOf("boundary=");
      if (b > 0) {
        boundary = "--" + line.substring(b + 9);
        boundary.trim();
      }
    }
    if (line == "\r" || line.length() <= 1) break;
  }

  if (boundary == "") {
    sendError(client, 405, "Malformed request");
    return;
  }

  // 2️⃣ Leggi header multipart e trova filename
  while (client.connected()) {
    String line = client.readStringUntil('\n');

    if (line.startsWith("Content-Disposition")) {
      int f = line.indexOf("filename=\"");
      if (f >= 0) {
        filename = line.substring(f + 10);
        filename = filename.substring(0, filename.indexOf("\""));
      }
    }

    if (line == "\r" || line.length() <= 1) break;
  }

  if (filename == "") {
    sendError(client, 500, "File not found");
    return;
  }

  String path = "/tx/" + filename;
  FILE* file = fopen(path.c_str(), "wb");
  if (!file) {
    sendError(client, 501, "File not found");
    return;
  }

  // 3️⃣ Scrittura binaria fino al boundary finale
  /*uint8_t buffer[512];
  String tail = "\r\n" + boundary;
  int tailLen = tail.length();

  /*while (client.connected()) {
    int len = client.read(buffer, sizeof(buffer));
    if (len <= 0) break;

    // Controlla boundary finale
    String chunk = String((char*)buffer).substring(0, len);
    int pos = chunk.indexOf(tail);
    if (pos >= 0) {
      fwrite(buffer, 1, pos, file);
      break;
    } else {
      fwrite(buffer, 1, len, file);
    }
  }*/
 uint8_t buffer[512];
uint8_t prev[128];  // per gestire boundary spezzato
int prevLen = 0;

String tail = "\r\n" + boundary;
int tailLen = tail.length();

while (client.connected()) {
  int len = client.read(buffer, sizeof(buffer));
  if (len <= 0) break;

  // Combina prev + buffer
  uint8_t temp[640];
  memcpy(temp, prev, prevLen);
  memcpy(temp + prevLen, buffer, len);

  int totalLen = prevLen + len;

  // Cerca boundary nei dati binari
  int pos = -1;
  for (int i = 0; i <= totalLen - tailLen; i++) {
    if (memcmp(temp + i, tail.c_str(), tailLen) == 0) {
      pos = i;
      break;
    }
  }

  if (pos >= 0) {
    fwrite(temp, 1, pos, file);
    break;
  }

  // Scrivi tutto tranne gli ultimi byte (potrebbero contenere metà boundary)
  int safeLen = totalLen - tailLen;
  if (safeLen > 0) {
    fwrite(temp, 1, safeLen, file);

    // salva ultimi byte
    prevLen = tailLen;
    memcpy(prev, temp + safeLen, prevLen);
  } else {
    // troppo corto, accumula
    memcpy(prev + prevLen, buffer, len);
    prevLen += len;
  }
}


  fclose(file);
}

void wifi_server::poll_wifi_client() {
  static int page = 0;
  WiFiClient client = server.accept();

  if (!client) return;

  String request = client.readStringUntil('\r');
  client.readStringUntil('\n');

  if (request.startsWith("POST")) {
    getImage(client);
    sendGallery(client, 0, true);
    return;
  }

  request = request.substring(5, request.length() - 8);  //Remove "GET /" and "HTTP 1.1"

  if (request.startsWith("img/")) {
    int pos = request.indexOf('/');
    String filename = request.substring(4);
    filename.trim();
    Serial.println(filename);
    sendImage(client, filename);
  } else if (request.startsWith("?del=")) {
    String filename = request.substring(5);
    Serial.print("deleting ");
    Serial.println(filename);
    SDFS.remove(filename);
    sendGallery(client, page, false);
  } else if (request.startsWith("?page=")) {
    page = request.substring(6).toInt();
    sendGallery(client, page, false);
  } else if (request.startsWith("favicon.ico")) {
    sendError(client, 404, "Not found");
  } else if (request.startsWith("?tx=")) {
    page = request.substring(4).toInt();
    sendGallery(client, page, true);
  } else {
    sendGallery(client, page, false);
  }

  delay(1);
  client.flush();

  while (client.available()) {
    client.read();
  }
  delay(100);
  client.stop();
}

void wifi_server::connectToWiFi() {
  digitalWrite(23, HIGH);  // Turn on WiFi chip power
  delay(100);              // Wait for stabilization
  Serial.print("Connecting to WiFi");
  //WiFi.setTimeout(5000);
  WiFi.begin(ssid, password);
}

// Function to disconnect WiFi and turn off WiFi chip power
void wifi_server::disconnectWiFi() {
  Serial.println("Disconnecting WiFi...");
  WiFi.disconnect();      // Disconnect WiFi
  delay(100);             // Wait a bit
  WiFi.mode(WIFI_OFF);    // Turn off WiFi mode
  delay(100);             // Wait a bit
  digitalWrite(23, LOW);  // Turn off WiFi chip power
  Serial.println("WiFi disconnected and power to WiFi chip is off.");
}

// Function to reconnect WiFi and WiFiClient
void wifi_server::reconnectWiFiAndClient() {
  digitalWrite(23, HIGH);  // Turn on WiFi chip power
  delay(100);              // Wait for stabilization
  Serial.println("Reconnecting WiFi...");
  WiFi.mode(WIFI_STA);  // Set WiFi mode to STA
  connectToWiFi();
}  // Reconnect to WiFi