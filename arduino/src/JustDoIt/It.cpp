#include "It.h"


const char It::MYISO8601[] = "Y-m-d~TH:i:sP";
const size_t It::DATE_LENGTH = sizeof(char) * ( 26 + 1 );

It::It() {
    date = (char*) malloc( DATE_LENGTH );
    done = false;
    synced = true;
}

const char* It::getDate() {
    return date;
}
void It::setDate(char* d) {
    date = d;
}
void It::setDate(String d) {
    strncpy(
      date, 
      d.c_str(),
      DATE_LENGTH
    );
}

bool It::isDone() {
    return done;
}
void It::setDone(bool d) {
    done = d;
}

bool It::isSynced() {
    return synced;
}
void It::setSynced(bool s) {
    synced = s;
}

void It::destroy() {
    free(date);
}