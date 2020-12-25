#ifndef _IT_H_
#define _IT_H_

// #include <string>
// #include <iostream> 
#include <Arduino.h>

class It {
public:
  It();

  static const char MYISO8601[];

  char* getDate();
  void setDate(char* d);
  void setDate(String d);

  bool isDone();
  void setDone(bool d);

  bool isSynced();
  void setSynced(bool s);

  void destroy();

private:

  static const size_t DATE_LENGTH;
  char* date;
  bool done;
  bool synced;
};

// char It::MYISO8601 = "Y-m-d~TH:i:sP";

#endif