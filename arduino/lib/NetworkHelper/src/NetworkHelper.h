#ifndef _NETWORK_HELPER_H_
#define _NETWORK_HELPER_H_

#include <WiFiNINA.h>
#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>

class NetworkHelper {
    public:
        NetworkHelper(const char*, const char*, const char*, const char*);

        static unsigned long getTimeCallback();

        WiFiClient client;
        BearSSLClient sslClient; 
        const char* backend;
        const char* certificate;

        BearSSLClient* getClient();
        bool isWifiConnected();
        bool isWifiTimeAvailable();
        void connectWifi();
        void printWifiStatus();
        void checkWifiModule();
        void checkWifiFirmware();
        void testBackend(const char*);
        
        static int freeMemory();

    private:
        const char* ssid;
        const char* pass;

        unsigned long lastWifiConnectTime = 0;  // the last time we tried to connect to wifi
        unsigned long wifiConnectDelay = 10000;  // wait this long before trying to reconnect to wifi
};

#endif