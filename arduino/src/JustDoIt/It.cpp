#include "It.h"


const char It::MYISO8601[] = "Y-m-d~TH:i:sP";
const size_t It::DATE_LENGTH = sizeof(char) * ( 26 + 1 );
char* It::todayDate = "undefined";

It::It()
 :done{false},
  synced{true} {
    date = (char*) malloc( DATE_LENGTH );
}

void It::setStrip(Adafruit_NeoPixel* _strip) {
    strip = _strip;
}
Adafruit_NeoPixel* It::getStrip() {
    return strip;
}

void It::setIndex(int i) {
    index = i;
}

int It::getIndex() {
    return index;
}

const char* It::getTodayDate() {
    return todayDate;
}
void It::setTodayDate(char* date) {
    todayDate = date;
}

const char* It::getDate() {
    return date;
}
void It::setDate(char* d) {
    if(index == 0) {
        It::setTodayDate(d);
    }
    date = d;
}
void It::setDate(String d) {
    strncpy(
      date, 
      d.c_str(),
      DATE_LENGTH
    );
    if(index == 0) {
        It::setTodayDate(date);
    }
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

bool It::getIt(NetworkHelper* networkHelper) {
    DynamicJsonDocument requestDoc(32);
    DynamicJsonDocument responseDoc(128);

    requestDoc["startDate"] = It::getTodayDate();
    requestDoc["count"] = getIndex();
    
    Serial.print("Today Date: ");
    Serial.println(It::getTodayDate());

    if(networkHelper->getRequest(&requestDoc, &responseDoc)) {
        const char* responseDate = responseDoc["history"][0]["date"];
        setDate(responseDate);

        if(responseDoc["history"][0]["done"] == 1) {
            setDone(true);
        } else {
            setDone(false);
        }

        setSynced(true);

        return true;
    }

    return false;
}

bool It::postIt(NetworkHelper* networkHelper) {
    DynamicJsonDocument requestDoc(48);
    DynamicJsonDocument responseDoc(32);

    if(! isSynced()) {
        requestDoc["dates"][0]["date"] = getDate();

        if(isDone()) {
            if(networkHelper->postRequest(&requestDoc, &responseDoc)) {
                if(responseDoc["added"] >= 0) {
                    Serial.println("Successfully added date to backend.");
                    setSynced(true);
                    return true;
                }
            }
        } else {
            if(networkHelper->deleteRequest(&requestDoc, &responseDoc)) {
                if(responseDoc["deleted"] >= 0) {
                    Serial.println("Successfully deleted date from backend.");
                    setSynced(true);
                    return true;
                }
            }
        }
    } else {
        return true;
    }

    return false;
}