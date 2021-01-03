#ifndef _NETWORK_HELPER_H_
#define _NETWORK_HELPER_H_

#include <WiFiNINA.h>
#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoJson.h>

class NetworkHelper {
    public:
        NetworkHelper(const char*, const char*);

        static int freeMemory();
        static unsigned long getTimeCallback();
        static bool isWifiConnected();
        static bool isWifiTimeAvailable();
        static void connectWifi(const char*, const char*);
        static void printWifiStatus();
        static void checkWifiModule();
        static void checkWifiFirmware();

        BearSSLClient* getClient();
        void testBackend(const char*);
        bool connectBackend();
        bool getRequest(const char* path, DynamicJsonDocument* requestDoc, DynamicJsonDocument* responseDoc);
        bool postRequest(const char* path, DynamicJsonDocument* requestDoc, DynamicJsonDocument* responseDoc);
        bool deleteRequest(const char* path, DynamicJsonDocument* requestDoc, DynamicJsonDocument* responseDoc);
        void disconnectBackend();

    private:
        WiFiClient client;
        BearSSLClient sslClient; 
        const char* backend;
        const char* certificate;

        static unsigned long lastWifiConnectTime;  // the last time we tried to connect to wifi
        static unsigned long wifiConnectDelay;  // wait this long before trying to reconnect to wifi
        
        bool httpRequest(const char* method, const char* path, DynamicJsonDocument* requestDoc, DynamicJsonDocument* responseDoc);
};

#endif