#include "Timing.h"

const char Timing::MYISO8601[] = "Y-m-d~TH:i:sP";

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