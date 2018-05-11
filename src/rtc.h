
// unit of alarm
#define SECONDS		1
#define MINUTES		2
#define HOURS			3

// other defines
#define NOSLEEP   0
#define SLEEP     1


/*

  These should not be used outside of RTC.C
  These functions do not contain an RTC sync!)

extern void rtcSetYear(uchar year);
extern void rtcSetMonth(uchar month);
extern void rtcSetDay(uchar day);
extern void rtcSetHour(uchar hour);
extern void rtcSetMinute(uchar minute);
extern void rtcSetSecond(uchar second);

extern void rtcSetAlarmHour(uchar hour);
extern uchar rtcGetAlarmHour(void);
extern void rtcSetAlarmMinute(uchar minute);
extern uchar rtcGetAlarmMinute(void);
extern void rtcSetAlarmSecond(uchar second);
extern uchar rtcGetAlarmSecond(void);

extern uchar rtcGetYear(void);
extern uchar rtcGetMonth(void);
extern uchar rtcGetDay(void);
extern uchar rtcGetHour(void);
extern uchar rtcGetMinute(void);
extern uchar rtcGetSecond(void);
*/


extern volatile uchar vDaTi[6];  // quick date and time atorage

extern void rtcInterrupt(void);

extern uchar rtcBCD2DEC(uchar bcd);
extern uchar rtcDEC2BCD(uchar dec);

extern void rtcSetTime(uchar hour, uchar minute, uchar second);
extern void rtcSetDate(uchar year, uchar month, uchar day);

extern uchar rtcGetTime(uchar unit);
extern void rtcGetTime2DaTi(void);

extern void rtcGetTimeString(uchar *tstr);
extern void rtcGetDateString(uchar *tstr);

extern uchar *rtcGetDateStringDaTi(uchar *tstr);
extern uchar *rtcGetTimeStringDaTi(uchar *tstr);

extern schar rtcSetAlarm(uchar unit, unsigned value);
extern void rtcEnableAlarm(void);
extern void rtcDisableAlarm(void);

extern void timTimeReset(void);
extern uchar timTimePassed(unsigned tval);
