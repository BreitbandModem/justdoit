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
    data[0] = It();
    data[0].setDate(date);
    data[0].setDone(false);
    data[0].setSynced(true);
}

void Strip::done(int index, String date, NetworkHelper* networkHelper) {
    data[index].setDate(date);
    data[index].setDone(! data[index].isDone());
    data[index].setSynced(false);
    if(data[index].isDone() && index < pixelCount - 1) {
      // continue streak calculation in case backend is offline
      data[index].setStreak( data[index+1].getStreak() + 1 );
    }

    // sync pending -> green
    setPixelPending(index);
    strip.show();

    if(networkHelper->connectBackend()) {
      if(data[index].postIt(networkHelper)) {
        data[index].getStreak(networkHelper);
      }

      networkHelper->disconnectBackend();
    }

    visualize();
}

void Strip::sync(NetworkHelper* networkHelper) {
    Serial.println("Sync");
    Serial.print("Free memory: ");
    Serial.println(NetworkHelper::freeMemory());

    if(networkHelper->connectBackend()) {
      for(int i=0; i<pixelCount; i++) {
        advanceLoadingAnimation();
        if(data[i].postIt(networkHelper)) {
          if (! data[i].getIt(i, data[0].getDate(), networkHelper)) {
            // assume backend is offline
            break;
          }
        }
      }

      advanceLoadingAnimation();

      // to calculate the current streak, get the pre-calculated streaks from the backend
      // for today and yesterday (in case today has not been done yet; to allow for offline calculation of today streak).
      data[0].getStreak(networkHelper);
      data[1].getStreak(networkHelper);

      networkHelper->disconnectBackend();
    }

    visualize();
}

void Strip::setAwake(bool a) {
    awake = a;
}

void Strip::setQuietHours(bool isQuietHours) {
    quietHours = isQuietHours;
}

bool Strip::getQuietHours() {
    return quietHours;
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
    int streak = data[0].getStreak();

    for (int i=0; i<pixelCount; i++) {
        if (streak > 0) 
            streak --;

        if (! data[i].isSynced() ) {
            setPixelPending(i);
        } else if ( i == 0 && ! data[i].isDone()) {
            setPixelTodo(i);
        } else if ( data[i].isDone() ){
            setPixelDone(i, streak);
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

  if(awake && !quietHours) {
    strip.setPixelColor(pixelIndex, strip.Color(  64, 64,   0));  // yellow
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}

void Strip::setPixelDone(int arrayIndex, int streak) {
  int pixelIndex = translatePixelLocation(arrayIndex);
  
  if(awake && !quietHours) {
    // start at turquoise -> blue -> red -> green -> ...
    int hue = 65536 / 2 + streak * 180;
    int saturation = 200;
    int brightness = 64;
    if (streak == 0)
      brightness = 30;

    strip.setPixelColor(pixelIndex, strip.gamma32(strip.ColorHSV(hue, saturation, brightness)));
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}

void Strip::setPixelUndone(int arrayIndex) {
  int pixelIndex = translatePixelLocation(arrayIndex);
  
  if(awake && !quietHours) {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}

void Strip::setPixelTodo(int arrayIndex) {
  int pixelIndex = translatePixelLocation(arrayIndex);
  
  // Don't check for quiet hours -> ALWAYS show TODO pixels!
  if(awake) {
    strip.setPixelColor(pixelIndex, strip.Color(  230, 40,   0));  // redish
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}

void Strip::setPixelLoading(int arrayIndex) {    
  int pixelIndex = translatePixelLocation(arrayIndex);
  
  if(awake && !quietHours) {
    strip.setPixelColor(pixelIndex, strip.Color(  127, 0,   0));  // red
  } else {
    strip.setPixelColor(pixelIndex, strip.Color(  0, 0,   0));  // off
  }
}
