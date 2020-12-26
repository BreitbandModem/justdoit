#ifndef _STRIP_H_
#define _STRIP_H_

#include "It.h"
#include <Arduino.h>
#include <ArduinoBearSSL.h>
#include <Adafruit_NeoPixel.h>

class Strip {
    public:
        Strip(int, int, int);

        static int freeMemory();

        void setAwake(bool);

        void visualize();
        void show();
        void newDay(String);
        void done(int, String, const char*, BearSSLClient*);
        void sync(const char*, BearSSLClient*);
        void advanceLoadingAnimation();

    private:
        Adafruit_NeoPixel strip;
        It* data;
        int pixelCount;
        bool awake;
        int loadingAnimationPixel;

        void initPixels();
        int translatePixelLocation(int);
        void setPixelPending(int);
        void setPixelUndone(int);
        void setPixelTodo(int);
        void setPixelDone(int);
        void setPixelLoading(int);
        bool syncUp(const char*, BearSSLClient*);
        bool syncDown(const char*, BearSSLClient*);
};

#endif