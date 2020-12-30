#include "NetworkHelper.h"

NetworkHelper::NetworkHelper(const char* _backend, const char* _certificate)
    : backend(_backend),
      certificate(_certificate),
      client(),
      sslClient(client) {
        sslClient.setEccSlot(0, certificate);
}

unsigned long NetworkHelper::lastWifiConnectTime = 0;
unsigned long NetworkHelper::wifiConnectDelay = 10000;

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__
 
int NetworkHelper::freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

static unsigned long NetworkHelper::getTimeCallback() {
    Serial.print("BearSSL get time from wifi: ");
    Serial.println(WiFi.getTime());
    return WiFi.getTime();
}

BearSSLClient* NetworkHelper::getClient() {
    return &sslClient;
}

static bool NetworkHelper::isWifiConnected() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }
    return false;
}

static bool NetworkHelper::isWifiTimeAvailable() {
    if (WiFi.getTime() > 0 ) {
        return true;
    }
    return false;
}

static void NetworkHelper::connectWifi(const char* ssid, const char* pass) {
    if ( (millis() - wifiConnectDelay) > lastWifiConnectTime) {
        int wifiStatus = WiFi.status();
        lastWifiConnectTime = millis();
        
        if(wifiStatus != WL_CONNECTED) {
            Serial.println("Not connected to Wifi. Attempting to connect...");
        
            wifiStatus = WiFi.begin(ssid, pass);
        
            if (wifiStatus == WL_CONNECTED) {
                Serial.println("Successfully connected to wifi");
                printWifiStatus();
            }
        }
    }
}

static void NetworkHelper::checkWifiFirmware() {
    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
        Serial.println("Please upgrade the firmware");
    }
}

static void NetworkHelper::checkWifiModule() {
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        // don't continue
        while (true);
    }
}

static void NetworkHelper::printWifiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI) in dBm:");
    Serial.println(rssi);
}

bool NetworkHelper::connectBackend() {
    Serial.println("Connecting to Backend.");

    ArduinoBearSSL.onGetTime(&NetworkHelper::getTimeCallback);
    bool connected = sslClient.connect(backend, 443);

    Serial.print("Connected: ");
    Serial.println(connected);

    return connected;
}

bool NetworkHelper::getRequest(DynamicJsonDocument* requestDoc, DynamicJsonDocument* responseDoc) {
    return httpRequest("GET", requestDoc, responseDoc);
}

bool NetworkHelper::postRequest(DynamicJsonDocument* requestDoc, DynamicJsonDocument* responseDoc) {
    return httpRequest("POST", requestDoc, responseDoc);
}

bool NetworkHelper::deleteRequest(DynamicJsonDocument* requestDoc, DynamicJsonDocument* responseDoc) {
    return httpRequest("DELETE", requestDoc, responseDoc);
}

bool NetworkHelper::httpRequest(const char* method, DynamicJsonDocument* requestDoc, DynamicJsonDocument* responseDoc) {
    if(sslClient.connected()) {
        Serial.println("Submitting http request to backend.");

        sslClient.print(method);
        sslClient.println(" /habit/meditation HTTP/1.1");
        sslClient.print("Host: ");
        sslClient.println(backend);
        sslClient.println("Content-type: application/json");
        sslClient.println("Accept: */*");
        sslClient.println("Cache-Control: no-cache");
        sslClient.println("Accept-Encoding: gzip, deflate");
        sslClient.print("Content-Length: ");
        sslClient.println(measureJson(*requestDoc));

        // Terminate header with blank line
        sslClient.println();

        // Send JSON document in body
        serializeJson(*requestDoc, Serial);
        serializeJson(*requestDoc, sslClient);

        // Catch empty lines
        while(sslClient.available()) {
            Serial.write(sslClient.read());
        }

        // Check HTTP status
        char status[32] = {0};
        sslClient.readBytesUntil('\r', status, sizeof(status));
        Serial.println(status);
        // It should be "HTTP/1.0 200 OK" or "HTTP/1.1 200 CREATED"
        if ( strcmp(status + 9, "200 OK") != 0 && strcmp(status + 9, "201 CREATED") != 0 ) 
        {
            Serial.print(F("Unexpected response: "));
            Serial.println(status);
            return false;
        }

        // Skip HTTP headers
        char endOfHeaders[] = "\r\n\r\n";
        if (!sslClient.find(endOfHeaders)) 
        {
            Serial.println(F("Invalid response"));
            return false;
        }

        DeserializationError error = deserializeJson(*responseDoc, sslClient);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            return false;
        }

        return true;
    } else {
        Serial.println("Backend is not connected. Connect before making a request!");
    }
}

void NetworkHelper::disconnectBackend() {
    // Close connection
    sslClient.println("Connection: close");
    sslClient.println();

    // Catch remaining output
    while(sslClient.available()) {
        Serial.write(sslClient.read());
    }

    // Disconnect client
    if (sslClient.connected()) {
        sslClient.stop();
    }
}

void NetworkHelper::testBackend(const char *logmessage) {
  // connect
  Serial.print("\nStarting connection to server ");
  Serial.println(logmessage);
  Serial.print("Free memory: ");
  Serial.println(NetworkHelper::freeMemory());

  ArduinoBearSSL.onGetTime(&NetworkHelper::getTimeCallback);

  // if you get a connection, report back via serial:
  if (sslClient.connect(backend, 443)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    sslClient.println("GET /habit/meditation HTTP/1.1");
    sslClient.print("Host: ");
    sslClient.println(backend);
    sslClient.println("Connection: close");
    sslClient.println();
  } else {
    Serial.println("Failed to connect.");
    sslClient.stop();
  }
  
  Serial.print("available: ");
  Serial.println(sslClient.available());
  Serial.print("connected: ");
  Serial.println(sslClient.connected());

  while(sslClient.connected()) {
    if(sslClient.available()) {
      while(sslClient.available()) {
        Serial.write(sslClient.read());
      }
      sslClient.stop();
      break;
    }
  }
}