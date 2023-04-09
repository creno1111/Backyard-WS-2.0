#ifndef timekeeper_h
#define timekeeper_h

void timeNTPStart(void);
bool timeCheck(void);
void timeOffset(long minutes,bool enable);
void dateOffset(int year, int month, int day);
int timeYear(void);
int timeMonth(void);
int timeDay(void);
int timeHour(void);
int timeMinute(void);
int timeSecond(void);

#endif      //timekeeper_h