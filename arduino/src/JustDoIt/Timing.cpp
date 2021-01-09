#include "Timing.h"

// Init static members

Timezone Timing::tz;
const char Timing::MYISO8601[] = "Y-m-d~TH:i:sP";

int Timing::intervalMinutes = 15;
void (*Timing::intervalCallback)() = NULL;

void (*Timing::nextDayCallback)() = NULL;

int Timing::quietHourStart = 21;
int Timing::quietHourEnd = 7;
void (*Timing::quietHourCallback)(bool) = NULL;

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

void Timing::callEvents() {
    // simply calls the eztime events trigger
    events();
}

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
};

void Timing::onIntervalScheduler() {
    intervalCallback();
    setEvent(&Timing::onIntervalScheduler, nextInterval(intervalMinutes));
};

void Timing::onNextDay(void(*callback)()) {
    nextDayCallback = callback;
    deleteEvent(&Timing::onNextDayScheduler);
    setEvent(&Timing::onNextDayScheduler, nextDay());
};

time_t Timing::nextDay() {

    tmElements_t tm;
    breakTime(tz.now(), tm);

    // Run at 03:00 on the next day
    tm.Day = tm.Day + 1;
    tm.Hour = 3;
    tm.Minute = 0;
    tm.Second = 0;
  
    time_t nextEventUTC = tz.tzTime(makeTime(tm));
    return nextEventUTC;
};

void Timing::onNextDayScheduler() {
    nextDayCallback();
    setEvent(&Timing::onNextDayScheduler, nextDay());
};

void Timing::onQuietHour(int start, int end, void(*callback)(bool)) {
    quietHourStart = start;
    quietHourEnd = end;
    quietHourCallback = callback;

    deleteEvent(&Timing::onQuietHourScheduler);
    // Calling the scheduler is always safe,
    // so we call it directly instead of scheduling an initial event for it.
    onQuietHourScheduler();
};

void Timing::pauseQuietHour(int minutes) {
    // Let the callback know that there's no quiet hours
    quietHourCallback(false);

    // Set an event in x minutes to reevaluate quiet hour status
    tmElements_t tm;
    breakTime(UTC.now(), tm);
    tm.Minute = tm.Minute + minutes;
    deleteEvent(&Timing::onQuietHourScheduler);
    setEvent(&Timing::onQuietHourScheduler, makeTime(tm));
}

void Timing::onQuietHourScheduler() {
    tmElements_t tm;
    breakTime(tz.now(), tm);

    if (tm.Hour >= quietHourStart || tm.Hour < quietHourEnd) {
        // We're in quiet hours -> next event at quiet hour end.
        tm.Day = tm.Day + 1;
        tm.Hour = quietHourEnd;

        quietHourCallback(true);
    } else {
        // We're outside of quiet hours -> next event at quiet hour start.
        tm.Hour = quietHourStart;

        quietHourCallback(false);
    }

    time_t nextEventUTC = tz.tzTime(makeTime(tm));
    setEvent(onQuietHourScheduler, nextEventUTC);
};