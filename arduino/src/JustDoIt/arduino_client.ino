#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include <ezTime.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>

#include "arduino_secrets.h" 


// Constants
const bool LOG_DEBUG   = false;
const byte LOG_INFO    = 0;
const byte LOG_WARNING = 1;
const byte LOG_ERROR   = 2;

const char MYISO8601[] = "Y-m-d~TH:i:sP";  // 2020-11-15T07:42:02+01:00

const byte BUTTON_PIN  = 2;
const byte PIR_PIN     = 3;
const byte PIXEL_PIN   = 6;
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
Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

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


struct PixelData {
   char date[26];
   bool done;
   bool syncme;
};

struct PixelData pixelHistory[PIXEL_COUNT];

/*
* Function prototypes
*/
void setup(void);
void loop(void);
bool syncUp(void);

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__
 
int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

unsigned long getTimeBearSSL() {
  return WiFi.getTime();
}

void setup() {
  initLog();
  pinMode(LED_PIN, OUTPUT);  // init artuino LED
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // init button
  pinMode(PIR_PIN, INPUT);  // init PIR motion sensor
  initPixels();
  
  checkWifiModule();
  checkWifiFirmware();
  
  sslClient.setEccSlot(0, certificate);

  Serial.print("Free memory: ");
  Serial.println(freeMemory());

  // ezTime
  setDebug(INFO);
}

void loop() {
  visualizeDoneHistory();

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
        buttonPress();
      }
    }
  }

  lastButtonState = readButton;
}

// Triggered every day by the ezTime events()
void everyDay() {
  Serial.println("Next Day. Shifting pixelHistory...");

  shiftPixelHistory();
  visualizeDoneHistory();

  // register next event
  setEvent( everyDay, nextDay() );
}

// Triggered every 5 Minutes by the ezTime events()
void fullSync() {
  Serial.println("Syncing to backend...");

  Serial.print("Free memory: ");
  Serial.println(freeMemory());


  // First, sync pending changes to backend. Only then download (overwrite) local status by remote.
  bool syncedUp = syncUp();

  Serial.print("Free memory: ");
  Serial.println(freeMemory());
  
  if( syncedUp ) {
    syncDown();
    visualizeDoneHistory();
  }

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

void shiftPixelHistory() {

  // Shift all pixels to the 'right' (last pixel will be dropped)
  for (int i=PIXEL_COUNT-1; i>0; i--) {
    pixelHistory[i] = pixelHistory[i-1];
  }

  // Set todays pixel
  strncpy(pixelHistory[0].date, myTimezone.dateTime(MYISO8601).c_str(), sizeof pixelHistory[0].date);
  pixelHistory[0].done = false;
  pixelHistory[0].syncme = false;

}

bool syncDown() {
  if (sslClient.connect(backend, 443)) {
    Serial.println("connected to server");

    DynamicJsonDocument requestDoc(32);
  
    DynamicJsonDocument responseDoc(128);

    for(int i=0; i<PIXEL_COUNT; i++) {
      
      requestDoc["startDate"] = String(pixelHistory[0].date).c_str();
      requestDoc["count"] = i;
      
      char body[64];
      serializeJson(requestDoc, body);
      size_t bodyLength = strlen(body);

      sslClient.println("GET /habit/meditation HTTP/1.1");
      sslClient.print("Host: ");
      sslClient.println(backend);
      sslClient.println("Content-type: application/json");
      sslClient.println("Accept: */*");
      sslClient.println("Cache-Control: no-cache");
      sslClient.println("Accept-Encoding: gzip, deflate");
      sslClient.print("Content-Length: ");
      sslClient.println(bodyLength);
      sslClient.println();
      sslClient.println(body);

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
      
      DeserializationError error = deserializeJson(responseDoc, sslClient);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return false;
      }

      const char* date = responseDoc["history"][0]["date"];
      int done = responseDoc["history"][0]["done"];

      strncpy(pixelHistory[i].date, date, strlen(date));
      if (done == 1) {
        pixelHistory[i].done = true;
      } else {
        pixelHistory[i].done = false;
      }
      pixelHistory[i].syncme = false;
    }

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

    return true;
  }
}

bool syncUp() {
  Serial.println("Sync Up ..");
  Serial.print("Free memory: ");
  Serial.println(freeMemory());

  if (sslClient.connect(backend, 443)) {
    Serial.println("connected to server");
    int successCount = 0;

    DynamicJsonDocument requestDoc(48);
    DynamicJsonDocument responseDoc(32);

    for(int i=0; i<PIXEL_COUNT; i++) {

      if( pixelHistory[i].syncme ) {

        successCount += 1;

        Serial.println("Date: ");
        Serial.println(pixelHistory[i].date);
        
        requestDoc["dates"][0]["date"] = String(pixelHistory[i].date).c_str();
        
        char body[96];
        serializeJson(requestDoc, body);
        size_t bodyLength = strlen(body);
        
        Serial.println(body);

        if ( pixelHistory[i].done ) {
          sslClient.print("POST");
        } else {
          sslClient.print("DELETE");
        }

        sslClient.println(" /habit/meditation HTTP/1.1");
        sslClient.print("Host: ");
        sslClient.println(backend);
        sslClient.println("Content-type: application/json");
        sslClient.println("Accept: */*");
        sslClient.println("Cache-Control: no-cache");
        sslClient.println("Accept-Encoding: gzip, deflate");
        sslClient.print("Content-Length: ");
        sslClient.println(bodyLength);
        sslClient.println();
        sslClient.println(body);
  
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
        
        DeserializationError error = deserializeJson(responseDoc, sslClient);
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
          return false;
        }

        if ( pixelHistory[i].done ) {
          if (responseDoc["added"] >= 0 ) {
            Serial.print("Server successfully added dates to backend.");
            pixelHistory[i].syncme = false;
            setPixelDone(i);
            strip.show();
            successCount -= 1;
          }
        } else {
          if (responseDoc["deleted"] >= 0 ) {
            Serial.print("Server successfully deleted dates from backend.");
            pixelHistory[i].syncme = false;
            setPixelUndone(i);
            strip.show();
            successCount -= 1;
          }
        }
      }
    }

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

    return (successCount == 0);
  }

  Serial.println("Failed to connect to server");
  
  return false;
}

void visualizeDoneHistory() {
  for (int i=0; i<PIXEL_COUNT; i++) {
//    Serial.print("Pixel #");
//    Serial.print(i);
//    Serial.print("  date: ");
//    Serial.print(pixelHistory[i].date);
//    Serial.print(" done: ");
//    Serial.println(pixelHistory[i].done);

    if ( i == 0 && ! pixelHistory[i].done) {
      setPixelTodo(i);
    } else if ( pixelHistory[i].syncme ) {
      setPixelPending(i);
    } else if ( pixelHistory[i].done ){
      setPixelDone(i);
    } else {
      setPixelUndone(i);
    }
  }
  strip.show();
}

// when button is pressed, update latest day in history and mark for sync
void buttonPress() {
  
  strncpy(pixelHistory[0].date, myTimezone.dateTime(MYISO8601).c_str(), sizeof pixelHistory[0].date);
  pixelHistory[0].done = !pixelHistory[0].done;
  pixelHistory[0].syncme = true;

  // sync pending -> green
  setPixelPending(0);
  strip.show();

  delay(1000);
  syncUp();
}

int translatePixelLocation(int index) {
  // fill pixel ring backwards except for current day, which is on first pixel (0).
  // 0 ->  0
  // 1 -> 59
  // 2 -> 58
  // 3 -> 57
  
  int pixelIndex = 0;

  int arrayIndex = index % PIXEL_COUNT;
  if (arrayIndex != 0) {
    pixelIndex = PIXEL_COUNT - arrayIndex;
  }

  return pixelIndex;
}

void setPixelPending(int arrayIndex) {
  int pixelIndex = translatePixelLocation(arrayIndex);

  if( switchOn) {
    strip.setPixelColor(pixelIndex, strip.Color(  127, 127,   0));  // yellow
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}

void setPixelDone(int arrayIndex) {
  int pixelIndex = translatePixelLocation(arrayIndex);
  
  if( switchOn) {
    float green = (float)pixelIndex / PIXEL_COUNT * 70.0 + 7.0;
    float blue = (float)pixelIndex / PIXEL_COUNT * 127.0 + 12.7;
    
    if ( pixelIndex == 0 ) {
      green = 70.0;
      blue = 127.0;
    }

    strip.setPixelColor(pixelIndex, strip.Color(  0, ceil(green),   ceil(blue)));  // blueish
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}

void setPixelUndone(int arrayIndex) {
  int pixelIndex = translatePixelLocation(arrayIndex);
  
  if( switchOn) {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}

void setPixelTodo(int arrayIndex) {
  int pixelIndex = translatePixelLocation(arrayIndex);
  
  if( switchOn) {
    strip.setPixelColor(pixelIndex, strip.Color(  230, 40,   0));  // redish
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}

void setPixelLoading(int arrayIndex) {    
  int pixelIndex = translatePixelLocation(arrayIndex);
  
  if( switchOn) {
    strip.setPixelColor(pixelIndex, strip.Color(  127, 0,   0));  // red
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}

void initLog() {
  Serial.begin(9600);
  if(LOG_DEBUG) {
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }
  }
  Serial.println("Hello Serial");
}

void initPixels() {
  strip.begin(); // Initialize NeoPixel strip object (REQUIRED)

  // strip.setPixelColor(0, strip.Color(  127, 0,   0));  //  Set pixel's color (in RAM)
  strip.setBrightness(BRIGHTNESS); // Set BRIGHTNESS (max = 255)

  strip.show();  // Initialize all pixels to [0]-red
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

void testBackend(const char *logmessage) {
  // connect
  Serial.print("\nStarting connection to server ");
  Serial.println(logmessage);

  // TODO: remove these two lines
  ArduinoBearSSL.onGetTime(getTimeBearSSL);
  sslClient.setEccSlot(0, certificate);
  
  // if you get a connection, report back via serial:
  if (sslClient.connect(backend, 443)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    sslClient.println("GET /habits HTTP/1.1");
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

// Block loop while time is not set
void waitForTimeSync() {
  if (timeStatus() == timeSet && WiFi.getTime() > 0) {
    return;
  }
  
  // run a red pixel around the ring
  int loadingPixel = 0;

  while (WiFi.getTime() <= 0 ) {
    Serial.println("Waiting for wifi time");
    setPixelLoading(loadingPixel);
    setPixelUndone(loadingPixel -1);
    loadingPixel += 1;
    strip.show();
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
    setPixelLoading(loadingPixel);
    setPixelUndone(loadingPixel -1);
    loadingPixel += 1;
    strip.show();
    delay(5000);
  }

  // ezTime events setup
  deleteEvent( fullSync );
  deleteEvent( everyDay );
  // setEvent( fullSync, nextFiveMinutes() );
  setEvent( everyDay, nextDay() );

  // init today
  strncpy(pixelHistory[0].date, myTimezone.dateTime(MYISO8601).c_str(), sizeof pixelHistory[0].date);
  pixelHistory[0].done = false;
  pixelHistory[0].syncme = false;

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

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 3 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 3*65536. Adding 256 to firstPixelHue each time
  // means we'll make 3*65536/256 = 768 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 3*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}
