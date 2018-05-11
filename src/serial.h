
#define NUMERR_LONG     0x80000000
#define MAX_LONG        2147483647

#define NONBLOCKING     0
#define BLOCKING        1

#define NOEXEC          0
#define EXEC            1

#define RXBUFEMPTY     -1
#define RXOVERFLOW     -2

#define RXBUFSIZE      32
#define COMMANDCOUNT   41		// must match COMMANDS and gCOMMANDS below




// this needs to match pCOMMANDS array below, except
// the last two entries "NONE" and "ERROR"
enum COMMANDS
{
	CMD_HELP = 0,
	CMD_OFF,
	CMD_START,
	CMD_STOP,
	CMD_SHOW,
	CMD_LOAD,
	CMD_SAVE,
	CMD_LOGDATESET,
	CMD_LOGDATEQUERY,
	CMD_LOGTIMESET,
	CMD_LOGTIMEQUERY,
	CMD_DATILASTSET,
	CMD_DATILASTQUERY,
	CMD_LOGEXCORESET,
	CMD_LOGEXCOREQUERY,
	CMD_TIMESET,
	CMD_TIMEQUERY,
	CMD_DATESET,
	CMD_DATEQUERY,
	CMD_SLEEPSET,
	CMD_SLEEPQUERY,
	CMD_SRATESET,
	CMD_SRATEQUERY,
	CMD_SMEANSSET,
	CMD_SMEANSQUERY,
	CMD_SUNITSET,
	CMD_SUNITQUERY,
	CMD_SERIALLOGSET,
	CMD_SERIALLOGQUERY,
	CMD_SEPSET,
	CMD_SEPQUERY,
	CMD_FNAMESET,
	CMD_FNAMEQUERY,
	CMD_USECARDSET,
	CMD_USECARDQUERY,
	CMD_BATQUERY,
	CMD_MEASNOW,
	CMD_MEASVOL,
	CMD_VERSIONQUERY,
	CMD_BGCORRSET,
	CMD_BGCORRQUERY,
	CMD_NONE,
	CMD_ERROR
};




#ifdef _asSERIAL

// SourceBoost can not handle two dimensional arrays in ROM...
// Note 1: This needs to match "COMMANDS" enum above...
// Note 2: Due to the internal, "fast" (aka.: q&d) string compare
//         algorithm, commands containing substrings of other commands
//         are required to appear before "their simpler versions".
//         (E.g.: "logtime" preceeds "time").
volatile uchar gCOMMANDS[COMMANDCOUNT+1][10]={
	"help",
	"off",
	"start",
	"stop",
	"show",
	"load",
	"save",
	"logdate=",
	"logdate?",
	"logtime=",
	"logtime?",
	"dati2nd=",
	"dati2nd?",
	"logxcor=",
	"logxcor?",
	"time=",
	"time?",
	"date=",
	"date?",
	"sleep=",
	"sleep?",
	"srate=",
	"srate?",
	"smeans=",
	"smeans?",
	"sunit=",
	"sunit?",
	"serial=",
	"serial?",
	"sep=",
	"sep?",
	"fname=",
	"fname?",
	"usecard=",
	"usecard?",
	"bat?",
	"measraw=",
	"measvol=",
	"version?",
	"bgcorr=",
	"bgcorr?",
	NULL
	};

#endif



// functions for sending out to TX
extern uchar serSendByte(uchar ch);
extern void serSendString(uchar *ch);
extern void serFlushOut(void);
extern void serSendStringDebug(uchar *ch1, long val, uchar *ch2);


// functions for reading from RX
extern void serRXBufInit(void);
extern int serRXBufWriteChar(uchar ch, uchar echo); // called from ISR
extern int serRXBufReadChar(int blocked);
extern void serRXBufFlushIn(void);
extern int serRXBufCount(void);

// functions evaluating command sequences
extern uchar cmdCheck(uchar exec);

// utilities
extern uchar *lltoa(uchar *buffer, long i);
extern void serUsage(void);
extern long serMatRead(uchar *str, uchar ind, uchar sep);
extern uchar cmdCheckStringAlphaNum(uchar *str);
extern void serShowParams(void);



