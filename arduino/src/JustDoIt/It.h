#ifndef _IT_H_
#define _IT_H_

#include <Arduino.h>
#include <NetworkHelper.h>

class It {
public:
  It();

  static const char MYISO8601[];

  const char* getDate();
  void setDate(char* d);
  void setDate(String d);

  bool isDone();
  void setDone(bool d);

  bool isSynced();
  void setSynced(bool s);

  int getStreak();
  void setStreak(int);

  void destroy();

  bool getIt(int, const char*, NetworkHelper*);
  bool postIt(NetworkHelper*);
  bool getStreak(NetworkHelper*);

private:

  static const size_t DATE_LENGTH;
  
  char* date;
  bool done;
  bool synced;
  int streak;
};

#endif