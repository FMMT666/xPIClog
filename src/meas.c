//
// xPIClog
//
// www.askrprojects.net
//

//--------------------------------------------------------------------------------------
// MEAS.C
//
// - measurement functions
// - 
//--------------------------------------------------------------------------------------
// (c)ASkr 2011



#ifndef _asMEAS
#define _asMEAS
#endif



#include <pic18f27j13.h>
#include <system.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "meas.h"
#include "rtc.h"
#include "serial.h"
#include "pinctrl.h"



//--------------------------------------------------------------------------------------
// GLOBAL VARIABLES
volatile unsigned vMeas[12]={0,0,0,0,0,0,0,0,0,0,0,0}; // direct storage of last samples

// vStor:
//--------
//
// 4 bytes in each buffer (4x6):
//   B1  B2  B3  B4| B1  B2  B3  B4
// +---+---+---+---+---+---+---+---+
// |aC1|aC2|aC3|aC4|bC1|bC2|bC3|bC4|...
// +---+---+---+---+---+---+---+---+
// |aC5|aC6|aC7|aC8|bC5|bC6|bC7|bC8|...
// +---+---+---+---+---+---+---+---+
// |aC9|aCA|aCB|aCC|bC9|bCA|bCB|bCC|...
// +---+---+---+---+---+---+---+---+
// |aH1|aH2|aH3|aH4|bH1|bH2|bH3|bH4|...
// +---+---+---+---+---+---+---+---+
// |aD1|aD2|aD3|aD4|bD1|bD2|bD3|bD4|...
// +---+---+---+---+---+---+---+---+
// |aU1|aU2|aU3|aU4|bU1|bU2|bU3|bU4|...
// +---+---+---+---+---+---+---+---+
// |<-  SAMPLE1  ->|<-  SAMPLE2  ->|
//
//  aCx = low bytes (#8 bits) of channels 1..12 for sample 1
//  aHy = higher bits (#2, 8..9) of channels 1..12 for sample 1
//  aUy = upper bits (#2, 10..11) of channels 1..12 for sample 1
//  bCx = low bytes (#8 bits) of channels 1..12 for sample 2
//  bHy = higher bits (#2, 8..9) of channels 1..12 for sample 2
//  bUy = upper bits (#2, 10..11) of channels 1..12 for sample 2
//  ...
//
// |<------------ xH1 ------------>|<------------ xH2 ------------>|
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
// |   |   |C9H|C9L|C5H|C5L|C1H|C1L|   |   |CAH|CAL|C6H|C6L|C2H|C2L|...
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
//
//    |<------------ xH3 ------------>|<------------ xH4 ------------>|
//    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
// ...| m | m |CBH|CBL|C7H|C7L|C3H|C3L|   |   |CCH|CCL|C8H|C8L|C4H|C4L|
//    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
//    minute H
//
// |<------------ xD1 ------------>|<------------ xD2 ------------>|
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
// | Y | Y | Y | Y | Y | Y | Y | M | M | M | M | D | D | D | D | D |...
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
// |<---------- year --------->|<--- month --->|<------ day ------>|
//
//    |<------------ xD3 ------------>|<------------ xD4 ------------>|
//    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
// ...| h | h | h | h | m | m | m | m | h |   | s | s | s | s | s | s |
//    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
//    |<--- hour L -->|<-- minute L ->|   |   |<------- second ------>|
//                                    |   |
//                                  ->|   |<- hour H
//
// |<------------ xU1 ------------>|<------------ xU2 ------------>|
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
// |   |   |C9H|C9L|C5H|C5L|C1H|C1L|   |   |CAH|CAL|C6H|C6L|C2H|C2L|...
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
//
//    |<------------ xU3 ------------>|<------------ xU4 ------------>|
//    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
// ...|   |   |CBH|CBL|C7H|C7L|C3H|C3L|   |   |CCH|CCL|C8H|C8L|C4H|C4L|
//    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
//
//
//
volatile uchar vStor[6][STORAGESIZE*4];		// see above
volatile uint  gStorW = 0;								// write buffer index
volatile uint  gStorR = 0;								// read buffer index
volatile uint  gStorC = 0;								// number of bytes in buffer
volatile uchar gStorO = 0;								// overflow flag



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void measStorInit()
{
	gStorW = 0;
	gStorR = 0;
	gStorC = 0;
}



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
schar measStorWriteVMeas()
{
	uchar i;
	uchar hi,lo;
	
	// check end of ringbuffer
	if( ++gStorW >= STORAGESIZE )
		gStorW = 0;

	// check overflow
	if( gStorW == gStorR )
	{
		measStorInit();
		
//		gStorO = 1;	// NEW v0.8a: ignore overflows

		return -1;
	}

	// faster than a loop...
	vStor[0][4*gStorW+0] = vMeas[0] & 0xff;
	vStor[0][4*gStorW+1] = vMeas[1] & 0xff;
	vStor[0][4*gStorW+2] = vMeas[2] & 0xff;
	vStor[0][4*gStorW+3] = vMeas[3] & 0xff;

	vStor[1][4*gStorW+0] = vMeas[4] & 0xff;
	vStor[1][4*gStorW+1] = vMeas[5] & 0xff;
	vStor[1][4*gStorW+2] = vMeas[6] & 0xff;
	vStor[1][4*gStorW+3] = vMeas[7] & 0xff;

	vStor[2][4*gStorW+0] = vMeas[8] & 0xff;
	vStor[2][4*gStorW+1] = vMeas[9] & 0xff;
	vStor[2][4*gStorW+2] = vMeas[10] & 0xff;
	vStor[2][4*gStorW+3] = vMeas[11] & 0xff;

	vStor[3][4*gStorW+0] = ((vMeas[0]>>8)&0b11)|(((vMeas[4]>>6)&0b1100))|(((vMeas[8]>>4)&0b110000));
	vStor[3][4*gStorW+1] = ((vMeas[1]>>8)&0b11)|(((vMeas[5]>>6)&0b1100))|(((vMeas[9]>>4)&0b110000));
	vStor[3][4*gStorW+2] = ((vMeas[2]>>8)&0b11)|(((vMeas[6]>>6)&0b1100))|(((vMeas[10]>>4)&0b110000))|((vDaTi[4]<<2)&0b11000000);
	vStor[3][4*gStorW+3] = ((vMeas[3]>>8)&0b11)|(((vMeas[7]>>6)&0b1100))|(((vMeas[11]>>4)&0b110000));

	vStor[4][4*gStorW+0] = ((vDaTi[0] << 1) & 0b11111110) | ((vDaTi[1] >> 3) & 0b1);
	vStor[4][4*gStorW+1] = ((vDaTi[1] << 5) & 0b11100000) | (vDaTi[2] & 0b11111);
	vStor[4][4*gStorW+2] = ((vDaTi[3] << 4) & 0b11110000) | (vDaTi[4] & 0b1111);
	vStor[4][4*gStorW+3] = ((vDaTi[3] << 3) & 0b10000000) | (vDaTi[5] & 0b111111);

	vStor[5][4*gStorW+0] = ((vMeas[0]>>10)&0b11)|(((vMeas[4]>>8)&0b1100))|(((vMeas[8]>>6)&0b110000));
	vStor[5][4*gStorW+1] = ((vMeas[1]>>10)&0b11)|(((vMeas[5]>>8)&0b1100))|(((vMeas[9]>>6)&0b110000));
	vStor[5][4*gStorW+2] = ((vMeas[2]>>10)&0b11)|(((vMeas[6]>>8)&0b1100))|(((vMeas[10]>>6)&0b110000));
	vStor[5][4*gStorW+3] = ((vMeas[3]>>10)&0b11)|(((vMeas[7]>>8)&0b1100))|(((vMeas[11]>>6)&0b110000));

	gStorC++;

	return 1;
}


//**************************************************************************************
//***
//***
//*** 
//**************************************************************************************
int measStorReadVMeas()
{
	if( gStorO )
		return STOROVERFLOW;
	
	if( gStorW == gStorR )
		return STOREMPTY;
	
	if( ++gStorR >= STORAGESIZE )
		gStorR=0;

	if( gStorC > 0 )
		gStorC--;

	vMeas[0] = vStor[0][4*gStorR+0] | ((((unsigned)vStor[5][4*gStorR+0] << 10) & 0x0c00) | (((unsigned)vStor[3][4*gStorR+0] << 8) & 0x0300));
	vMeas[1] = vStor[0][4*gStorR+1] | ((((unsigned)vStor[5][4*gStorR+1] << 10) & 0x0c00) | (((unsigned)vStor[3][4*gStorR+1] << 8) & 0x0300));
	vMeas[2] = vStor[0][4*gStorR+2] | ((((unsigned)vStor[5][4*gStorR+2] << 10) & 0x0c00) | (((unsigned)vStor[3][4*gStorR+2] << 8) & 0x0300));
	vMeas[3] = vStor[0][4*gStorR+3] | ((((unsigned)vStor[5][4*gStorR+3] << 10) & 0x0c00) | (((unsigned)vStor[3][4*gStorR+3] << 8) & 0x0300));
                                                                                     
	vMeas[4] = vStor[1][4*gStorR+0] | ((((unsigned)vStor[5][4*gStorR+0] << 8) & 0x0c00)  | (((unsigned)vStor[3][4*gStorR+0] << 6) & 0x0300));
	vMeas[5] = vStor[1][4*gStorR+1] | ((((unsigned)vStor[5][4*gStorR+1] << 8) & 0x0c00)  | (((unsigned)vStor[3][4*gStorR+1] << 6) & 0x0300));
	vMeas[6] = vStor[1][4*gStorR+2] | ((((unsigned)vStor[5][4*gStorR+2] << 8) & 0x0c00)  | (((unsigned)vStor[3][4*gStorR+2] << 6) & 0x0300));
	vMeas[7] = vStor[1][4*gStorR+3] | ((((unsigned)vStor[5][4*gStorR+3] << 8) & 0x0c00)  | (((unsigned)vStor[3][4*gStorR+3] << 6) & 0x0300));
                                                                                     
	vMeas[8] = vStor[2][4*gStorR+0] | ((((unsigned)vStor[5][4*gStorR+0] << 6) & 0x0c00)  | (((unsigned)vStor[3][4*gStorR+0] << 4) & 0x0300));
	vMeas[9] = vStor[2][4*gStorR+1] | ((((unsigned)vStor[5][4*gStorR+1] << 6) & 0x0c00)  | (((unsigned)vStor[3][4*gStorR+1] << 4) & 0x0300));
	vMeas[10]= vStor[2][4*gStorR+2] | ((((unsigned)vStor[5][4*gStorR+2] << 6) & 0x0c00)  | (((unsigned)vStor[3][4*gStorR+2] << 4) & 0x0300));
	vMeas[11]= vStor[2][4*gStorR+3] | ((((unsigned)vStor[5][4*gStorR+3] << 6) & 0x0c00)  | (((unsigned)vStor[3][4*gStorR+3] << 4) & 0x0300));


	vDaTi[0] = (vStor[4][4*gStorR+0]>>1) & 0b1111111;
	vDaTi[1] = ((vStor[4][4*gStorR+1]>>5) & 0b111) | ((vStor[4][4*gStorR+0]<<3) & 0b1000);
	vDaTi[2] = vStor[4][4*gStorR+1] & 0b11111;
	vDaTi[3] = ((vStor[4][4*gStorR+2]>>4) & 0b1111) | ((vStor[4][4*gStorR+3]>>3) & 0b10000);
	vDaTi[4] = (vStor[4][4*gStorR+2] & 0b1111) | ((vStor[3][4*gStorR+2]>>2) & 0b110000);
	vDaTi[5] = vStor[4][4*gStorR+3] & 0b111111;
	
	return 1;
}




//**************************************************************************************
//***
//***
//***
//**************************************************************************************
int measStorCount()
{
	if(gStorO)
		return -1;
	else
		return gStorC;
}






//**************************************************************************************
//***
//***
//*** channel mapping:
//***     
//***      channel    CHS  function    int/ext
//***         0        4    POWad        ext
//***         1        0    ch 1         ext
//***         2        1    ch 2         ext
//***         3        2    ch 3         ext
//***         4        3    ch 4         ext
//***         5        9    ch 5         ext
//***         6        8    ch 6         ext
//***         7       10    ch 7         ext
//***         8       12    ch 8         ext
//***         9       11    UBext			   ext
//***        10       14    VDDcore      int
//***        11       15    Vbg          int
//***            
//**************************************************************************************
void measSelectChannel(uchar channel)
{
	uchar chsel;
	
	if( channel > 11 )
		channel = 11;

	switch(channel)
	{
		case  0: chsel =  4; break;
		case  1: chsel =  0; break;
		case  2: chsel =  1; break;
		case  3: chsel =  2; break;
		case  4: chsel =  3; break;
		case  5: chsel =  9; break;
		case  6: chsel =  8; break;
		case  7: chsel = 10; break;
		case  8: chsel = 12; break;
		case  9: chsel = 11; break;
		case 10: chsel = 14; break;
		case 11: chsel = 15; break;
	}	
	
	adcon0 &= 0b11000011;
	adcon0 |= (chsel << 2) & 0b00111100;
	
}	




//**************************************************************************************
//***
//***
//*** Performs a complete acquisistion of all available 12 channels and
//*** stores the results in the global array "vMeas[]".
//**************************************************************************************
void measMeasureAll()
{
	uchar i,j;
	long l;

	// calibration
	adcon1.ADCAL = 1;
	adcon0.GO = 1;
	nop();
	while( adcon0.GO == 1)
	{;}
	adcon1.ADCAL = 0;

	
	// real measurement
	for(i=0;i<12;i++)
		vMeas[i] = 0;


	if( sCfg.rtcSMeans == 0 )
		sCfg.rtcSMeans = 1;
	else
	{
		if( sCfg.rtcSMeans > 16 )
			sCfg.rtcSMeans = 16;
	}
		
	for(j=0;j<sCfg.rtcSMeans;j++)
	{
		for(i=0;i<12;i++)
		{
			measSelectChannel(i);
			adcon0.GO = 1;
			nop();
			while( adcon0.GO == 1)
			{;}
			vMeas[i] += (( (unsigned)adresh << 8 ) & 0xff00 ) + ( (unsigned)adresl & 0xff );
		}	
	}

	for(i=0;i<12;i++)
		vMeas[i] /= sCfg.rtcSMeans;
	
}	



//**************************************************************************************
//***
//***
//***
//**************************************************************************************
unsigned int measGetChannelValue(uchar channel)
{
	if( channel > 11 )
		channel = 11;
	return vMeas[channel];
}



//**************************************************************************************
//***
//***
//*** Returns the real A/D measured voltage in mV.
//*** Uses band gap conversion result and correction factor.
//*** If channel AD_UBEXT is selected, the resistor divider 3M3/2M2 will be
//*** considered too...
//**************************************************************************************
unsigned int measGetChannelVoltage(uchar channel)
{
	long l;
	
	if( channel > 11 )
		channel = 11;

	if( channel == AD_UBEXT )
		return ((long)vMeas[channel]*250*((long)sCfg.meaBandCorr+1200))/((long)vMeas[AD_BANDGAP]*100);
	else
		return ((long)vMeas[channel]*((long)sCfg.meaBandCorr+1200))/vMeas[AD_BANDGAP];
}
	


//**************************************************************************************
//***
//***
//***
//*** In addition to "measMeasureAll()", this function:
//***  - turns on acq peripherals
//***  - waits to stabilize
//***  - measures
//***  - turns acq peripherals off
//**************************************************************************************
void measMeasureAllComplete()
{
	timTimeReset();
	BANDGAP(1);
	POWOUT(1);
	while( !timTimePassed( (uint)sCfg.timPowOn * TIM10ms ) )
	{;}
	LED(1);
	rtcGetTime2DaTi();
	measMeasureAll();
	BANDGAP(0);
	POWOUT(0);
	LED(0);
}



	
		