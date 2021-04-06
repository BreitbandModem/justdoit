#include "JustDoIt.h"
#include "arduino_secrets.h" 
#include "Strip.h"
#include "It.h"
#include "Timing.h"

#include <Wire.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <ezTime.h>
#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <NetworkHelper.h>

const byte BUTTON_PIN  = 2;
const byte PIR_PIN     = 3;
const int PIXEL_PIN   = 6;
const byte LED_PIN     = 13;  // Arduino built-in LED
const int  PIXEL_COUNT = 60;  // Number of NeoPixels
const int  BRIGHTNESS  = 255;
const bool WAIT_FOR_SERIAL = false;
const int QUIET_HOUR_START = 21;
const int QUIET_HOUR_END = 8;
const int QUIET_HOUR_PAUSE = 1;
const int SYNC_INTERVAL = 15;

// Supply backend address and certificate via secrets file
NetworkHelper networkHelper(BACKEND_ADDRESS, CERTIFICATE);

// Pixel variables
Strip strip(PIXEL_COUNT, PIXEL_PIN, BRIGHTNESS);

// Control flow variables
int currentButtonState;       // the actual current button state after debouncing
int lastButtonState = HIGH;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

unsigned long lastPirTime = 0;  // the last time the PIR sensor was triggered by movement
unsigned long pirDelay = 30000;  // Turn on LEDs for this long after PIR Sensor was triggered

/*
* Function prototypes
*/
void setup(void);
void loop(void);
void everyDay();
void fullSync();
void quietHour(bool);
void initLog();
void requireLoop();

void setup() {
  initLog();
  pinMode(LED_PIN, OUTPUT);  // init artuino LED
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // init button
  pinMode(PIR_PIN, INPUT);  // init PIR motion sensor
  
  NetworkHelper::checkWifiModule();
  NetworkHelper::checkWifiFirmware();

  Serial.print("Free memory: ");
  Serial.println(NetworkHelper::freeMemory());
}

void loop() {
  strip.visualize();

  // while no time is available, this method will try to reconnect to NTP
  requireLoop();

  // ezTime event trigger
  Timing::callEvents();

  int readPir = digitalRead(PIR_PIN);
  if (readPir == HIGH) {
    lastPirTime = millis();
    strip.setAwake(true);
  }

  if ((millis() - lastPirTime) > pirDelay) {
    strip.setAwake(false);
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
        // get quiet hours...
        if(strip.getQuietHours()) {
          Timing::pauseQuietHour(QUIET_HOUR_PAUSE);
        } else {
          strip.done(0, Timing::getDate(), &networkHelper);
        }
      }
    }
  }

  lastButtonState = readButton;
}

// Triggered every day by the ezTime events()
void everyDay() {
  Serial.println("Next Day. Shifting pixelHistory...");

  strip.newDay(Timing::getDate());
  strip.visualize();
}

// Triggered every 5 Minutes by the ezTime events()
void fullSync() {
  Serial.println("Syncing to backend...");

  Serial.print("Free memory: ");
  Serial.println(NetworkHelper::freeMemory());

  strip.sync(&networkHelper);
}

void quietHour(bool isQuietHour) {
    Serial.print("Quiet Hour is active: ");
    Serial.println(isQuietHour);

    strip.setQuietHours(isQuietHour);
}

void initLog() {
  Serial.begin(9600);
  if(WAIT_FOR_SERIAL) {
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }
  }
  Serial.println("Hello Serial");
}

void requireLoop() {
  bool requirementMissing = false;
  // wait for wifi connection
  while(! NetworkHelper::isWifiConnected()) {
    requirementMissing = true;
    strip.advanceLoadingAnimation();
    NetworkHelper::connectWifi(SECRET_SSID, SECRET_PASS);
    delay(5000);
  }

  // wait for wifi time 
  while(! NetworkHelper::isWifiTimeAvailable()) {
    requirementMissing = true;
    strip.advanceLoadingAnimation();
    delay(500);
  }

  // wait for eztime sync
  while(!Timing::isSynced()) {
      requirementMissing = true;
      strip.advanceLoadingAnimation();

      Timing::syncTime();

      delay(5000);
  }

  if (requirementMissing) {
    // Verify backend
    Serial.print("Free memory: ");
    Serial.println(NetworkHelper::freeMemory());
    // networkHelper.testBackend("test backend #1");

    // init strip by setting first day and syncing with backend
    strip.newDay(Timing::getDate());
    fullSync();

    // setup timing event callbacks
    Timing::onInterval(SYNC_INTERVAL, fullSync);
    Timing::onNextDay(everyDay);
    Timing::onQuietHour(QUIET_HOUR_START, QUIET_HOUR_END, quietHour);
  }
}