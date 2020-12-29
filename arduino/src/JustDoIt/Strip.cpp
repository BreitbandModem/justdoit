#include "Strip.h"

#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <math.h>

Strip::Strip(int _pixelCount, int pixelPin, int brightness) 
    : pixelCount(_pixelCount),
      loadingAnimationPixel{0},
      awake{true},
      strip(_pixelCount, pixelPin, NEO_GRB + NEO_KHZ800) {

        strip.begin();
        strip.setBrightness(brightness);
        strip.show();

        data = new It[pixelCount];
}

void Strip::setAwake(bool a) {
    awake = a;
}

void Strip::show() {
    strip.show();
}

void Strip::done(int index, String date, NetworkHelper* networkHelper) {
    data[index].setDate(date);
    data[index].setDone(! data[index].isDone());
    data[index].setSynced(false);

    // sync pending -> green
    setPixelPending(index);
    strip.show();

    syncUp(networkHelper);
}

void Strip::newDay(String date) {
    // Free memory of it that falls of the table!
    data[pixelCount - 1].destroy();

    // Shift all pixels to the 'right' (last pixel will be dropped)
    for (int i=pixelCount-1; i>0; i--) {
        data[i] = data[i-1];
    }

    // Set todays pixel
    data[0].setDate(date);
    data[0].setDone(false);
    data[0].setSynced(true);
}

void Strip::visualize() {
    for (int i=0; i<pixelCount; i++) {
        if ( i == 0 && ! data[i].isDone()) {
        setPixelTodo(i);
        } else if (! data[i].isSynced() ) {
        setPixelPending(i);
        } else if ( data[i].isDone() ){
        setPixelDone(i);
        } else {
        setPixelUndone(i);
        }
    }
    strip.show();
}

int Strip::translatePixelLocation(int index) {
  // fill pixel ring backwards except for current day, which is on first pixel (0).
  // 0 ->  0
  // 1 -> 59
  // 2 -> 58
  // 3 -> 57
  
  int pixelIndex = 0;

  int arrayIndex = index % pixelCount;
  if (arrayIndex != 0) {
    pixelIndex = pixelCount - arrayIndex;
  }

  return pixelIndex;
}

void Strip::setPixelPending(int arrayIndex) {
  int pixelIndex = translatePixelLocation(arrayIndex);

  if(awake) {
    strip.setPixelColor(pixelIndex, strip.Color(  127, 127,   0));  // yellow
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}

void Strip::setPixelDone(int arrayIndex) {
  int pixelIndex = translatePixelLocation(arrayIndex);
  
  if(awake) {
    float green = (float)pixelIndex / pixelCount * 70.0 + 7.0;
    float blue = (float)pixelIndex / pixelCount * 127.0 + 12.7;
    
    if ( pixelIndex == 0 ) {
      green = 70.0;
      blue = 127.0;
    }

    strip.setPixelColor(pixelIndex, strip.Color(  0, ceil(green),   ceil(blue)));  // blueish
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}

void Strip::setPixelUndone(int arrayIndex) {
  int pixelIndex = translatePixelLocation(arrayIndex);
  
  if(awake) {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}

void Strip::setPixelTodo(int arrayIndex) {
  int pixelIndex = translatePixelLocation(arrayIndex);
  
  if(awake) {
    strip.setPixelColor(pixelIndex, strip.Color(  230, 40,   0));  // redish
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}

void Strip::setPixelLoading(int arrayIndex) {    
  int pixelIndex = translatePixelLocation(arrayIndex);
  
  if(awake) {
    strip.setPixelColor(pixelIndex, strip.Color(  127, 0,   0));  // red
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}

void Strip::advanceLoadingAnimation() {
    setPixelLoading(loadingAnimationPixel);
    setPixelUndone(loadingAnimationPixel - 1);
    loadingAnimationPixel += 1;
    strip.show();
}

void Strip::sync(NetworkHelper* networkHelper) {
    bool syncedUp = syncUp(networkHelper);

    if(syncedUp) {
        syncDown(networkHelper);
        visualize();
    }
}

// bool Strip::syncUp(NetworkHelper* networkHelper) {

// }

// bool Strip::syncDown(NetworkHelper* networkHelper) {

// }

bool Strip::syncUp(NetworkHelper* networkHelper) {
    ArduinoBearSSL.onGetTime(&NetworkHelper::getTimeCallback);
    Serial.println("Sync Up ..");
    BearSSLClient NetworkHelper::*cli = &NetworkHelper::sslClient;
    BearSSLClient* sslClient = &(networkHelper->*cli);
    // BearSSLClient* sslClient = networkHelper->getClient();

    Serial.print("Connecting to ");
    Serial.println(networkHelper->backend);

    if (sslClient->connect(networkHelper->backend, 443)) {
        Serial.println("connected to server");
        int successCount = 0;

        DynamicJsonDocument requestDoc(48);
        DynamicJsonDocument responseDoc(32);

        for(int i=0; i<pixelCount; i++) {

        if(! data[i].isSynced()) {

            successCount += 1;

            Serial.println("Date: ");
            Serial.println(data[i].getDate());
            
            requestDoc["dates"][0]["date"] = data[i].getDate();
            
            char body[96];
            serializeJson(requestDoc, body);
            size_t bodyLength = strlen(body);
            
            Serial.println(body);

            if(data[i].isDone()) {
            sslClient->print("POST");
            } else {
            sslClient->print("DELETE");
            }

            sslClient->println(" /habit/meditation HTTP/1.1");
            sslClient->print("Host: ");
            sslClient->println(networkHelper->backend);
            sslClient->println("Content-type: application/json");
            sslClient->println("Accept: */*");
            sslClient->println("Cache-Control: no-cache");
            sslClient->println("Accept-Encoding: gzip, deflate");
            sslClient->print("Content-Length: ");
            sslClient->println(bodyLength);
            sslClient->println();
            sslClient->println(body);
    
            // Catch empty lines
            while(sslClient->available()) {
            Serial.write(sslClient->read());
            }
    
            // Check HTTP status
            char status[32] = {0};
            sslClient->readBytesUntil('\r', status, sizeof(status));
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
            if (!sslClient->find(endOfHeaders)) 
            {
            Serial.println(F("Invalid response"));
            return false;
            }
            
            DeserializationError error = deserializeJson(responseDoc, *sslClient);
            if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            return false;
            }

            if(data[i].isDone()) {
            if (responseDoc["added"] >= 0 ) {
                Serial.print("Server successfully added dates to backend.");
                data[i].setSynced(true);
                setPixelDone(i);
                strip.show();
                successCount -= 1;
            }
            } else {
            if (responseDoc["deleted"] >= 0 ) {
                Serial.print("Server successfully deleted dates from backend.");
                data[i].setSynced(true);
                setPixelUndone(i);
                strip.show();
                successCount -= 1;
            }
            }
        }
        }

        // Close connection
        sslClient->println("Connection: close");
        sslClient->println();

        // Catch remaining output
        while(sslClient->available()) {
            Serial.write(sslClient->read());
        }

        // Disconnect client
        if (sslClient->connected()) {
            sslClient->stop();
        }

        return (successCount == 0);
    }

    Serial.println("Failed to connect to server");
    
    return false;
}

bool Strip::syncDown(NetworkHelper* networkHelper) {
    ArduinoBearSSL.onGetTime(&NetworkHelper::getTimeCallback);
    BearSSLClient NetworkHelper::*cli = &NetworkHelper::sslClient;
    BearSSLClient* sslClient = &(networkHelper->*cli);
    if (sslClient->connect(networkHelper->backend, 443)) {
        Serial.println("connected to server");

        DynamicJsonDocument requestDoc(32);
    
        DynamicJsonDocument responseDoc(128);

        for(int i=0; i<pixelCount; i++) {
        
        Serial.println(data[0].getDate());
        requestDoc["startDate"] = data[0].getDate();
        requestDoc["count"] = i;
        
        char body[64];
        serializeJson(requestDoc, body);
        size_t bodyLength = strlen(body);
        Serial.println(body);

        sslClient->println("GET /habit/meditation HTTP/1.1");
        sslClient->print("Host: ");
        sslClient->println(networkHelper->backend);
        sslClient->println("Content-type: application/json");
        sslClient->println("Accept: */*");
        sslClient->println("Cache-Control: no-cache");
        sslClient->println("Accept-Encoding: gzip, deflate");
        sslClient->print("Content-Length: ");
        sslClient->println(bodyLength);
        sslClient->println();
        sslClient->println(body);

        // Catch empty lines
        while(sslClient->available()) {
            Serial.write(sslClient->read());
        }

        // Check HTTP status
        char status[32] = {0};
        sslClient->readBytesUntil('\r', status, sizeof(status));
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
        if (!sslClient->find(endOfHeaders)) 
        {
            Serial.println(F("Invalid response"));
            return false;
        }
        
        DeserializationError error = deserializeJson(responseDoc, *sslClient);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            return false;
        }

        const char* date = responseDoc["history"][0]["date"];
        int done = responseDoc["history"][0]["done"];

        data[i].setDate(date);
        if (done == 1) {
            data[i].setDone(true);
        } else {
            data[i].setDone(false);
        }
        data[i].setSynced(true);
        }

        // Close connection
        sslClient->println("Connection: close");
        sslClient->println();

        // Catch remaining output
        while(sslClient->available()) {
            Serial.write(sslClient->read());
        }

        // Disconnect client
        if (sslClient->connected()) {
            sslClient->stop();
        }

        return true;
    }
}