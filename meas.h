
#ifdef _asMEAS

#endif


#define WAITSTABLE 1		// determines acq. peripheral and bandgap stabilize wait time in 10ms units

#define STORAGESIZE 70	// number of samples (do not change!)

#define STOREMPTY  		   -1
#define STOROVERFLOW     -2

// AD CHANNELS
#define AD_POWOUT   0
#define AD_1        1
#define AD_2        2
#define AD_3        3
#define AD_4        4
#define AD_5        5
#define AD_6        6
#define AD_7        7
#define AD_8        8
#define AD_UBEXT    9
#define AD_VDDCORE 10
#define AD_BANDGAP 11

// prototypes
extern void measSelectChannel(uchar channel);
extern void measMeasureAll(void);
extern uint measGetChannelValue(uchar channel);
extern void measStorInit(void);
extern schar measStorWriteVMeas(void);
extern int measStorReadVMeas(void);
extern int measStorCount(void);
extern void measMeasureAllComplete(void);
extern unsigned int measGetChannelVoltage(uchar channel);

