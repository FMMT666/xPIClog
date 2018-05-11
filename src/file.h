
#ifdef _asFILE

#endif


// Time, after which data is written to SD-card (time equal or greater).
// Will be rounded up to the next, possible value.
// NOTE:
// STORAGESIZE in meas.h must be large enough to hold all these values!
#define WRITEAFTERSECONDS   60	// max: 70
#define WRITEAFTERMINUTES   5		// max: 70
#define WRITEAFTERHOURS     1		// unused



extern schar filOpenAppendClose(uchar *fname);
extern schar filParseConfig(uchar *fname);
extern void filReadConfig(uchar *fname);
extern schar filSaveConfig(uchar *fname);
