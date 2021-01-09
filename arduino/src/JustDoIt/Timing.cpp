#include "Timing.h"

const char Timing::MYISO8601[] = "Y-m-d~TH:i:sP";
int Timing::intervalMinutes = 15;
void (*Timing::intervalCallback)() = NULL;

Timing::Timing() {
    // setDebug(INFO);
};

String Timing::getDate() {
    return tz.dateTime(MYISO8601);
};

bool Timing::syncTime() {
    setDebug(INFO);
    waitForSync(10);
    tz.setLocation(F("de"));
    return isSynced();
};

bool Timing::isSynced() {
    return (timeStatus() == timeSet);
};


void Timing::onInterval(int minutes, void(*callback)()) {
    intervalMinutes = minutes;
    intervalCallback = callback;
    deleteEvent(&Timing::onIntervalScheduler);
    setEvent(&Timing::onIntervalScheduler, nextInterval(intervalMinutes));
};

time_t Timing::nextInterval(int minutes) {
    tmElements_t tm;
    breakTime(UTC.now(), tm);
    tm.Minute = tm.Minute + minutes;
    return makeTime(tm);
}

void Timing::onIntervalScheduler() {
    intervalCallback();
    setEvent(&Timing::onIntervalScheduler, nextInterval(intervalMinutes));
}