
// DEBUG pin
#ifdef DEBUGDEBUG
#ifndef dbgPin
#define dbgPin(dState)		latc.4 = dState
#endif
extern void dbgTicks(unsigned char n);
#endif // END DEBUG DEBUG


// LED
#ifndef LED
#define LED(dState)		lata.7 = dState
#endif


// pushbutton (also mapped to INT1 via RP14)
#ifndef SWITCH
#define SWITCH							portc.3
#endif


// SDPOW (1=ON, 0=OFF)
#ifndef SDPOW
#define SDPOW(dState)		latc.5 = !dState
#endif


// POWOUT (1=ON, 0=OFF)
#ifndef POWOUT
#define POWOUT(dState)		lata.6 = !dState
#endif


// BANDGAP (1=ON, 0=OFF)
#ifndef BANDGAP
#define BANDGAP(dState)		ancon1.VBGEN = dState
#endif

