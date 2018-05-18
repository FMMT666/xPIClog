//
// xPIClog
//
// www.askrprojects.net
//

//--------------------------------------------------------------------------------------
// SERIAL.C
//
// - serial comm interface functions
// - command interface
// - tools
//--------------------------------------------------------------------------------------
// (c)ASkr 2011

#ifndef _asSERIAL
#define _asSERIAL
#endif


#include <pic18f27j13.h>
#include <system.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "global.h"
#include "serial.h"
#include "rtc.h"
#include "pinctrl.h"
#include "meas.h"



#define CMDBUFSIZE (RXBUFSIZE-2)



//--------------------------------------------------------------------------------------
// GLOBAL VARIABLES
volatile uchar gSerRXBuf[RXBUFSIZE];	// RX buffer
volatile uint  gSerRXBufW=0;	// write buffer index
volatile uint  gSerRXBufR=0;	// read buffer index
volatile uint  gSerRXBufC=0;	// number of bytes in buffer
volatile uchar gSerRXBufO=0;	// overflow flag

volatile uchar gCmdBuf[CMDBUFSIZE]={0};	// command buffer





//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void serUsage()
{
	serSendString("\r\n\r\nAVAILABLE COMMANDS:\r\n\r\n");
	serSendString(" help             - what you're reading right now...\r\n");
	serSendString(" start            - start acquisition\r\n");
	serSendString(" stop|ESC         - stop acquisition (only valid if not sleeping)\r\n");
	serSendString(" show             - show acquisition parameters\r\n");
	serSendString(" fname=<filename> - set log and config file name w/o extension (max 8 chars/digits)\r\n");	
	serSendString(" fname?           - show log and config file name\r\n");
	serSendString(" load             - load acquisition parameters from SD-card file '<fname>.cfg'\r\n");
	serSendString(" save             - save acquisition parameters to SD-card file '<fname>.cfg'\r\n");	
	serSendString(" time=<hh:mm:ss>  - set time\r\n");	
	serSendString(" time?            - query time\r\n");	
	serSendString(" date=<dd.mm.yy>  - set date\r\n");	
	serSendString(" date?            - query date\r\n");	
	serSendString(" sleep=<0|1>      - sleep during acqusition? 0=NO, 1=YES\r\n");	
	serSendString(" sleep?           - show sleep flag\r\n");	
	serSendString(" srate=<n>        - set sampling rate to <n>\r\n");	
	serSendString(" srate?           - query sampling rate\r\n");	
	serSendString(" smeans=<n>       - set number of samples for mean calc (1..16)\r\n");	
	serSendString(" smeans?          - query number of samples for mean calc\r\n");	
//	serSendString(" sunit=<s|m|h>    - set sampling rate unit; s=SECS, m=MINS, h=HOURS\r\n");	
	serSendString(" sunit=<s|m>      - set sampling rate unit; s=SECS, m=MINS\r\n");	
	serSendString(" sunit?           - query sampling rate unit\r\n");	
//	serSendString(" serial=<0|1>     - serial output during acquisition? 0=NO, 1=YES\r\n");	
//	serSendString(" serial?          - query serial output flag\r\n");	
	serSendString(" sep=<ch>         - set log file separator character; e.g. ';', ',', ...\r\n");	
	serSendString(" sep?             - query log file separator character\r\n");	
	serSendString(" logdate=<0|1>    - write date to logfile? 0=NO, 1=YES\r\n");	
	serSendString(" logdate?         - query date log flag\r\n");	
	serSendString(" logtime=<0|1>    - write time to logfile? 0=NO, 1=YES\r\n");	
	serSendString(" logtime?         - query time log flag\r\n");
	serSendString(" dati2nd=<0|1>    - show date/time behind channels in log file? 0=NO, 1=YES\r\n");
	serSendString(" dati2nd?         - show if date/time is behind channels in log file\r\n");
	serSendString(" logxcor=<0|1>    - log UBext and Vdd core to log file? 0=NO, 1=YES\r\n");
	serSendString(" logxcor?         - show UBext and Vdd core log flag\r\n");
	serSendString(" usecard=<0|1>    - use SD-card? 0=NO, 1=YES\r\n");	
	serSendString(" usecard?         - show if logging to SD-card is enabled\r\n");
	serSendString(" bat?             - show battery voltage in mV (channel 9)\r\n");
	serSendString(" measraw=<ch>     - measure and show raw A/D channel value <ch> 1-8 (0-11)\r\n");
	serSendString(" measvol=<ch>     - measure and show voltage at channel <ch> 1-8 (0-11)\r\n");
	serSendString(" bgcorr=<n>       - set band gap correction in mV (-100..100)\r\n");
	serSendString(" bgcorr?          - show band gap correction term\r\n");
	serSendString(" version?         - show firmware version string\r\n");
	serSendString(" off              - turn unit off (sleep)\r\n");
	
	serSendString("\r\n");	
	serSendString("\r\n");	
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void serRXBufInit()
{
	gSerRXBufW = 0;
	gSerRXBufR = 0;
	gSerRXBufC = 0;
}



//**************************************************************************************
//***
//***
//*** Called from interrupt routine
//**************************************************************************************
int serRXBufWriteChar(uchar ch, uchar echo)
{
	// check end of ringbuffer
	if( ++gSerRXBufW >= RXBUFSIZE )
		gSerRXBufW=0;

	// check overflow
	if(gSerRXBufW == gSerRXBufR )
	{
		serRXBufInit();
		gSerRXBufO=1;
		return -1;
	}

	gSerRXBuf[gSerRXBufW] = ch;
	gSerRXBufC++;


	if( echo )
	{
		while( pir1.TX1IF == 0)
		{;}	
		txreg1 = ch;
	}

	return 1;
}


//**************************************************************************************
//***
//***
//*** Reads a character from the RX buffer
//*** If <blocked> is set to "BLOCKING", this function does not return until
//*** at least one character was received.
//***
//*** RETURN values:
//*** >= 0 : a character
//***   -1 : nothing in buffer (RXBUFEMPTY)
//***   -2 : a buffer overflow occured
//**************************************************************************************
int serRXBufReadChar(int blocked)
{
	if( gSerRXBufO )
		return RXOVERFLOW;
	
	if( blocked )
	{
		while ( gSerRXBufW == gSerRXBufR )
		{;}
	}
	else
	{
		if( gSerRXBufW == gSerRXBufR )
			return RXBUFEMPTY;
	}
	
	if( ++gSerRXBufR >= RXBUFSIZE )
		gSerRXBufR=0;

	// TOCHK!
	// The RX interrupt routine may interfere right here!
	if( gSerRXBufC > 0 )
		gSerRXBufC--;
	return gSerRXBuf[gSerRXBufR];
}



//**************************************************************************************
//***
//***
//*** Flushes the RX input buffer.
//*** There are faster ways, indeed, but this one is the safe method while the RX
//*** interrupt is active...
//**************************************************************************************
void serRXBufFlushIn()
{
	while( serRXBufReadChar(NONBLOCKING) >= 0 )
	{;}
	gSerRXBufO = 0;
}



//**************************************************************************************
//***
//***
//***
//*** Returns the number of bytes in the receive buffer.
//*** If an overflow occured, '-1' will be returned.
//**************************************************************************************
int serRXBufCount()
{
	if(gSerRXBufO)
		return -1;
	else
		return (int)gSerRXBufC;
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar serSendByte(uchar ch)
{
	while( pir1.TX1IF == 0)
	{;}	
	txreg1 = ch;
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void serSendString(uchar *ch)
{
	if( ch != NULL)
		while( *ch != 0 )
			serSendByte(*ch++);
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void serSendStringDebug(uchar *ch1, long val, uchar *ch2)
{
	uchar tmp[30];
	uchar *p1;
	
	if( ch1 != NULL )
		while( *ch1 != 0 )
			serSendByte(*ch1++);

	if( val != NUMERR_LONG )
	{
		p1 = (uchar *)lltoa((uchar *)&tmp,val);
		while( *p1 != NULL )
			serSendByte(*p1++);
	}

	if( ch2 != NULL )
		while( *ch2 != 0 )
			serSendByte(*ch2++);
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void serFlushOut()
{
	while( ( txsta1.TRMT == 0) || ( pir1.TX1IF == 0) )
	{;}	
}



//**************************************************************************************
//*** lltoa()
//***
//*** This might overflow!
//**************************************************************************************
uchar *lltoa(uchar *buffer, long i)
{
	unsigned long n;
	unsigned int negate = 0;
	int c = 24;

	if (i < 0)
	{
		negate=1;
		n = -i;
	}
	else
		if (i == 0)
		{
			buffer[0] = '0';
			buffer[1] = 0;
			return buffer;
		}
		else
		{
			n = (unsigned long)i;
		}
	
	buffer[c--] = 0;
	do
	{
		buffer[c--] = (n % 10) + '0';
		n = n / 10;
	} while (n);
	
	if (negate)
	{
		buffer[c--] = '-';
	}

	return &buffer[c+1];
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
long serMatRead(uchar *str, uchar ind, uchar sep)
{
	uchar *p1 = str, *p2;
	uchar i = 0;
	long ret;

	// determine amount of separators (if ind == 0)
	if( ind == 0 )
	{
		while( *p1 != 0 )
		{
			if( *p1 == sep )
				i++;
			p1++;
		}
		return i;
	}
	else
	{
		ind--; // first field is right BEFORE the first separator
		while( ind )
		{
			if( *p1 == 0 )
				return NUMERR_LONG;
			if( *p1 == sep )
				ind--;
			p1++;
		}
		if(( *p1 == sep ) || ( *p1 == 0 ))
			return 0;

		p2 = p1;
		while( *p2 != sep )
		{
			if( *p2 == 0 )
				break;
			p2++;
		}
		i = *p2;
		*p2 = 0;
		ret=atol(p1);
		*p2 = i;
		return ret == MAX_LONG ? NUMERR_LONG : ret;
	}// END else (ind != 0)
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar cmdParse(uchar *args)
{
	uchar ret = CMD_NONE;
	uchar i,j;
	uchar *p1,*p2;;
	uchar tmp[2]={0,0};
	int ch, n;
	
	
	// default is empty
	*args = 0;
	
	// check for overflow
	if( (n=serRXBufCount()) < 0 )
	{
		serRXBufFlushIn();
		return CMD_NONE;
	}

	for(i=0;i<n;i++)
	{
		// check if anything unusual happened
		if( (ch=serRXBufReadChar(NONBLOCKING)) < 0 )
		{
			serRXBufFlushIn();
			serRXBufInit();
			ret = CMD_ERROR;
			break;
		}


		// check if ESC
		if( ch == 27 )
		{
			ret = (uchar)CMD_STOP;
			serRXBufFlushIn();
			break;
		}


		// check if RETURN typed
		if( ch != '\r' )
		{
			// skip a \n
			if( ch == '\n' )
				continue;
			
			// check command buffer overflow
			if( strlen(&gCmdBuf) >= CMDBUFSIZE )
			{
				gCmdBuf[0] = 0;
				serRXBufFlushIn();
				break;	// not an error
			}


			// copy new character
			tmp[0] = (uchar)ch & 0xff;
			strcat((char *)&gCmdBuf,(char *)&tmp);
			continue;
			
		}
		else
		{
			// RETURN was pressed
			serRXBufFlushIn();
			
			for(j=0;j<COMMANDCOUNT;j++)
			{
				if( (p2=(uchar *)strstr((uchar *)&gCmdBuf,(uchar *)&gCOMMANDS[j])) != NULL )
					break;
			}
			
			if( p2 == NULL )
				ret = (uchar)CMD_ERROR;
			else
			{
				ret = j;
				strcpy(args,(uchar *)&gCmdBuf[strlen((uchar *)&gCOMMANDS[j])]);
			}
			
			break;
		}
			
	}// END for
	

	// clear buffer if command is valid
	if( ret != CMD_NONE )
		gCmdBuf[0] = 0;
	
	return ret;
	
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar cmdCheckStringAlphaNum(uchar *str)
{
	if( str == NULL )
		return 0;
	while( *str != 0 )
		if( !isalnum(*str++) )
			return 0;

	return 1;
}
	


//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void serShowParams()
{
	uchar tmp[8];
	
	serSendString("\r\n\r\nPARAMETERS:\r\n\r\n");
	serSendStringDebug("  USECARD: ",sCfg.crdUseCard,"\r\n");
	serSendString     ("  FNAME  : ");
	serSendStringDebug( (uchar *)&sCfg.crdFName,NUMERR_LONG,"\r\n");
	serSendStringDebug("  SRATE  : ",sCfg.rtcSRate,"\r\n");
	serSendString     ("  SUNIT  : ");
	switch( sCfg.rtcSUnit )
	{
		case SECONDS: serSendString("s\r\n");break;
		case MINUTES: serSendString("m\r\n");break;
		case HOURS:   serSendString("h\r\n");break;
	}
	serSendStringDebug("  SMEANS : ",sCfg.rtcSMeans,"\r\n");
	serSendStringDebug("  SLEEP  : ",sCfg.rtcSleep,"\r\n");
	serSendStringDebug("  LOGDATE: ",sCfg.crdLogDate,"\r\n");
	serSendStringDebug("  LOGTIME: ",sCfg.crdLogTime,"\r\n");
	serSendStringDebug("  LOGXCOR: ",sCfg.crdLogExCore,"\r\n");
	serSendStringDebug("  DATI2ND: ",sCfg.crdLogDaTi2nd,"\r\n");
	serSendString     ("  SEP    : ");
	tmp[0] = ''';
	tmp[1] = sCfg.crdSeparator;
	tmp[2] = ''';
	tmp[3] = 0;
	serSendStringDebug((uchar *)&tmp,NUMERR_LONG,"\r\n");
	serSendStringDebug("  BGCORR : ",sCfg.meaBandCorr,"\r\n");

	serSendString("\r\n\r\n");

}
	


//**************************************************************************************
//***
//***
//***
//**************************************************************************************
uchar cmdCheck(uchar exec)
{
	uchar cmd = CMD_NONE;
	uchar args[RXBUFSIZE];
	long li;
	uchar a[3];
	uchar i,j;
	
	// ASkr DEBUG
	uchar tmp[30];
	
	cmd = cmdParse(&args);

	
	if( exec != EXEC )
		return cmd;


	switch( cmd )
	{
		//----------------------------------------------------------------------------------
		case CMD_NONE: break;			// NO ACTION IN HERE!
		//----------------------------------------------------------------------------------
		case CMD_OFF: break;			// NO ACTION IN HERE!
		//----------------------------------------------------------------------------------
		case CMD_ERROR: break;		// NO ACTION IN HERE!
		//----------------------------------------------------------------------------------
		case CMD_HELP: serUsage(); break;
		//----------------------------------------------------------------------------------
		case CMD_START:break;			// NO ACTION IN HERE!
		//----------------------------------------------------------------------------------
		case CMD_STOP:break;			// NO ACTION IN HERE!
		//----------------------------------------------------------------------------------
		case CMD_SHOW:
			serShowParams();
			break;
		//----------------------------------------------------------------------------------
		case CMD_LOAD:break;			// NO ACTION IN HERE!
		//----------------------------------------------------------------------------------
		case CMD_SAVE:break;			// NO ACTION IN HERE!
		//----------------------------------------------------------------------------------
		case CMD_TIMESET:
			j = 1;
			for(i=1;i<4;i++)
			{
				li = serMatRead((uchar *)&args,i,':');
				if( li == NUMERR_LONG )
				{
					j = 0;
					break;
				}
				a[i-1] = (uchar)li;
			}
			if(!j)
			{
				serSendString("ERROR\r\n");
				break;
			}
			rtcSetTime(a[0],a[1],a[2]);
			// continue with next CASE below (TIMEQUERY)...
		
		//----------------------------------------------------------------------------------
		case CMD_TIMEQUERY:
			rtcGetTimeString((uchar *)&args);
			serSendStringDebug((uchar *)&args,NUMERR_LONG,"\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_DATESET:
			j = 1;
			for(i=1;i<4;i++)
			{
				li = serMatRead((uchar *)&args,i,'.');
				if( li == NUMERR_LONG )
				{
					j = 0;
					break;
				}
				
				// quick and more than dirty 4-digit year entry hack
				if( ( i == 3 ) && ( li > 2000 ) )
					li-=2000;
				
				a[i-1] = (uchar)li;
			}
			if(!j)
			{
				serSendString("ERROR\r\n");
				break;
			}
      // TODO: either error checks in here or return code upon illegal stuff in rtcSetDate()
			rtcSetDate(a[2],a[1],a[0]);
      
			// continue with next CASE below (DATEQUERY)...
		
		//----------------------------------------------------------------------------------
		case CMD_DATEQUERY:
			rtcGetDateString((uchar *)&args);
			serSendStringDebug((uchar *)&args,NUMERR_LONG,"\r\n");
			break;
		
		//----------------------------------------------------------------------------------
		case CMD_SLEEPSET:
			li = serMatRead((uchar *)&args,1,'X');
			if( (li == 0) || (li == 1) )
				sCfg.rtcSleep = (uchar)li;
			else
			{
				serSendString("ERROR\r\n");
				break;
			}
			// continue with next CASE below (SLEEPQUERY)...
			
		//----------------------------------------------------------------------------------
		case CMD_SLEEPQUERY:
			if( sCfg.rtcSleep == NOSLEEP )
				serSendString("0\r\n");
			else
				serSendString("1\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_SRATESET:
			li = serMatRead((uchar *)&args,1,'X');
			if( (li > 0)&&(li < 65536) )
				sCfg.rtcSRate = (unsigned)li;
			else
			{
				serSendString("ERROR\r\n");
				break;
			}
			// continue with next CASE below (SRATEQUERY)...
		//----------------------------------------------------------------------------------
		case CMD_SRATEQUERY:
			serSendStringDebug(NULL,sCfg.rtcSRate,"\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_SMEANSSET:
			li = serMatRead((uchar *)&args,1,'X');
			if( (li > 0)&&(li < 17) )
				sCfg.rtcSMeans = (uchar)li;
			else
			{
				serSendString("ERROR\r\n");
				break;
			}
			// continue with next CASE below (SMEANSQUERY)...
		//----------------------------------------------------------------------------------
		case CMD_SMEANSQUERY:
			serSendStringDebug(NULL,sCfg.rtcSMeans,"\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_SUNITSET:
			i=0;
			switch((uchar)args[0])
			{
				case 's': sCfg.rtcSUnit = SECONDS; break;
				case 'm': sCfg.rtcSUnit = MINUTES; break;
//				case 'h': sCfg.rtcSUnit = HOURS;   break;		// BLOCKED (does not make sense...)
				default:  serSendString("ERROR\r\n"); i=1;
			}
			if( i )
				break;
			// continue with next CASE below (SUNITQUERY)...
				
		//----------------------------------------------------------------------------------
		case CMD_SUNITQUERY:
			switch(sCfg.rtcSUnit)
			{
				case SECONDS: serSendString("S\r\n"); break;
				case MINUTES: serSendString("M\r\n"); break;
				case HOURS:   serSendString("H\r\n"); break;
				default:      serSendString("ERROR\r\n");
			}
			break;
		//----------------------------------------------------------------------------------
		case CMD_SERIALLOGSET:
			serSendString("NOT IMPLEMENTED\r\n");		
			break;
		//----------------------------------------------------------------------------------
		case CMD_SERIALLOGQUERY:
			serSendString("NOT IMPLEMENTED\r\n");		
			break;
		//----------------------------------------------------------------------------------
		case CMD_SEPSET:
			i = (uchar)strlen((uchar*)&args);
			if( i != 1 )
			{
				serSendString("ERROR\r\n");				
				break;
			}
			sCfg.crdSeparator = args[0];
			// continue with next CASE below (SEPQUERY)...

		//----------------------------------------------------------------------------------
		case CMD_SEPQUERY:
			args[0] = ''';
			args[1] = sCfg.crdSeparator;
			args[2] = ''';
			args[3] = 0;
			serSendStringDebug((uchar *)&args,NUMERR_LONG,"\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_LOGDATESET:
			li = serMatRead((uchar *)&args,1,'X');
			if( (li == 0) || (li == 1) )
				sCfg.crdLogDate = (uchar)li;
			else
			{
				serSendString("ERROR\r\n");
				break;
			}
			// continue with next CASE below (LOGDATEQUERY)...

		//----------------------------------------------------------------------------------
		case CMD_LOGDATEQUERY:
			if( sCfg.crdLogDate == NO )
				serSendString("0\r\n");
			else
				serSendString("1\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_LOGTIMESET:
			li = serMatRead((uchar *)&args,1,'X');
			if( (li == 0) || (li == 1) )
				sCfg.crdLogTime = (uchar)li;
			else
			{
				serSendString("ERROR\r\n");
				break;
			}
			// continue with next CASE below (LOGDATEQUERY)...
		
		//----------------------------------------------------------------------------------
		case CMD_LOGTIMEQUERY:
			if( sCfg.crdLogTime == NO )
				serSendString("0\r\n");
			else
				serSendString("1\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_FNAMESET:
			if( !cmdCheckStringAlphaNum( (char *)&args) )
			{
				serSendString("ERROR\r\n");				
				break;
			}
			i = (uchar)strlen((uchar*)&args);
			if( (i < 1 ) || (i > 8) )
			{
				serSendString("ERROR\r\n");	
				break;
			}
			strcpy( (uchar*)&sCfg.crdFName, (uchar*)&args );
			// continue with next CASE below (SUNITQUERY)...

		//----------------------------------------------------------------------------------
		case CMD_FNAMEQUERY:
			serSendString( (uchar *)&sCfg.crdFName);
			serSendString("\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_USECARDSET:
			li = serMatRead((uchar *)&args,1,'X');
			if( (li == 0) || (li == 1) )
				sCfg.crdUseCard = (uchar)li;
			else
			{
				serSendString("ERROR\r\n");
				break;
			}
			// continue with next CASE below (USECARDQUERY)...
		//----------------------------------------------------------------------------------
		case CMD_USECARDQUERY:
			if( sCfg.crdUseCard == NO )
				serSendString("0\r\n");
			else
				serSendString("1\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_BATQUERY:
			measMeasureAllComplete();
			serSendStringDebug(NULL,(long)measGetChannelVoltage(AD_UBEXT),"\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_MEASNOW:
			measMeasureAllComplete();
			li = serMatRead((uchar *)&args,1,'X');
			if( (li >= 0) && (li < 12) )
				serSendStringDebug(NULL,measGetChannelValue((uchar)li),"\r\n");
			else
				serSendString("ERROR\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_MEASVOL:
			measMeasureAllComplete();
			li = serMatRead((uchar *)&args,1,'X');
			if( (li >= 0) && (li < 12) )
				serSendStringDebug(NULL,measGetChannelVoltage((uchar)li),"\r\n");
			else
				serSendString("ERROR\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_VERSIONQUERY:
			serSendStringDebug((uchar*)&sCfg.appVersion,NUMERR_LONG,"\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_DATILASTSET:
			li = serMatRead((uchar *)&args,1,'X');
			if( (li == 0) || (li == 1) )
				sCfg.crdLogDaTi2nd = (uchar)li;
			else
			{
				serSendString("ERROR\r\n");
				break;
			}
			// continue with next CASE below (USECARDQUERY)...
		//----------------------------------------------------------------------------------
		case CMD_DATILASTQUERY:
			if( sCfg.crdLogDaTi2nd == NO )
				serSendString("0\r\n");
			else
				serSendString("1\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_LOGEXCORESET:
			li = serMatRead((uchar *)&args,1,'X');
			if( (li == 0) || (li == 1) )
				sCfg.crdLogExCore = (uchar)li;
			else
			{
				serSendString("ERROR\r\n");
				break;
			}
			// continue with next CASE below (USECARDQUERY)...
		//----------------------------------------------------------------------------------
		case CMD_LOGEXCOREQUERY:
			if( sCfg.crdLogExCore == NO )
				serSendString("0\r\n");
			else
				serSendString("1\r\n");
			break;
		//----------------------------------------------------------------------------------
		case CMD_BGCORRSET:
			li = serMatRead((uchar *)&args,1,'X');
			if( (li < -100) || (li > 100) )
			{
				serSendString("ERROR\r\n");
				break;
			}
			else
				sCfg.meaBandCorr = (schar)li;
			// continue with next CASE below (LOGDATEQUERY)...
		//----------------------------------------------------------------------------------
		case CMD_BGCORRQUERY:
			serSendStringDebug(NULL,(long)sCfg.meaBandCorr,"\r\n");
			break;
		//----------------------------------------------------------------------------------
		default:
			serSendString("***UNKNOWN***\r\n"); break;
	}// END switch
	

	return cmd;
}

