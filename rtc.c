//
// xPIClog
//
// www.askrprojects.net
//

//--------------------------------------------------------------------------------------
// RTC.C
//
// - real time clock
// - other timing functions
//--------------------------------------------------------------------------------------
// (c)ASkr 2011



// TODO:
//  - RTCSYNC!



#ifndef _asRTC
#define _asRTC
#endif



#include <pic18f27j13.h>
#include <system.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "rtc.h"
#include "file.h"
#include "serial.h"
#include "meas.h"
#include "pinctrl.h"
#include "diskio.h"




//--------------------------------------------------------------------------------------
// GLOBAL VARIABLES
volatile uchar vDaTi[6] ={0,0,0,0,0,0};  // quick date and time storage (Y,M,D,h,m,s)




//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcInterrupt()
{
	uchar tmp[14];
	schar res;
	uchar sdwrite = 0;
			
	sCfg.crdWrites++;
				
	if( ++sCfg.rtcWakeUp >= sCfg.rtcWakeUpSkip )
	{
		sCfg.rtcWakeUp = 0;
		
		if( sCfg.crdWrites >= sCfg.crdWriteSkip )
		{
			sdwrite++;	// marker for below (faster)
			
			if( sCfg.crdUseCard == YES )
			{
				SDPOW(1);			// SD card power on
				pinInitSD();	// other SD card pins
			}
			
		}

		// -----------------------------------------------------------------------
		// PERFORM COMPLETE ACQUISITION
		measMeasureAllComplete();


		// -----------------------------------------------------------------------
		// STORE RESULTS		
		measStorWriteVMeas();

		// -----------------------------------------------------------------------
		// WRITE DATA TO SD-CARD
		if( sdwrite )
		{
			sCfg.crdWrites = 0;
			strcpy((uchar *)&tmp,(uchar*)&sCfg.crdFName);
			strcat((uchar *)&tmp,".log");
			
			if( sCfg.crdUseCard == YES )
			{
				// TODO
				// this always reports OK...
				if( (res=filOpenAppendClose( (uchar*)&tmp )) > 0 )
				{
					sCfg.crdSamplesWritten+=(uchar)res;
					serSendString("W");
				}
				else
				{
					// flush buffer
					while( measStorReadVMeas() > 0 )
					{;}
					serSendString("E");
				}
				
				pinExitSD();	// prepare card shutdown
				SDPOW(0);			// SD card power on
			}// END if UseCard
			else
			{
				// flush buffer
				while( measStorReadVMeas() > 0 )
				{;}
				serSendString("N");
			}
			
		}// END if write to card
		else
			serSendString("S");
		serFlushOut();
	}
	else
	{
		serSendString("-");
		serFlushOut();
	}	
	
	// -----------------------------------------------------------------------
	// CHECK SERIAL (STOP CMD)
	if( cmdCheck(NOEXEC) == (uchar)CMD_STOP )
		sCfg.rtcActive = NO;
	
}




//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcSetYear(uchar year)
{
	rtccfg |= 0b11;
	rtcvall = year;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar rtcGetYear()
{
	rtccfg |= 0b11;
	return rtcvall;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcSetMonth(uchar month)
{
	rtccfg &= 0b11111100;
	rtccfg |= 0b10;
	rtcvalh = month;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar rtcGetMonth()
{
	rtccfg &= 0b11111100;
	rtccfg |= 0b10;
	return rtcvalh;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcSetDay(uchar day)
{
	rtccfg &= 0b11111100;
	rtccfg |= 0b10;
	rtcvall = day;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar rtcGetDay()
{
	rtccfg &= 0b11111100;
	rtccfg |= 0b10;
	return rtcvall;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcSetHour(uchar hour)
{
	rtccfg &= 0b11111100;
	rtccfg |= 0b01;
	rtcvall = hour;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar rtcGetHour()
{
	rtccfg &= 0b11111100;
	rtccfg |= 0b01;
	return rtcvall;
}	


//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcSetMinute(uchar minute)
{
	rtccfg &= 0b11111100;
	rtcvalh = minute;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar rtcGetMinute(void)
{
	rtccfg &= 0b11111100;
	return rtcvalh;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcSetSecond(uchar second)
{
	rtccfg &= 0b11111100;
	rtcvall = second;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar rtcGetSecond()
{
	rtccfg &= 0b11111100;
	return rtcvall;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcSetAlarmHour(uchar hour)
{
	alrmcfg &= 0b11111100;
	alrmcfg |= 0b01;
	alrmvall = hour;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar rtcGetAlarmHour()
{
	alrmcfg &= 0b11111100;
	alrmcfg |= 0b01;
	return alrmvall;
}	


//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcSetAlarmMinute(uchar minute)
{
	alrmcfg &= 0b11111100;
	alrmvalh = minute;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar rtcGetAlarmMinute(void)
{
	alrmcfg &= 0b11111100;
	return alrmvalh;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcSetAlarmSecond(uchar second)
{
	alrmcfg &= 0b11111100;
	alrmvall = second;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar rtcGetAlarmSecond()
{
	alrmcfg &= 0b11111100;
	return alrmvall;
}	




//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar rtcBCD2DEC(uchar bcd)
{
	uchar ret;
	
	ret = bcd & 0b1111;
	ret += (bcd >> 4) * 10;
	
	return ret;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar rtcDEC2BCD(uchar dec)
{
	uchar ret;
	
	ret = ((dec / 10) << 4) & 0b11110000;
	ret += (dec % 10) & 0b00001111;
	
	return ret;
}	



//**************************************************************************************
//***
//***
//*** hour, minute and second in decimal format (not BCD)
//**************************************************************************************
void rtcSetTime(uchar hour, uchar minute, uchar second)
{
	if(hour > 23)
		hour = 0;
	if(minute > 59)
		minute = 0;
	if(second > 59)
		second = 0;
	
	while( rtccfg.RTCSYNC )
	{;}
	rtcSetHour(rtcDEC2BCD(hour));
	rtcSetMinute(rtcDEC2BCD(minute));
	rtcSetSecond(rtcDEC2BCD(second));
}



//**************************************************************************************
//***
//***
//*** year, month and day in decimal format (not BCD)
//**************************************************************************************
void rtcSetDate(uchar year, uchar month, uchar day)
{
	if(year > 99)
		year = 99;
	if(month > 12)
		month = 12;
	if(day > 31) // TODO
		day = 0;
	
	while( rtccfg.RTCSYNC )
	{;}
	rtcSetYear(rtcDEC2BCD(year));
	rtcSetMonth(rtcDEC2BCD(month));
	rtcSetDay(rtcDEC2BCD(day));
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcGetTimeString(uchar *tstr)
{
	uchar tmp[5];

	*tstr = 0;

	while( rtccfg.RTCSYNC )
	{;}

	sprintf((uchar *)tmp,"%02d",rtcBCD2DEC(rtcGetHour()));
	strcat((uchar *)tstr,(uchar *)&tmp);
	strcat((uchar *)tstr,":");
	sprintf((uchar *)tmp,"%02d",rtcBCD2DEC(rtcGetMinute()));
	strcat((uchar *)tstr,(uchar *)&tmp);
	strcat((uchar *)tstr,":");
	sprintf((uchar *)tmp,"%02d",rtcBCD2DEC(rtcGetSecond()));
	strcat((uchar *)tstr,(uchar *)&tmp);
	
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcGetDateString(uchar *tstr)
{
	uchar tmp[5];

	*tstr = 0;

	while( rtccfg.RTCSYNC )
	{;}

	sprintf((uchar *)tmp,"%02d",rtcBCD2DEC(rtcGetDay()));
	strcat((uchar *)tstr,(uchar *)&tmp);
	strcat((uchar *)tstr,".");
	sprintf((uchar *)tmp,"%02d",rtcBCD2DEC(rtcGetMonth()));
	strcat((uchar *)tstr,(uchar *)&tmp);
	strcat((uchar *)tstr,".20");
	sprintf((uchar *)tmp,"%02d",rtcBCD2DEC(rtcGetYear()));
	strcat((uchar *)tstr,(uchar *)&tmp);
	
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar rtcGetTime(uchar unit)
{
	while( rtccfg.RTCSYNC )
	{;}

	switch(unit)
	{
		//------------------------------------------
		case SECONDS: return rtcBCD2DEC(rtcGetSecond());
		case MINUTES: return rtcBCD2DEC(rtcGetMinute());
		case HOURS:   return rtcBCD2DEC(rtcGetHour());
		default:			return 0;
	}		
}	


//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcGetTime2DaTi()
{
	while( rtccfg.RTCSYNC )
	{;}
	
	vDaTi[0]=rtcBCD2DEC(rtcGetYear());
	vDaTi[1]=rtcBCD2DEC(rtcGetMonth());
	vDaTi[2]=rtcBCD2DEC(rtcGetDay());
	vDaTi[3]=rtcBCD2DEC(rtcGetHour());
	vDaTi[4]=rtcBCD2DEC(rtcGetMinute());
	vDaTi[5]=rtcBCD2DEC(rtcGetSecond());
}
	


//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar *rtcGetDateStringDaTi(uchar *tstr)
{
	uchar tmp[5];

	*tstr = 0;

	sprintf((uchar *)tmp,"%02d",vDaTi[2]);
	strcat((uchar *)tstr,(uchar *)&tmp);
	strcat((uchar *)tstr,".");
	sprintf((uchar *)tmp,"%02d",vDaTi[1]);
	strcat((uchar *)tstr,(uchar *)&tmp);
	strcat((uchar *)tstr,".20");
	sprintf((uchar *)tmp,"%02d",vDaTi[0]);
	strcat((uchar *)tstr,(uchar *)&tmp);
	
	return tstr;
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar *rtcGetTimeStringDaTi(uchar *tstr)
{
	uchar tmp[5];

	*tstr = 0;

	sprintf((uchar *)tmp,"%02d",vDaTi[3]);
	strcat((uchar *)tstr,(uchar *)&tmp);
	strcat((uchar *)tstr,":");
	sprintf((uchar *)tmp,"%02d",vDaTi[4]);
	strcat((uchar *)tstr,(uchar *)&tmp);
	strcat((uchar *)tstr,":");
	sprintf((uchar *)tmp,"%02d",vDaTi[5]);
	strcat((uchar *)tstr,(uchar *)&tmp);
	
	return tstr;
}	




//**************************************************************************************
//***
//***
//***
//**************************************************************************************
schar rtcSetAlarm(uchar unit, unsigned value)
{
	schar ret = RETERR;
	uchar ch;
	uchar newAlrmCfg = alrmcfg & 0b11000011;	// snip AMASK

	if( value < 1 )
		value = 1;

	switch(unit)
	{
		//------------------------------------------
		case SECONDS:
			{
				// check tens
				if( value % 10 == 0)
				{
					newAlrmCfg |= 0b00001000;					// 10 second mask
					ch = rtcGetTime(SECONDS);
					if( ++ch > 9 )
						ch = 0;
					rtcSetAlarmSecond(rtcDEC2BCD(ch));
					sCfg.rtcWakeUpSkip = value/10;		// number of "wake-ups" without action
					sCfg.crdWriteSkip = WRITEAFTERSECONDS/10;
				}
				else
				{
					newAlrmCfg |= 0b00000100;					// 1 second mask
					sCfg.rtcWakeUpSkip = value;				// number of "wake-ups" until "action"
					sCfg.crdWriteSkip = WRITEAFTERSECONDS;
				}	
				
				sCfg.rtcWakeUp = 0;									// current number of "wake-up skips"
				sCfg.crdWrites = 0;									// current number of "write skips"
				ret = RETOK;
				
			} break;
		//------------------------------------------
		case MINUTES:
			{
				// check tens
				if( value % 10 == 0)
				{
					newAlrmCfg |= 0b00010000;					// 10 minutes mask
					ch = rtcGetTime(MINUTES);
					if( ++ch > 9 )
						ch = 0;
					rtcSetAlarmMinute(rtcDEC2BCD(ch));
					sCfg.rtcWakeUpSkip = value/10;		// number of "wake-ups" without action
					sCfg.crdWriteSkip = WRITEAFTERMINUTES/10;
				}
				else
				{
					newAlrmCfg |= 0b00001100;					// 1 minute mask
					sCfg.rtcWakeUpSkip = value;				// number of "wake-ups" until "action"
					sCfg.crdWriteSkip = WRITEAFTERMINUTES;
				}	
				
				sCfg.rtcWakeUp = 0;									// current number of "wake-ups"
				sCfg.crdWrites = 0;									// current number of "write skips"
				ret = RETOK;

			} break;
		//------------------------------------------
		case HOURS:
			{
				// TODO
				// Full hour(s) wakeup?
				// This probably does not make any sense...
			} break;
		//------------------------------------------
		default:
	}		


	if( ret == RETOK )
		alrmcfg = newAlrmCfg;


	return ret;
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcEnableAlarm()
{
	sCfg.rtcActive = YES;
	sCfg.crdSamplesWritten = 0;
	pir3.RTCCIF = 1;					// clear INT flag
	pie3.RTCCIE = 1;					// enable RTC interrupt
	alrmcfg.ALRMEN = 1;				// enable alarm
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void rtcDisableAlarm()
{
	sCfg.rtcActive = NO;
	alrmcfg.ALRMEN = 0;				// disable alarm
	pie3.RTCCIE = 0;					// disable interrupt
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void timTimeReset()
{
	tmr0h = 0;	// will get updated on write to tmr0l
	tmr0l = 0;
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar timTimePassed(unsigned tval)
{
	unsigned i;
	
	i = tmr0l;		// updates tmr0h register too
	i |= ( (unsigned)tmr0h << 8 ) & 0xff00;

	if( i >= tval )
		return YES;
	else
		return NO;
}	




