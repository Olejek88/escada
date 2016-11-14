//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "modbus/modbus.h"
#include "panel.h"
#include "db.h"
//-----------------------------------------------------------------------------
static	MYSQL_RES 	*res;
static	db		dbase;
static 	MYSQL_ROW 	row;

static 	CHAR   		query[500];
static 	INT		fd;
static 	termios 	tio;

static	UINT	data_adr;	// for test we remember what we send to device
static	UINT	data_len;	// data address and lenght
static	UINT	data_op;	// and last operation

extern 	"C" BOOL WorkRegim;	// work regim flag
extern 	"C" tm *currenttime;	// current system time

static 	union fnm fnum[5];

VOID    ULOGW (const CHAR* string, ...);        // log function
UINT 	baudrate (UINT baud);			// baudrate select
float	Qdom=0,Qst=0,Qpr=0;

extern  "C" UINT	debug;
//-----------------------------------------------------------------------------
static 	modbus_param_t mb_param;
//-----------------------------------------------------------------------------
void * vpDeviceThread (void * devr)
{
 int devdk=*((int*) devr);
 BYTE		data[8000];	// output buffer
 uint16_t 	cn=0,chan=0;
 float rt=0;
 float	val;
 dbase.sqlconn("","root","");			// connect to database

 modbus_init_tcp(&mb_param, "192.168.255.1", 502);

 modbus_set_debug(&mb_param, TRUE);
      
 while (modbus_connect(&mb_param) == -1) {
        ULOGW ("[vp] error: connection failed\n");
        sleep (10);
    }

 sleep (5);
 while (WorkRegim)
    {
     ret = read_holding_registers(&mb_param, SLAVE, START_MAIN_PARAMS, 100, data);
     for (cn=0; cn<sizeof (viewpac)/sizeof(viewpac[0]); cn++)
        {
         val=*(float *)data[viewpac[cn].adr*2];
	 ULOGW ("[vp] dP=%f",val);
         // function store current data to database
	 chan=GetChannelNum (dbase,14,0,this->device);
         StoreData (db dbase, UINT dv, UINT prm, UINT pipe, UINT status, FLOAT value, uint8_t dest, uint16_t chan)
	}
     sleep (3);
    }
 dbase.sqldisconn();
 modbus_close(&mb_param);

 if (debug>0) ULOGW ("[vp] viewpac thread end");
 panel_thread=FALSE;
 pthread_exit (NULL);
}
