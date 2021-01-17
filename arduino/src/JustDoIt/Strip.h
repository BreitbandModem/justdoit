#ifndef _STRIP_H_
#define _STRIP_H_

#include "It.h"
#include <Arduino.h>
#include <NetworkHelper.h>
#include <Adafruit_NeoPixel.h>

class Strip {
    public:
        Strip(int, int, int);

        void setAwake(bool);
        void setQuietHours(bool);
        bool getQuietHours();

        void visualize();
        void show();
        void newDay(String);
        void done(int, String, NetworkHelper*);
        void sync(NetworkHelper*);
        void advanceLoadingAnimation();

    private:
        Adafruit_NeoPixel strip;
        It* data;
        int pixelCount;
        bool awake;
        bool quietHours;
        bool freshDay;  // is true right after new day has started until quiet hour end. 
        int loadingAnimationPixel;

        void initPixels();
        int translatePixelLocation(int);
        void setPixelPending(int);
        void setPixelUndone(int);
        void setPixelTodo(int);
        void setPixelDone(int, int);
        void setPixelLoading(int);
        bool syncUp(NetworkHelper*);
        bool syncDown(NetworkHelper*);
};

#endif