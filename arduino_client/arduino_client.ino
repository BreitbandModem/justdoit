#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include <ezTime.h>
#include <Adafruit_NeoPixel.h>

#include "arduino_secrets.h" 


// Constants
const bool LOG_DEBUG   = true;
const byte LOG_INFO    = 0;
const byte LOG_WARNING = 1;
const byte LOG_ERROR   = 2;

const char MYISO8601[] = "Y-m-d~TH:i:sP";  // 2020-11-15T07:42:02+01:00

const byte BUTTON_PIN  = 2;
const byte PIR_PIN     = 3;
const byte PIXEL_PIN   = 6;
const byte LED_PIN     = 13;  // Arduino built-in LED
const int  PIXEL_COUNT = 60;  // Number of NeoPixels

const IPAddress backendAddress(192,168,0,248);
const int backendPort = 5555;

// Wifi variables
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int wifiStatus = WL_IDLE_STATUS;

// Timing variables
Timezone myTimezone;

// Pixel variables
Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Initialize the Wifi client library
WiFiClient client;

// Control flow variables
int currentButtonState;       // the actual current button state after debouncing
int lastButtonState = HIGH;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() {
  initLog();
  pinMode(LED_PIN, OUTPUT);  // init artuino LED
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // init button
  pinMode(PIR_PIN, INPUT);  // init PIR motion sensor
  initPixels();

  strip.setPixelColor(59, strip.Color(  127, 0,   0));
  strip.show();
  
  checkWifiModule();
  checkWifiFirmware();
  connectWifi();
  printWifiStatus();
  ezTimeSetup();
  // TODO: get habit history

  strip.setPixelColor(59, strip.Color(  0, 127,   0));
  strip.show();
}

void loop() {
  // ezTime event trigger
  events();

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
        habitDone();
      }
    }
  }

  lastButtonState = readButton;
}

void habitDone() {
  if(submitDate()) {
    strip.setPixelColor(0, strip.Color(  0, 70,   127));
    strip.show();
  } else {
    strip.setPixelColor(0, strip.Color(  127, 50,   50));
    strip.show();
  }
}

void getDoneHistory() {
//  curl -X GET -H "Content-Type: application/json" \
//    -d '{"startDate": "2020-11-15T10:14:43+01:00", "count": 60 }' \
//    http://localhost:5555/habit/meditation

  
}

// return true if date has been added successfully to backend
bool submitDate() {
  // allocate json object
  const size_t requestCapacity = JSON_ARRAY_SIZE(1) + 2*JSON_OBJECT_SIZE(1);
  DynamicJsonDocument requestDoc(requestCapacity);

  // populate json with current date
  JsonArray dates = requestDoc.createNestedArray("dates");
  JsonObject dates_0 = dates.createNestedObject();
  dates_0["date"] = myTimezone.dateTime(MYISO8601).c_str();

  const int bodyLength = measureJson(requestDoc);
  char body[256];
  serializeJson(requestDoc, body);
  Serial.println(body);

  // Allocate the JSON response document
  const size_t responseRequest = JSON_OBJECT_SIZE(1) + 10;
  DynamicJsonDocument responseDoc(responseRequest);

  if (httpRequest("POST", body, &responseDoc)) {
    if (responseDoc["added"] == 1 ) {
      Serial.print("Server successfully added date to backend.");
      return true;
    }
  }
  
  return false;
}

bool httpRequest(const char *method, char *body, DynamicJsonDocument * responseDoc) {
  // if there's a successful connection:
  if (client.connect(backendAddress, backendPort)) 
  {
    log("connecting to backend ... ");
  
    log("body length:");
    size_t bodyLength = strlen(body);
    Serial.println(bodyLength);
    log("body content:");
    Serial.println(body);

    client.print(method);
    client.println(" /habit/meditation HTTP/1.1"); 
    client.print("Host: ");
    client.println(backendAddress);
    client.println("Content-type: application/json");
    client.println("Accept: */*");
    client.println("Cache-Control: no-cache");
    client.println("Accept-Encoding: gzip, deflate");
    client.print("Content-Length: ");
    client.println(bodyLength);
    client.println("Connection: close");
    client.println();
    client.println(body);

    // Check HTTP status
    char status[32] = {0};
    client.readBytesUntil('\r', status, sizeof(status));
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
    if (!client.find(endOfHeaders)) 
    {
      Serial.println(F("Invalid response"));
      return false;
    }
    
    DeserializationError error = deserializeJson(*responseDoc, client);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return false;
    }

    return true;
    
  }
}

void initLog() {
  if(LOG_DEBUG) {
    Serial.begin(9600);
    /*while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }*/
  }
}

void initPixels() {
  strip.begin(); // Initialize NeoPixel strip object (REQUIRED)

  // strip.setPixelColor(0, strip.Color(  127, 0,   0));  //  Set pixel's color (in RAM)
  strip.setBrightness(10); // Set BRIGHTNESS (max = 255)

  strip.show();  // Initialize all pixels to [0]-red
}

void checkWifiModule() {
  if (WiFi.status() == WL_NO_MODULE) {
    log("Communication with WiFi module failed!", LOG_ERROR);
    // don't continue
    while (true);
  }
}

void checkWifiFirmware() {
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    log("Please upgrade the firmware", LOG_WARNING);
  }
}

void connectWifi() {
  while (wifiStatus != WL_CONNECTED) {
    log("Attempting to connect to SSID: ", LOG_WARNING);
    
    wifiStatus = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  log("Connected to wifi", LOG_INFO);
  printWifiStatus();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  log("SSID: ", LOG_INFO);
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  log("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  log("signal strength (RSSI) in dBm:");
  Serial.println(rssi);
}

void ezTimeSetup() {
  // enable ezTime debug info
  setDebug(INFO);
  
  // sync with NTP server
  waitForSync();

  myTimezone.setLocation(F("de"));
  
  log("ezTime sync finished.");
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


void log(const char *logmessage) {
  log(logmessage, LOG_INFO);
}

void log(const char *logmessage, byte severity) {
  if(LOG_DEBUG) {
    Serial.print(myTimezone.dateTime(ISO8601));

    switch(severity) {
      case LOG_INFO:
        Serial.print(" - INFO  - ");
        break;
      case LOG_WARNING:
        Serial.print(" - WARN  - ");
        break;
      case LOG_ERROR:
        Serial.print(" - ERROR - ");
        strip.setPixelColor(59, strip.Color(  127, 0,   0));
        strip.show();
        break;
      default:
        Serial.print(" - ");
        break;
    }

    Serial.print(logmessage);
    Serial.print("\n");
    
  }
}
