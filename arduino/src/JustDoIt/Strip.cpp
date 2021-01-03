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

void Strip::done(int index, String date, NetworkHelper* networkHelper) {
    data[index].setDate(date);
    data[index].setDone(! data[index].isDone());
    data[index].setSynced(false);

    // sync pending -> green
    setPixelPending(index);
    strip.show();

    if(networkHelper->connectBackend()) {
      data[index].postIt(networkHelper);
      networkHelper->disconnectBackend();
    }
}

void Strip::sync(NetworkHelper* networkHelper) {
    Serial.println("Sync");
    Serial.print("Free memory: ");
    Serial.println(NetworkHelper::freeMemory());

    if(networkHelper->connectBackend()) {
      for(int i=0; i<pixelCount; i++) {
        if(data[i].postIt(networkHelper)) {
          data[i].getIt(i, data[0].getDate(), networkHelper);
        }
      }

      networkHelper->disconnectBackend();
    }

    visualize();
}

void Strip::setAwake(bool a) {
    awake = a;
}

void Strip::show() {
    strip.show();
}

void Strip::advanceLoadingAnimation() {
    setPixelLoading(loadingAnimationPixel);
    setPixelUndone(loadingAnimationPixel - 1);
    loadingAnimationPixel += 1;
    strip.show();
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
