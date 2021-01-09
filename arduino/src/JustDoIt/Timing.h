#ifndef _TIMING_H_
#define _TIMING_H_

#include <Arduino.h>
#include <eztime.h>

class Timing {
    public:
        Timing();
        
        static const char MYISO8601[];

        bool syncTime();
        bool isSynced();

        String getDate();

        // void onInterval(int minutes, void(*function)());
        // void onNextDay(void (*function)());
        // void onQuietHour(int start, int end, void(*function)(bool));

    private:
        Timezone tz;

};

#endif