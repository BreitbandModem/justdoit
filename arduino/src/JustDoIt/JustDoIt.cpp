#include "JustDoIt.h"
#include "arduino_secrets.h" 
#include "Strip.h"
#include "It.h"
#include "Timing.h"

#include <Wire.h>
#include <SPI.h>
#include <ezTime.h>
#include <WiFiNINA.h>
#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <NetworkHelper.h>

const byte BUTTON_PIN  = 2;
const byte PIR_PIN     = 3;
const int PIXEL_PIN   = 6;
const byte LED_PIN     = 13;  // Arduino built-in LED
const int  PIXEL_COUNT = 60;  // Number of NeoPixels
const int  BRIGHTNESS  = 255;
const bool WAIT_FOR_SERIAL = true;
const int QUIET_HOUR_START = 21;
const int QUIET_HOUR_END = 7;

// Supply backend address and certificate via secrets file
NetworkHelper networkHelper(BACKEND_ADDRESS, CERTIFICATE);

// Timing variables
Timezone myTimezone;
Timing timeHelper;

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
time_t nextFiveMinutes();
time_t nextDay();
void quietHour();
void initLog();
void requireLoop();

void setup() {
  initLog();
  pinMode(LED_PIN, OUTPUT);  // init artuino LED
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // init button
  pinMode(PIR_PIN, INPUT);  // init PIR motion sensor
  
  NetworkHelper::checkWifiModule();
  NetworkHelper::checkWifiFirmware();

  // ezTime
  setDebug(INFO);

  Serial.print("Free memory: ");
  Serial.println(NetworkHelper::freeMemory());
}

void loop() {
  strip.visualize();

  // while no time is available, this method will try to reconnect to NTP
  requireLoop();

  // ezTime event trigger
  events();

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
          // set strip active
          strip.setAwake(true);
          strip.setQuietHours(false);

          // set event to recalculate quiet hours in 1min
          tmElements_t tm;
          breakTime(UTC.now(), tm);
          tm.Minute = tm.Minute + 1;
          deleteEvent(quietHour);
          setEvent(quietHour, makeTime(tm));
        } else {
          strip.done(0, timeHelper.getDate(), &networkHelper);
        }
      }
    }
  }

  lastButtonState = readButton;
}

// Triggered every day by the ezTime events()
void everyDay() {
  Serial.println("Next Day. Shifting pixelHistory...");

  strip.newDay(timeHelper.getDate());
  strip.visualize();

  // register next event
  setEvent( everyDay, nextDay() );
}

// Triggered every 5 Minutes by the ezTime events()
void fullSync() {
  Serial.println("Syncing to backend...");

  Serial.print("Free memory: ");
  Serial.println(NetworkHelper::freeMemory());

  strip.sync(&networkHelper);

  // register next event
  setEvent( fullSync, nextFiveMinutes() );
}

void quietHour() {
  Serial.println("Toggle quiet hour");

  tmElements_t tm;
  breakTime(myTimezone.now(), tm);

  if (tm.Hour >= QUIET_HOUR_START || tm.Hour < QUIET_HOUR_END) {
    // We're in quiet hours -> next event at quiet hour end.
    tm.Day = tm.Day + 1;
    tm.Hour = QUIET_HOUR_END;

    strip.setQuietHours(true);
  } else {
    // We're outside of quiet hours -> next event at quiet hour start.
    tm.Hour = QUIET_HOUR_START;

    strip.setQuietHours(false);
  }

  time_t nextEventUTC = myTimezone.tzTime(makeTime(tm));

  setEvent( quietHour, nextEventUTC );
}

// Calculate timestamp five minutes from now
time_t nextFiveMinutes() {
  Serial.println("Calculating next 5 Minutes...");

  // TODO: change from UTC to timezone
  tmElements_t tm;
  breakTime(UTC.now(), tm);

  // Debug: Change interval to every 10 seconds
  // tm.Second = tm.Second + 10;
  tm.Minute = tm.Minute + 15;
  
  return makeTime(tm);
}

// Calculate timestamp for next day 3:00 am
time_t nextDay() {
  Serial.println("Calculating next Day...");

  // TODO: change from UTC to timezone
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
  while(!timeHelper.isSynced()) {
      requirementMissing = true;
      strip.advanceLoadingAnimation();

      timeHelper.syncTime();

      delay(5000);
  }

  if (requirementMissing) {
    // Verify backend
    Serial.print("Free memory: ");
    Serial.println(NetworkHelper::freeMemory());
    // networkHelper.testBackend("test backend #1");

    // init sync and events
    strip.newDay(timeHelper.getDate());

    // delete existing ezTime events
    deleteEvent( fullSync );
    deleteEvent( everyDay );
    deleteEvent( quietHour );
    // setup events from scratch
    setEvent( everyDay, nextDay() );
    fullSync(); // calling the event will schedule the next event
    quietHour(); // calling the event will schedule the next event
  }
}