//
// xPIClog
//
// www.askrprojects.net
//

//--------------------------------------------------------------------------------------
// PINCTRL.C
//
// - additional pin and port control
// - take a look at the header file too!
//--------------------------------------------------------------------------------------
// (c)ASkr 2011

#ifndef _asPINCTRL
#define _asPINCTRL
#endif


#include <pic18f27j13.h>
#include "global.h"
#include "pinctrl.h"


//--------------------------------------------------------------------------------------
// GLOBAL VARIABLES






#ifdef DEBUGDEBUG
//**************************************************************************************
//***
//***
//***
//**************************************************************************************
void dbgTicks(uchar n)
{
	uchar i;
	for(i=0;i<n;i++)
	{
		dbgPin(1);
		dbgPin(0);
	}
}

#endif // END DEBUGDEBUG

