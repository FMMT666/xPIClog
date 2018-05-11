
// ASkr 3/2011:
// SourceBoost & PIC18 mods


/*------------------------------------------------------------------------/
/  Bitbanging MMCv3/SDv1/SDv2 (in SPI mode) control module
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2010, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/--------------------------------------------------------------------------/
 Features and Limitations:

 * Very Easy to Port
   It uses only 4-6 bit of GPIO port. No interrupt, no SPI port is used.

 * Platform Independent
   You need to modify only a few macros to control GPIO ports.

 * Low Speed
   The data transfer rate will be several times slower than hardware SPI.

 * No Media Change Detection
   Application program must re-mount the volume after media change or it
   results a hard error.

/-------------------------------------------------------------------------*/



// ASkr
#include <pic18f27j13.h>
#include <system.h>
#include <memory.h>
#include "global.h"
#include "serial.h"



#include "diskio.h"		/* Common include file for FatFs and disk I/O layer */




#define DLY_US(n)	dly_us(n)	/* Delay n microseconds */


// ASkr
// REMEMBER TO CHANGE init_ports() TOO!
#define	CS_H()		latb.7=1	/* Set MMC CS "high" */
#define CS_L()		latb.7=0	/* Set MMC CS "low" */
#define CK_H()		latb.6=1	/* Set MMC SCLK "high" */
#define	CK_L()		latb.6=0	/* Set MMC SCLK "low" */
#define DI_H()		latb.5=1	/* Set MMC DI "high" */
#define DI_L()		latb.5=0	/* Set MMC DI "low" */
#define DO				portb.4		/* Get MMC DO value (high:true, low:false) */

#define	INS			 1			/* Card is inserted (yes:true, no:false, default:true) */
#define	WP			 0			/* Card is write protected (yes:true, no:false, default:false) */



/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/

/* MMC/SD command (SPI mode) */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD41	(41)		/* SEND_OP_COND (ACMD) */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

/* Card type flags (CardType) */
#define CT_MMC		0x01		/* MMC ver 3 */
#define CT_SD1		0x02		/* SD ver 1 */
#define CT_SD2		0x04		/* SD ver 2 */
#define CT_SDC		(CT_SD1|CT_SD2)	/* SD */
#define CT_BLOCK	0x08		/* Block addressing */

static DSTATUS Stat = STA_NOINIT;	/* Disk status */

static BYTE CardType;			/* b0:MMC, b1:SDv1, b2:SDv2, b3:Block addressing */





// ASkr TODO
// Nur ne Notl�sung zum testen ;-)
void dly_us(unsigned int ti)
{
	unsigned int i,j;
	
	for(j=0;j<ti;j++)
	{
	}
}







//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void pinInitSD()
{
	// ASkr TODO:
	// - POWER PIN
	CS_H();
	CK_L();
	DI_H();
	trisb &= 0b00011111;	// CS, SCLK, DI output
	trisb |= 0b00010000;	// DO input
}	




//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void pinExitSD()
{
	// ASkr TODO:
	// - POWER PIN
	CS_H();
	CK_L();
	DI_H();
	trisb |= 0b11100000;	// CS, SCLK, DI input
}	















/*-----------------------------------------------------------------------*/
/* Transmit bytes to the MMC (bitbanging)                                */
/*-----------------------------------------------------------------------*/
static
void xmit_mmc (
	const BYTE* buff,	/* Data to be sent */
	UINT bc				/* Number of bytes to send */
)
{
	BYTE d;


	do {
		d = *buff++;	/* Get a byte to be sent */
		if (d & 0x80) DI_H(); else DI_L();	/* bit7 */
		CK_H();  CK_L();
		if (d & 0x40) DI_H(); else DI_L();	/* bit6 */
		CK_H();  CK_L();
		if (d & 0x20) DI_H(); else DI_L();	/* bit5 */
		CK_H();  CK_L();
		if (d & 0x10) DI_H(); else DI_L();	/* bit4 */
		CK_H();  CK_L();
		if (d & 0x08) DI_H(); else DI_L();	/* bit3 */
		CK_H();  CK_L();
		if (d & 0x04) DI_H(); else DI_L();	/* bit2 */
		CK_H();  CK_L();
		if (d & 0x02) DI_H(); else DI_L();	/* bit1 */
		CK_H();  CK_L();
		if (d & 0x01) DI_H(); else DI_L();	/* bit0 */
		CK_H();  CK_L();
	} while (--bc);
}


/*-----------------------------------------------------------------------*/
/* Receive bytes from the MMC (bitbanging)                               */
/*-----------------------------------------------------------------------*/

static
void rcvr_mmc (
	BYTE *buff,	/* Pointer to read buffer */
	UINT bc		/* Number of bytes to receive */
)
{
	BYTE r;


	DI_H();	/* Send 0xFF */

	do {
		r = 0;   if (DO) r++;	/* bit7 */
		CK_H();  CK_L(); 
		r <<= 1; if (DO) r++;	/* bit6 */
		CK_H();  CK_L(); 
		r <<= 1; if (DO) r++;	/* bit5 */
		CK_H();  CK_L(); 
		r <<= 1; if (DO) r++;	/* bit4 */
		CK_H();  CK_L(); 
		r <<= 1; if (DO) r++;	/* bit3 */
		CK_H();  CK_L(); 
		r <<= 1; if (DO) r++;	/* bit2 */
		CK_H();  CK_L(); 
		r <<= 1; if (DO) r++;	/* bit1 */
		CK_H();  CK_L(); 
		r <<= 1; if (DO) r++;	/* bit0 */
		CK_H();  CK_L(); 
		*buff++ = r;			/* Store a received byte */
	} while (--bc);
}



/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

static
int wait_ready (void)	/* 1:OK, 0:Timeout */
{
	BYTE d;
	UINT tmr;
	
	for (tmr = 5000; tmr; tmr--)
	{	/* Wait for ready in timeout of 500ms */
		rcvr_mmc(&d, 1);
		if (d == 0xFF)
			return 1;
		DLY_US(100);
	}
	
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/

static
void deselect (void)
{
	BYTE d;

	CS_H();
	rcvr_mmc(&d, 1);
}



/*-----------------------------------------------------------------------*/
/* Select the card and wait for ready                                    */
/*-----------------------------------------------------------------------*/

static
int select (void)	/* 1:OK, 0:Timeout */
{
	CS_L();
	if (!wait_ready())
	{
		deselect();
		return 0;
	}
	return 1;
}



/*-----------------------------------------------------------------------*/
/* Receive a data packet from MMC                                        */
/*-----------------------------------------------------------------------*/

static
int rcvr_datablock (	/* 1:OK, 0:Failed */
	BYTE *buff,			/* Data buffer to store received data */
	UINT btr			/* Byte count */
)
{
	BYTE d[2];
	UINT tmr;


	for (tmr = 1000; tmr; tmr--)
	{	/* Wait for data packet in timeout of 100ms */
		rcvr_mmc(d, 1);
		if (d[0] != 0xFF)
			break;
		DLY_US(100);
	}
	if (d[0] != 0xFE)
		return 0;		/* If not valid data token, retutn with error */

	rcvr_mmc(buff, btr);			/* Receive the data block into buffer */
	rcvr_mmc(d, 2);					/* Discard CRC */

	return 1;						/* Return with success */
}



/*-----------------------------------------------------------------------*/
/* Send a data packet to MMC                                             */
/*-----------------------------------------------------------------------*/

static
int xmit_datablock (	/* 1:OK, 0:Failed */
	const BYTE *buff,	/* 512 byte data block to be transmitted */
	BYTE token			/* Data/Stop token */
)
{
	BYTE d[2];


	if (!wait_ready()) return 0;

	d[0] = token;
	xmit_mmc(d, 1);				/* Xmit a token */
	if (token != 0xFD) {		/* Is it data token? */
		xmit_mmc(buff, 512);	/* Xmit the 512 byte data block to MMC */
		rcvr_mmc(d, 2);			/* Dummy CRC (FF,FF) */
		rcvr_mmc(d, 1);			/* Receive data response */
		if ((d[0] & 0x1F) != 0x05)	/* If not accepted, return with error */
			return 0;
	}

	return 1;
}



/*-----------------------------------------------------------------------*/
/* Send a command packet to MMC                                          */
/*-----------------------------------------------------------------------*/

static
BYTE send_cmd(/* Returns command response (bit7==1:Send failed)*/
	BYTE cmd,		/* Command byte */
	DWORD arg		/* Argument */
)
{
	BYTE n, d, buf[6];

	// ASkr
	BYTE cmd2;
	DWORD arg2;

	// ASkr
	// THIS DOES NOT WORK WITH SOURCEBOOST!
/*
	if (cmd & 0x80)
	{
		cmd &= 0x7F;
		n = send_cmd(CMD55, 0);
		if (n > 1)
			return n;
	}
*/


	if (cmd & 0x80)
	{
		cmd2 = cmd & 0x7F;
		arg2 = arg;
		n = send_cmd(CMD55, 0);
		if (n > 1)
			return n;
		cmd = cmd2;
		arg = arg2;
	}
	

	// ASkr
	// sets CS, "reads" one byte
	deselect();
	
	// ASkr
	// clears CS, "reads" one byte
	if (!select())
		return 0xFF;

	/* Send a command packet */
	buf[0] = 0x40 | cmd;			/* Start + Command index */
	buf[1] = (BYTE)(arg >> 24);		/* Argument[31..24] */
	buf[2] = (BYTE)(arg >> 16);		/* Argument[23..16] */
	buf[3] = (BYTE)(arg >> 8);		/* Argument[15..8] */
	buf[4] = (BYTE)arg;				/* Argument[7..0] */
	n = 0x01;						/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;		/* (valid CRC for CMD0(0)) */
	if (cmd == CMD8) n = 0x87;		/* (valid CRC for CMD8(0x1AA)) */
	buf[5] = n;
	xmit_mmc(buf, 6);

	/* Receive command response */
	if (cmd == CMD12)
		rcvr_mmc(&d, 1);	/* Skip a stuff byte when stop reading */

	n = 10;								/* Wait for a valid response in timeout of 10 attempts */
	do
		rcvr_mmc(&d, 1);
	while ((d & 0x80) && --n);

	return d;			/* Return with the response value */
}



/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE drv			/* Drive number (0) */
)
{
	DSTATUS s = Stat;


	if (drv || !INS) {
		s = STA_NODISK | STA_NOINIT;
	} else {
		s &= ~STA_NODISK;
    
    // ASkr FIX write error, 5/2018
    // This was the source of the WRITE ERROR, with newer Sourceboost
    // versions. WP was defined as (0), but the code "s |= STA_PROTECT;"
    // was always executed...
		if (WP)
			s |= STA_PROTECT;
		else
			s &= ~STA_PROTECT;
	}
	Stat = s;

	return s;
}



/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(
	BYTE drv		/* Physical drive nmuber (0) */
)
{
	BYTE n, ty, cmd, buf[4];
	UINT tmr;
	DSTATUS s;


	pinInitSD();


  // ASkr TODO: FIX BROKEN SAVE
  // SPI protocol looks good, no difference to a working version (old Sourceboost).
  // Looks more like a SW issue. Possible stack corruption/overflow/whatever?
  
  
	s = disk_status(drv);
	if (s & STA_NODISK)
		return s;


	// CS HIGH
	CS_H();
	
	
	// 80 CLOCK PULSES
	for (n = 10; n; n--)
		rcvr_mmc(buf, 1);


	// SEND CMD0
	ty = 0;
	if (send_cmd(CMD0, 0) == 1)
	{
	
		// enter idle state
		if (send_cmd(CMD8, 0x1AA) == 1)
		{
			
			// SDv2?
			rcvr_mmc(buf, 4);							/* Get trailing return value of R7 resp */
		
			if (buf[2] == 0x01 && buf[3] == 0xAA)
			{ /* The card can work at vdd range of 2.7-3.6V */
				for (tmr = 1000; tmr; tmr--)
				{ /* Wait for leaving idle state (ACMD41 with HCS bit) */
					if (send_cmd(ACMD41, 1UL << 30) == 0)
						break;
					DLY_US(1000);
				}
				
				if (tmr && send_cmd(CMD58, 0) == 0)
				{	/* Check CCS bit in the OCR */
					rcvr_mmc(buf, 4);
					ty = (buf[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* SDv2 */
				}
			}
		}
		else
		{	/* SDv1 or MMCv3 */
			
			if (send_cmd(ACMD41, 0) <= 1)
			{
				ty = CT_SD1;
				cmd = ACMD41;	/* SDv1 */
	
			}
			else
			{
				ty = CT_MMC;
				cmd = CMD1;	/* MMCv3 */
				
			}
			
			
			for (tmr = 1000; tmr; tmr--)
			{	/* Wait for leaving idle state */
				
				if (send_cmd(ACMD41, 0) == 0)
					break;
				DLY_US(1000);
			}
			
			if (!tmr || send_cmd(CMD16, 512) != 0)	/* Set R/W block length to 512 */
				ty = 0;
		}
	}
	
	
	
	CardType = ty;
	if (ty)		/* Initialization succeded */
		s &= ~STA_NOINIT;
	else		/* Initialization failed */
		s |= STA_NOINIT;

		
	Stat = s;

	deselect();

	return s;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE drv,			/* Physical drive nmuber (0) */
	BYTE *buff,			/* Pointer to the data buffer to store read data */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..128) */
)
{
	DSTATUS s;


	s = disk_status(drv);
	if (s & STA_NOINIT)
		return RES_NOTRDY;
		

	if (!count)
		return RES_PARERR;


	if (!(CardType & CT_BLOCK))
		sector *= 512;	/* Convert LBA to byte address if needed */

	if (count == 1)
	{	/* Single block read */
		
		if ((send_cmd(CMD17, sector) == 0) && rcvr_datablock(buff, 512))
			count = 0;
			
	}
	else
	{	/* Multiple block read */
		
		if (send_cmd(CMD18, sector) == 0)
		{	/* READ_MULTIPLE_BLOCK */
		
			do
			{
			
				if (!rcvr_datablock(buff, 512)) break;
				buff += 512;
			} while (--count);
			send_cmd(CMD12, 0);				/* STOP_TRANSMISSION */
		}
		
	}
	deselect();

	return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0) */
	const BYTE *buff,	/* Pointer to the data to be written */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..128) */
)
{
	DSTATUS s;


	s = disk_status(drv);
	if (s & STA_NOINIT) return RES_NOTRDY;
	if (s & STA_PROTECT) return RES_WRPRT;
	if (!count) return RES_PARERR;
	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert LBA to byte address if needed */

	if (count == 1) {	/* Single block write */
		if ((send_cmd(CMD24, sector) == 0)	/* WRITE_BLOCK */
			&& xmit_datablock(buff, 0xFE))
			count = 0;
	}
	else {				/* Multiple block write */
		if (CardType & CT_SDC) send_cmd(ACMD23, count);
		if (send_cmd(CMD25, sector) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			do {
				if (!xmit_datablock(buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!xmit_datablock(0, 0xFD))	/* STOP_TRAN token */
				count = 1;
		}
	}
	deselect();

	return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	BYTE n, csd[16];
	WORD cs;


	if (disk_status(drv) & STA_NOINIT)					/* Check if card is in the socket */
		return RES_NOTRDY;

	res = RES_ERROR;
	switch (ctrl) {
		case CTRL_SYNC :		/* Make sure that no pending write process */
			if (select()) {
				deselect();
				res = RES_OK;
			}
			break;

		case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
			if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
				if ((csd[0] >> 6) == 1) {	/* SDC ver 2.00 */
					cs= csd[9] + ((WORD)csd[8] << 8) + 1;
					*(DWORD*)buff = (DWORD)cs << 10;
				} else {					/* SDC ver 1.XX or MMC */
					n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
					cs = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
					*(DWORD*)buff = (DWORD)cs << (n - 9);
				}
				res = RES_OK;
			}
			break;

		case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
			*(DWORD*)buff = 128;
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
	}

	deselect();

	return res;
}
