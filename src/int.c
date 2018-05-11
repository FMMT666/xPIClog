//
// xPIClog
//
// www.askrprojects.net
//

//--------------------------------------------------------------------------------------
// INT.C
//
// - interrupt handling
// - ...
//--------------------------------------------------------------------------------------
// (c)ASkr 2011



#ifndef _asINT
#define _asINT
#endif



#include <pic18f27j13.h>
#include <system.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "rtc.h"
#include "serial.h"
#include "file.h"
#include "pinctrl.h"



//--------------------------------------------------------------------------------------
// GLOBAL VARIABLES



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void interrupt()
{
	uchar i;
	
	// -----------------------------------------------------------------------------------
	// CHECK RTC INTERRUPT
	if( pir3.RTCCIF )
	{
		
		if(	sCfg.rtcActive == YES )
			rtcInterrupt();

		pir3.RTCCIF = 0;
	} // END RTC interrupt



	// -----------------------------------------------------------------------------------
	// CHECK RX INTERRUPT
	if( pir1.RC1IF )
	{
		if (serRXBufWriteChar((uchar)rcreg1, sCfg.rtcActive ? NOECHO : ECHO) < 0)
		{
			serRXBufFlushIn();
			serRXBufInit();
		}

		// some times, strange things happen...
		if( rcsta1.OERR )
		{
			rcsta1.CREN = 0;
			serRXBufFlushIn();
			serRXBufInit();
			rcsta1.CREN = 1;
		}
			
	}// END RX interrupt



	// -----------------------------------------------------------------------------------
	// CHECK INT1 INTERRUPT (pushbutton)
	if( intcon3.INT1IF )
	{
		if( sCfg.rtcActive )
			sCfg.rtcActive = NO;
		
		intcon3.INT1IF = 0;
	}

	
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void interrupt_low()
{
}




