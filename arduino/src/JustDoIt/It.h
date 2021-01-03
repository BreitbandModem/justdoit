#ifndef _IT_H_
#define _IT_H_

#include <Arduino.h>
#include <NetworkHelper.h>

class It {
public:
  It();

  static const char MYISO8601[];
  static const char* getTodayDate();
  static void setTodayDate(char* date);

  void setIndex(int i);
  int getIndex();

  const char* getDate();
  void setDate(char* d);
  void setDate(String d);

  bool isDone();
  void setDone(bool d);

  bool isSynced();
  void setSynced(bool s);

  void destroy();

  bool getIt(NetworkHelper*);
  bool postIt(NetworkHelper*);

private:

  static const size_t DATE_LENGTH;
  static char* todayDate;

  int index;
  char* date;
  bool done;
  bool synced;
};

#endif