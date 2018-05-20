//
// xPIClog
//
// www.askrprojects.net
//

//--------------------------------------------------------------------------------------
// FILE.C
//
// - SD-card file functions
//--------------------------------------------------------------------------------------
// (c)ASkr 2011


#ifndef _asFILE
#define _asFILE
#endif


#include <pic18f27j13.h>
#include <system.h>
#include <string.h>
#include <stdio.h>


#include "global.h"
#include "file.h"
#include "meas.h"
#include "rtc.h"
#include "serial.h"
#include "ff.h"





//--------------------------------------------------------------------------------------
// GLOBAL VARIABLES
volatile FATFS fatfs;



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
schar filOpenAppendClose(uchar *fname)
{
	FRESULT rc;
	FIL fil;
	uchar cline[85];
	uchar tmp[13];
	uchar tmp2[2]={0,0};
	unsigned i;
	unsigned j;
	schar num = 0; // WARNING: 127 MAX!


	// TODO:
	//   - return codes
	
	if( measStorCount() < 1 )
		return 0;

	// MOUNT
	rc = f_mount( (FATFS *)&fatfs, "", 1 );
	if (rc)
		return -1;

	// OPEN
	rc = f_open(&fil, fname, FA_WRITE | FA_OPEN_ALWAYS);
	if (rc)
	{
		f_mount( NULL, "", 1 );
		return -2;
	}

	// MOVE TO END
	rc = f_lseek(&fil, f_size(&fil));
	if (rc)
	{
		f_close(&fil);
		f_mount( NULL, "", 1 );
		return -3;
	}

	// WRITE DATA
	tmp2[0] = sCfg.crdSeparator;
	while( measStorReadVMeas() > 0 )
	{
		cline[0] = 0;

		if( sCfg.crdLogDate && !sCfg.crdLogDaTi2nd )
		{
			// date
			rtcGetDateStringDaTi((uchar *)&tmp);
			strcpy((uchar *)&cline,(uchar *)&tmp);
			strcat((uchar *)&cline,(uchar *)&tmp2);
		}
		
		if( sCfg.crdLogTime && !sCfg.crdLogDaTi2nd )
		{
			// time
			rtcGetTimeStringDaTi((uchar *)&tmp);
			strcat((uchar *)&cline,(uchar *)&tmp);
			strcat((uchar *)&cline,(uchar *)&tmp2);
		}

		for(i=0;i<12;i++)
		{
			if( ( sCfg.crdLogExCore == NO ) && ( (i==9)||(i==10) ) )
				continue;
				
			sprintf((uchar *)&tmp,"%04d",(int)measGetChannelValue(i));
			strcat((uchar *)&cline,(uchar *)&tmp);
			if( i<11 )
				strcat((uchar *)&cline,(uchar *)&tmp2);
		}

		if( sCfg.crdLogDate && sCfg.crdLogDaTi2nd )
		{
			// date
			strcat((uchar *)&cline,(uchar *)&tmp2);
			rtcGetDateStringDaTi((uchar *)&tmp);
			strcat((uchar *)&cline,(uchar *)&tmp);
		}
		
		if( sCfg.crdLogTime && sCfg.crdLogDaTi2nd )
		{
			// time
			strcat((uchar *)&cline,(uchar *)&tmp2);
			rtcGetTimeStringDaTi((uchar *)&tmp);
			strcat((uchar *)&cline,(uchar *)&tmp);
		}

		strcat((uchar *)&cline,"\r\n");

		rc = f_write(&fil, (uchar *)&cline, strlen((uchar *)&cline), &j);
		if (rc)
			break;
		else
			num++;
	}

	f_close(&fil);
	f_mount( NULL, "", 1 );
	
	if( rc )
		return -4;

	return num;
	
}




//**************************************************************************************
//***
//***
//*** Serial interrupt should be disabled before calling this!
//**************************************************************************************
schar filParseConfig(uchar *fname)
{
	FRESULT rc;
	FIL fil;
	uchar cline[RXBUFSIZE+1];
	uchar *pline;
	
	// do not update while acquisition is active
	if( sCfg.rtcActive )
		return -1;

	// MOUNT
	rc = f_mount( (FATFS *)&fatfs, "", 1 );
	if (rc)
		return -2;

	// OPEN
	rc = (FRESULT)f_open(&fil, fname, FA_READ | FA_OPEN_EXISTING);

  // ASkr DEBUG ONLY
  serSendStringDebug("*** OPEN ERROR: ", rc, "\r\n");

	if( rc > FR_OK )
	{
		f_mount( NULL, "", 1 );
		return -3;
	}


	serRXBufFlushIn();
	

	// READ DATA
	while( !f_eof(&fil) )
	{
		// read one line from the config file
		pline = f_gets((uchar *)&cline, RXBUFSIZE, &fil);

    
    // ASkr DEBUG ONLY
		if( pline == NULL )
      serSendString("*** pline == NULL\r\n");
    else
      serSendString(cline);
      
    
    
		if( pline == NULL )
			continue;

		// write the line content to the receive buffer		
		while( *pline != 0 )
			serRXBufWriteChar(*pline++, ECHO);
			
		// parse and evaluate
		cmdCheck(EXEC);
	}
	
	f_close(&fil);
	f_mount( NULL, "", 1 );

	serRXBufFlushIn();
	
	if( rc )
		return -4;

	return 0;
	
}


//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void filReadConfig(uchar *fname)
{
	uchar tmp[14];
	uchar i;

	// TODO:
	// - return codes

	i = (uchar)strlen((char *)fname);
	if( (i<1) || (i>8) )
		return;
	
	pie1.RC1IE = 0;						// disable RX interrupt

	strcpy((uchar *)&tmp,(uchar *)fname);
	strcat((uchar *)&tmp,".cfg");
	serSendString("LOADING: ");
	serSendStringDebug((uchar *)&tmp,NUMERR_LONG,"\r\n\r\n");

	if( filParseConfig((uchar *)&tmp) < 0 )
		serSendString("ERROR\r\n");
	else
	{
		serSendString("OK\r\n");
		serShowParams();
	}
	
	pir1.RC1IF = 0;
	pie1.RC1IE = 1;						// re-enable RX interrupt
	
}





//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void serFileStringDebug(FIL *fil, uchar *ch1, long val, uchar *ch2)
{
	uchar tmp[30];
	uchar *p1;

	if( fil == NULL )
		return;
	
	if( ch1 != NULL )
		while( *ch1 != 0 )
			f_write((FIL*)fil, (uchar*)ch1++, 1, NULL);			

	if( val != NUMERR_LONG )
	{
		p1 = (uchar *)lltoa((uchar *)&tmp,val);
		while( *p1 != NULL )
			f_write((FIL*)fil, (uchar*)p1++, 1, NULL);			
	}

	if( ch2 != NULL )
		while( *ch2 != 0 )
			f_write((FIL*)fil, (uchar*)ch2++, 1, NULL);			
}






//**************************************************************************************
//***
//***
//*** Serial interrupt should be disabled before calling this!
//**************************************************************************************
schar filCreateConfig(uchar *fname)
{
	FRESULT rc;
	FIL fil;
	uchar tmp[20]; // this should be OK ;-)
//	uchar *pline;

	// do not update while acquisition is active
	if( sCfg.rtcActive )
		return -1;

	// MOUNT
	rc = f_mount( (FATFS *)&fatfs, "", 1 );
	if (rc)
		return -2;

	// OPEN
	rc = (FRESULT)f_open(&fil, fname, FA_WRITE | FA_CREATE_ALWAYS);
	if( rc > FR_OK )
	{
		f_mount( NULL, "", 1 );
		return -3;
	}

	// WRITE NEW FILE CONTENTS

	f_write(&fil, (uchar*)"fname=", 6, NULL);			
	f_write(&fil, (uchar*)&sCfg.crdFName, strlen((uchar*)&sCfg.crdFName), NULL);			
	f_write(&fil, (uchar*)"\r\n", 2, NULL);			

	serFileStringDebug(&fil, "sleep=", (long)sCfg.rtcSleep, "\r\n");

	serFileStringDebug(&fil, "srate=", (long)sCfg.rtcSRate, "\r\n");

	serFileStringDebug(&fil, "smeans=",(long)sCfg.rtcSMeans, "\r\n");

	f_write(&fil, (uchar*)"sunit=", 6, NULL);
	switch( sCfg.rtcSUnit )
	{
		case SECONDS: f_write(&fil, (uchar*)"s\r\n", 3, NULL); break;
		case MINUTES: f_write(&fil, (uchar*)"m\r\n", 3, NULL); break;
		case HOURS:		f_write(&fil, (uchar*)"h\r\n", 3, NULL); break;
	}

	f_write(&fil, (uchar*)"sep=", 4, NULL);
	tmp[0] = sCfg.crdSeparator;
	tmp[1] = 0;
	f_write(&fil, (uchar*)&tmp, 1, NULL);			
	f_write(&fil, (uchar*)"\r\n", 2, NULL);
	
	serFileStringDebug(&fil, "logdate=",(long)sCfg.crdLogDate, "\r\n");
	
	serFileStringDebug(&fil, "logtime=",(long)sCfg.crdLogTime, "\r\n");
		
	serFileStringDebug(&fil, "dati2nd=",(long)sCfg.crdLogDaTi2nd, "\r\n");
	
	serFileStringDebug(&fil, "logxcor=",(long)sCfg.crdLogExCore, "\r\n");
	
	serFileStringDebug(&fil, "usecard=",(long)sCfg.crdUseCard, "\r\n");

	serFileStringDebug(&fil, "bgcorr=", (long)sCfg.meaBandCorr, "\r\n");
	
	f_close(&fil);
	f_mount( NULL, "", 1 );

	if( rc )
		return -4;

	return 0;
	
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
schar filSaveConfig(uchar *fname)
{
	uchar tmp[14];
	uchar i;

	i = (uchar)strlen((char *)fname);
	if( (i<1) || (i>8) )
		return -1;
	
	pie1.RC1IE = 0;						// disable RX interrupt

	strcpy((uchar *)&tmp,(uchar *)fname);
	strcat((uchar *)&tmp,".cfg");
	serSendString("SAVING: ");
	serSendStringDebug((uchar *)&tmp,NUMERR_LONG,"\r\n\r\n");

	if( filCreateConfig((uchar *)&tmp) < 0 )
		serSendString("ERROR\r\n");
	else
	{
		serSendString("OK\r\n");
		serShowParams();
	}
	
	pir1.RC1IF = 0;
	pie1.RC1IE = 1;						// re-enable RX interrupt
	
}
