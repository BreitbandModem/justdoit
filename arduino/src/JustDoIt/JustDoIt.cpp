#include "JustDoIt.h"
#include "arduino_secrets.h" 
#include "Strip.h"
#include "It.h"

#include <Wire.h>
#include <SPI.h>
#include <WiFiNINA.h>
// #include <ArduinoJson.h>
#include <ezTime.h>
// #include <Adafruit_NeoPixel.h>
#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>

const byte BUTTON_PIN  = 2;
const byte PIR_PIN     = 3;
const int PIXEL_PIN   = 6;
const byte LED_PIN     = 13;  // Arduino built-in LED
const int  PIXEL_COUNT = 60;  // Number of NeoPixels
const int  BRIGHTNESS  = 10;

// Supply backend address and certificate via secrets file
// Wifi variables
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
const char* certificate = CERTIFICATE;
const char backend[] = BACKEND_ADDRESS;
int wifiStatus = WL_IDLE_STATUS;

// Timing variables
Timezone myTimezone;

// Pixel variables
Strip strip(PIXEL_COUNT, PIXEL_PIN, BRIGHTNESS);

// Initialize the Wifi client library
WiFiClient client;
BearSSLClient sslClient(client);

// Control flow variables
int currentButtonState;       // the actual current button state after debouncing
int lastButtonState = HIGH;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

unsigned long lastPirTime = 0;  // the last time the PIR sensor was triggered by movement
unsigned long pirDelay = 30000;  // Turn on LEDs for this long after PIR Sensor was triggered
bool switchOn = true;  // whether to turn on or off the LEDs

unsigned long lastWifiConnectTime = 0;  // the last time we tried to connect to wifi
unsigned long wifiConnectDelay = 10000;  // wait this long before trying to reconnect to wifi

// struct PixelData pixelHistory[PIXEL_COUNT];
// It stripData[PIXEL_COUNT];

/*
* Function prototypes
*/
void setup(void);
void loop(void);
unsigned long getTimeBearSSL();
void everyDay();
void fullSync();
time_t nextFiveMinutes();
time_t nextDay();
void initLog();
void checkWifiModule();
void checkWifiFirmware();
void waitForTimeSync();
void connectWifi();
void printWifiStatus();

unsigned long getTimeBearSSL() {
  return WiFi.getTime();
}

void setup() {
  initLog();
  pinMode(LED_PIN, OUTPUT);  // init artuino LED
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // init button
  pinMode(PIR_PIN, INPUT);  // init PIR motion sensor
  
  checkWifiModule();
  checkWifiFirmware();
  
  sslClient.setEccSlot(0, certificate);

  // ezTime
  setDebug(INFO);
  Serial.print("Free memory: ");
  Serial.println(Strip::freeMemory());
}

void loop() {
  strip.visualize();

  // if not connected wifi, try to reconnect
  connectWifi();

  // while no time is available, this method will try to reconnect to NTP
  waitForTimeSync();

  // ezTime event trigger
  events();

  int readPir = digitalRead(PIR_PIN);
  if (readPir == HIGH) {
    lastPirTime = millis();
    switchOn = true;
  }

  if ((millis() - lastPirTime) > pirDelay) {
    switchOn = false;
  }

  int readButton = digitalRead(BUTTON_PIN);
  if (readButton != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  // signal is present > debounce time
  if ((millis() - lastDebounceTime) > debounceDelay) {

    // update current button state
    if (readButton != currentButtonState) {
      currentButtonState = readButton;

      // Button has been pressed
      if (currentButtonState == LOW) {
        Serial.println("Button pressed.");
        strip.done(0, myTimezone.dateTime(It::MYISO8601), backend, &sslClient);
      }
    }
  }

  lastButtonState = readButton;
}

// Triggered every day by the ezTime events()
void everyDay() {
  Serial.println("Next Day. Shifting pixelHistory...");

  strip.newDay(myTimezone.dateTime(It::MYISO8601));
  strip.visualize();

  // register next event
  setEvent( everyDay, nextDay() );
}

// Triggered every 5 Minutes by the ezTime events()
void fullSync() {
  Serial.println("Syncing to backend...");

  Serial.print("Free memory: ");
  Serial.println(Strip::freeMemory());

  strip.sync(backend, &sslClient);

  // register next event
  setEvent( fullSync, nextFiveMinutes() );
}

// Calculate timestamp five minutes from now
time_t nextFiveMinutes() {
  Serial.println("Calculating next 5 Minutes...");

  tmElements_t tm;
  breakTime(UTC.now(), tm);

  // Debug: Change interval to every 10 seconds
  // tm.Second = tm.Second + 10;
  tm.Minute = tm.Minute + 5;
  
  return makeTime(tm);
}

// Calculate timestamp for next day 3:00 am
time_t nextDay() {
  Serial.print("Calculating next Day...");

  tmElements_t tm;
  breakTime(UTC.now(), tm);

  // Run at 03:00 on the next day
  tm.Day = tm.Day + 1;
  tm.Hour = 3;
  tm.Minute = 0;
  tm.Second = 0;
  
  return makeTime(tm);
}

void initLog() {
  Serial.begin(9600);
  if(true) {
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }
  }
  Serial.println("Hello Serial");
}

void checkWifiModule() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
}

void checkWifiFirmware() {
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
}

// Block loop while time is not set
void waitForTimeSync() {
  if (timeStatus() == timeSet && WiFi.getTime() > 0) {
    return;
  }

  Serial.print("Free memory: ");
  Serial.println(Strip::freeMemory());
  
  while (WiFi.getTime() <= 0 ) {
    Serial.println("Waiting for wifi time");
    strip.advanceLoadingAnimation();
    delay(500);
  }

  ArduinoBearSSL.onGetTime(getTimeBearSSL);
  
  while (timeStatus() == timeNotSet) {
    if (wifiStatus != WL_CONNECTED) {
      wifiStatus = WiFi.begin(ssid, pass);
    } else {
      waitForSync(10);
      myTimezone.setLocation(F("de"));
  
      // if ez time eventually synced
      // then setup events and sync with backend
      if(timeStatus() == timeSet) {
        break;
      }
    }
    strip.advanceLoadingAnimation();
    delay(5000);
  }

  Serial.print("Free memory: ");
  Serial.println(Strip::freeMemory());

  strip.newDay(myTimezone.dateTime(It::MYISO8601));

  Serial.print("Free memory: ");
  Serial.println(Strip::freeMemory());

  // ezTime events setup
  deleteEvent( fullSync );
  deleteEvent( everyDay );
  setEvent( everyDay, nextDay() );
  fullSync();
}

void connectWifi() {
  if ( (millis() - wifiConnectDelay) > lastWifiConnectTime) {
    wifiStatus = WiFi.status();
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

void printWifiStatus() {
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