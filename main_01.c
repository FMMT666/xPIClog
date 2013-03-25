//
// xPIClog
//
// www.askrprojects.net
//

//--------------------------------------------------------------------------------------
// MAIN_XX.C
//
//--------------------------------------------------------------------------------------
// (c)ASkr 2011



#include <pic18f27j13.h>
#include <system.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "ff.h"
#include "rtc.h"
#include "serial.h"
#include "file.h"
#include "meas.h"
#include "pinctrl.h"
#include "diskio.h"



//
//
//                           +--------V--------+
//                          -| MCLR    PGD/RB7 |-> SD CS
//                A/D CH 1 ->| AN0     PGC/RB6 |-> SD SCLK
//                A/D CH 2 ->| AN1         RB5 |-> SD DI
//                A/D CH 3 ->| AN2         RB4 |<- SD DO
//                A/D CH 4 ->| AN3         AN9 |<- A/D CH 5
//                     CAP ->| VCAP        AN8 |<- A/D CH 6
//              A/D POWOUT ->| AN4        AN10 |<- A/D CH 7
//                     GND ->| VSS        AN12 |<- A/D CH 8
//                     LED <-| RA7         VDD |<- 3V3 (3V5 SD)           
//                  POWOUT <-| RA6         VSS |<- GND
//                     32k <-| T1OSO       RX1 |<- RX
//                     32k ->| T1OSI       TX1 |-> TX
//               A/D UBext ->| AN11        RC5 |-> SD POW
//              PUSHBUTTON ->| RC3/RP14    RC4 |-> TRIGGER (AND DBGPIN)
//                           +-----------------+
//
// MISSING:
//   1 ANA -> RTC backup battery OR POWER (must not power on with backup battery!)
//




// NOTES:
//
// - band-gap:
//   - 10ms warten (performance)
//     oder bit BGVST==1
//   




//--------------------------------------------------------------------------------------
// CHIP CONFIG
//
// - Internal Osc
// - 4x-PLL, boosted to 32MHz
// -
//
// These are missing in the header file...
#define _CONFIG1	0x1FFF8
#define _CONFIG2	0x1FFF9
#define _CONFIG3	0x1FFFA
#define _CONFIG4	0x1FFFB
#define _CONFIG5	0x1FFFC
#define _CONFIG6	0x1FFFD
#define _CONFIG7	0x1FFFE
#define _CONFIG8	0x1FFFF


#pragma DATA _CONFIG1, 0xAC
#pragma DATA _CONFIG2, 0xF7
#pragma DATA _CONFIG3, 0x1A				// high power T1 osc
//#pragma DATA _CONFIG3, 0x0A			// low power T1 osc
#pragma DATA _CONFIG4, 0xFF
#pragma DATA _CONFIG5, 0x52
#pragma DATA _CONFIG6, 0xFC
#pragma DATA _CONFIG7, 0xFF
#pragma DATA _CONFIG8, 0xF3



//--------------------------------------------------------------------------------------
// GLOBAL VARIABLES
//volatile struct tCfg sCfg;
volatile tCfg sCfg;


//--------------------------------------------------------------------------------------
// OTHER



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void picInit()
{
	// oscillator
	osccon |= 0b01110000;		// switch to 32MHz (int. 8MHz x 4PLL)
	
	// pins and ports
	porta = 0b01000000;			// RA7 LED OFF, RA6 POWOUT OFF (active low)
	portb = 0b11110000;			// SD OUT PINS HIGH (CS, CLK, DI)
	portc = 0b00100000;			// RC5 SDPOW OFF (active low),
	trisa = 0b00101111;			// RA0,1,2,3,5 AD
	trisb = 0b00011111;			// RB0,1,2,3   AD  RB7,6,5,4 SD
	trisc	= 0b10011111;			// RC2         RX  RC7

	// overrides TRIG pin
#ifdef DEBUGDEBUG
	latc.4 = 0;
	trisc.4 = 0;
#endif


	// map RP14 to INT1 (RP14/RC3)
	asm
	{
		movlw		0x55
		movwf		_eecon2,0
		movlw		0xaa
		movwf		_eecon2,0
		bcf			_ppscon,0,1		// SourceBoost automatically creates a bank select
	}
	rpinr1 = 14;
	asm
	{
		movlw		0x55
		movwf		_eecon2,0
		movlw		0xaa
		movwf		_eecon2,0
		bsf			_ppscon,0,1		// SourceBoost automatically creates a bank select
	}
	
	// a/d converter
	ancon0 = 0;				// AN0-4 on
	ancon1 = 0x80;	  // AN8-12 on, band gap ON (MSB)
	
	// serial port 1
	spbrgh1 = 0;
	spbrg1 = 16;						// 115k2
//	baudcon1 = 0b01101000;  // idle state high, 16 bit brg
	baudcon1 = 0b01001000;  // idle state high, 16 bit brg
	txsta1 = 0b00100000;		// async, 8 bit transmission, low speed
	rcsta1 = 0b10010000;		// on

	// ext. 32768Hz crystal and RTCC
	t1con = 0b10001001;	// timer1 crystal mode, synced, on
	tmr1l = 0;
	pir1.TMR1IF = 0;
	tmr1h = 0;
	while( pir1.TMR1IF == 0 ) // wait for timer 1 overflow (2s)
	{;}

	// RTC
	eecon2 = 0x55;						// TODO: in ASM!
	eecon2 = 0xaa;
	rtccfg.RTCWREN = 1;				// enable access to RTCVALx
	alrmcfg.CHIME = 1;				// enable ARPT rollover
	rtccfg.RTCEN = 1;					// RTC on
	
	// ADC (12 bit set in CONFIG3H)
	adcon1 = 0b10101110;			// TAD = 2us; sampling time = 12 TADs to avoid annoying spikes...
	adcon0 = 0b00000001;			// AD on, int. ref
	
	// interrupts
	intcon2.INTEDG1 = 0;			// INT1 fires on falling edge
	intcon3.INT1IE = 1;				// enable INT1 interrupt
	intcon3.INT1IF = 0;
	pir1.RC1IF = 0;
	pie1.RC1IE = 1;						// enable RX interrupt
	pir3.RTCCIF = 0;
	pie3.RTCCIE = 0;					// do not yet enable RTC interrupt
	intcon.TMR0IE = 0;				// just in case: NO int for timer 0
	intcon.PEIE = 1;
	intcon.GIE = 1;

	// timer 0
	t0con = 0b10000111;				// on, 16 bit, 32us/digit (#define TIMTICK 0.000032)
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void memInit()
{
	// might get overridden by "SETTINGS.CFG" file on SD-card
	
	// internal stuff
	sCfg.rtcActive = NO;					// RTC interrupts not active
	sCfg.rtcWakeUpSkip = 0;				// number of "wake-ups" without action
	sCfg.rtcWakeUp = 0;						// current number of "wake-ups"
	sCfg.crdSamplesWritten = 0;		// number of samples written to card
	sCfg.crdWriteSkip = 0;				// number of samples without writing them to SD-card
	sCfg.crdWrites = 0;						// current number of non-written samples
	
	// user config
	sCfg.timPowOn = WAITSTABLE;		// nx10ms power up for acq. peripherals and bandgap
	sCfg.meaBandCorr = 0;					// band gap reference correction term, in mV
	sCfg.rtcSleep = SLEEP;				// specifies behaviour between 2 acquisitions
	sCfg.rtcSRate = 2;						// sampling rate
	sCfg.rtcSMeans = 3;						// number of samples for mean value
	sCfg.rtcSUnit = SECONDS;			// seconds, minutes or hours
	sCfg.serUseSerial = YES;			// serial debug/value output?
	sCfg.crdSeparator = ' ';			// separator for xSV (CSV) output
	sCfg.crdUseCard = YES;				// using SD-card is the default...
	sCfg.crdLogDate = YES;				// shall a date code appear in the CSV output?
	sCfg.crdLogTime = YES;				// shall a time code appear in the CSV output?
	sCfg.crdLogDaTi2nd = NO;			// log date and time behind channels in log file?
	sCfg.crdLogExCore	= NO;				// log UBext and core voltage?
	strcpy( (uchar *)&sCfg.crdFName,"default");  // default file name without extension

	strcpy( (uchar *)&sCfg.appVersion,xVERSION); // version string

	measStorInit();
	serRXBufInit();
	
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void main()
{
	uchar cmd;
	schar j;
	uchar fname[14];
	unsigned i;
	long swtrig = 0;
	
	
	
	picInit();
	memInit();


	serSendString("\r\nxPIClog\r\n");	
	serSendString("=======\r\n");	
	
	serUsage();
	serSendString("\r\n\r\nCOLD START\r\n");	
	serSendString("CHECK TIME AND DATE!\r\n\r\n");


	SDPOW(1);			// SD card power on
	pinInitSD();	// other SD card pins
	filReadConfig("poweron");
	pinExitSD();	// prepare card shutdown
	SDPOW(0);			// SD card power on

	
	
	// a little ugly...
	for(;;)
	{

		// MENU LOOP
		for(;;)
		{
			// check serial commands
			cmd = cmdCheck(EXEC);
			
			// check pushbutton
			if( !SWITCH )
			{
				LED(1);
				if( swtrig++ > 120000 )
					cmd = CMD_OFF;
			}
			else
			{
				LED(1);	// new v0.8a -> dim LED light during menu operation
				
				if( ( swtrig < 120000)&&(swtrig > 1000) )
					cmd = CMD_START;
				swtrig = 0;
				LED(0);
			}

			if( cmd == CMD_START )
				break;

			// power off?
			if( cmd == CMD_OFF )
			{
				serSendString("\r\n\r\nGOING TO SLEEP\r\n\r\n");
				LED(0);
				swtrig = 0;
				for(i=0;i<2000;i++)
					LED(1);
				LED(0);
				
				POWOUT(0);
				pinExitSD();	// prepare card shutdown
				SDPOW(0);			// SD card power off
				BANDGAP(0);		// voltage reference off
				
				sleep();
				nop();
				nop();
				
				while( !SWITCH )
					LED(1);
				LED(0);
				serSendString("\r\n\r\nWOKE UP\r\n\r\n");
				cmd = CMD_LOAD;	// ;-)
			}

			// read config file?
			if( cmd == CMD_LOAD )
			{
				SDPOW(1);			// SD card power on
				pinInitSD();	// other SD card pins
				filReadConfig((uchar *)&sCfg.crdFName);
				pinExitSD();	// prepare card shutdown
				SDPOW(0);			// SD card power on
			}// END CMD load

			// read config file?
			if( cmd == CMD_SAVE )
			{
				SDPOW(1);			// SD card power on
				pinInitSD();	// other SD card pins
				filSaveConfig((uchar *)&sCfg.crdFName);
				pinExitSD();	// prepare card shutdown
				SDPOW(0);			// SD card power on
			}// END CMD load

			
		}// END for menu loop


		// PREPARE ACQUISITION
		for(j=0;j<10;j++)
		{
			for(i=0;i<20000;i++)
				LED(1);
			for(i=0;i<20000;i++)
				LED(0);
		}
		serSendString("\r\nSTART\r\n");

		measStorInit();	// new v0.8a; might be advantageous to clear the buffer first...
		
		rtcSetAlarm(sCfg.rtcSUnit,sCfg.rtcSRate);
		rtcEnableAlarm();


		// ACQUISITION LOOP
		for(;;)
		{
			if( sCfg.rtcActive == NO )
				break;
			
			if( sCfg.rtcSleep )
			{
				sleep();
				nop();
				nop();
			}
		}
		
		rtcDisableAlarm();

		SDPOW(1);			// SD card power on
		pinInitSD();	// other SD card pins
		LED(1);

		// just in case we left because of a serial transmission...
		serRXBufFlushIn();
		serRXBufInit();

		serSendString("\r\nWRITE PENDING DATA: ");
		strcpy((uchar *)&fname,(uchar*)&sCfg.crdFName);
		strcat((uchar *)&fname,".log");
		
		// TODO
		if( (j=filOpenAppendClose((uchar *)&fname)) < 0)
		{
			// flush buffer
			while( measStorReadVMeas() > 0 )
			{;}
			serSendString("ERROR\r\n");
		}
		else
		{
			sCfg.crdSamplesWritten+=(uchar)j;
			serSendString("OK\r\n");
		}
		
		pinExitSD();	// prepare card shutdown
		SDPOW(0);			// SD card power off
		
		
		serSendString("\r\nSTOP\r\n");
		
		// long->ulong might overflow (in years ;-)
		serSendStringDebug("SAMPLES WRITTEN: ",(long)sCfg.crdSamplesWritten,"\r\n");

		while( !SWITCH )
		{;}

		LED(0);
		
	}// END main for loop

}
