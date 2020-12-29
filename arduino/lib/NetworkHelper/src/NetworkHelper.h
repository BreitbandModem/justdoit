#ifndef _NETWORK_HELPER_H_
#define _NETWORK_HELPER_H_

#include <WiFiNINA.h>
#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>

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

        WiFiClient client;
        BearSSLClient sslClient; 
        const char* backend;
        const char* certificate;

        BearSSLClient* getClient();
        void testBackend(const char*);

    private:
        static unsigned long lastWifiConnectTime;  // the last time we tried to connect to wifi
        static unsigned long wifiConnectDelay;  // wait this long before trying to reconnect to wifi
        
        static const char* ssid;
        static const char* pass;
};

#endif