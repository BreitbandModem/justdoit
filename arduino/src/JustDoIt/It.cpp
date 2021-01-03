#include "It.h"


const char It::MYISO8601[] = "Y-m-d~TH:i:sP";
const size_t It::DATE_LENGTH = sizeof(char) * ( 26 + 1 );

It::It()
 :done{false},
  synced{true},
  streak{0} {
    date = (char*) malloc( DATE_LENGTH );
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

int It::getStreak() {
    return streak;
}
void It::setStreak(int s) {
    streak = s;
}

void It::destroy() {
    free(date);
}

bool It::getIt(int index, const char* todayDate, NetworkHelper* networkHelper) {
    DynamicJsonDocument requestDoc(32);
    DynamicJsonDocument responseDoc(128);

    requestDoc["startDate"] = todayDate;
    requestDoc["count"] = index;
    
    if(networkHelper->getRequest("/habit/meditation", &requestDoc, &responseDoc)) {
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
            if(networkHelper->postRequest("/habit/meditation", &requestDoc, &responseDoc)) {
                if(responseDoc["added"] >= 0) {
                    Serial.println("Successfully added date to backend.");
                    setSynced(true);
                    return true;
                }
            }
        } else {
            if(networkHelper->deleteRequest("/habit/meditation", &requestDoc, &responseDoc)) {
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

bool It::getStreak(NetworkHelper* networkHelper) {
    DynamicJsonDocument requestDoc(16);
    DynamicJsonDocument responseDoc(32);

    requestDoc["startDate"] = getDate();
    if(networkHelper->getRequest("/habit/meditation/streak", &requestDoc, &responseDoc)) {
        setStreak(responseDoc["streak"]);
        return true;
    }

    return false;
}