
// use debug code? (NO!)
//#define DEBUGDEBUG


// version
#define xVERSION "v0.8b"				// max 6 chars


// timer0 calculation helper
#define TIM01ms   31						//  1ms; timer 0; 32us per timer tick
#define TIM10ms  313						// 10ms; timer 0; 32us per timer tick

// simple function return values
#define RETOK       0
#define RETERR     -1

#define NO          0
#define YES         1

#define NOECHO      0
#define ECHO        1


// other types
typedef unsigned char uchar;
typedef signed char   schar;		// SourceBoost's char ist unsigned, by default
typedef unsigned int  uint;
typedef unsigned long ulong;


// global variables and types (main_xx.c)
typedef struct
{
	uchar    appVersion[7];				// firmware version string
	uchar    rtcActive;						// determines if RTC interrupt/functions are enabled (NO/YES)
	unsigned rtcWakeUpSkip;				// number of "wake-ups" without action
	unsigned rtcWakeUp;						// current number of "wake-ups"
	uchar    rtcSleep;						// specifies behaviour between 2 acquisitions (SLEEP/NOSLEEP)
	unsigned rtcSRate;						// sampling rate
	unsigned rtcSUnit;						// seconds, minutes or hours? (SECONDS/MINUTES/HOURS)
	uchar    rtcSMeans;						// number of samples per acquisition; used to calc a mean value
	
	schar    meaBandCorr;					// band gap reference correction term, in mV
	
	uchar    timPowOn;						// time, in 10ms steps, before measurement is done (after power)

	uchar    serUseSerial;				// serial debug/value output? (YES/NO)

	uchar    crdUseCard;					// use SD-card at all?	
	unsigned crdWriteSkip;				// number of samples without writing them to SD-card
	unsigned crdWrites;						// current number of non-written samples
	uchar    crdFName[10];				// file name without ending (max 8 chars)
	uchar    crdSeparator;				// separator for xSV (CSV) output (character)
	uchar    crdLogDaTi2nd;				// date and time behind channels in log file?
	uchar    crdLogExCore;				// log UBext and core voltage?
	uchar    crdLogDate;					// shall a date code appear in the CSV output? (NO/YES)
	uchar    crdLogTime;					// shall a time code appear in the CSV output? (NO/YES
	ulong    crdSamplesWritten;		// number of samples written to card
} tCfg;


extern volatile tCfg sCfg;

