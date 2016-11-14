//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "elf.h"
#include "db.h"
//-----------------------------------------------------------------------------
static	MYSQL_RES 	*res;
static	db		dbase;
static 	MYSQL_ROW 	row;

static 	CHAR   		query[500];
static	UINT	data_adr;	// for test we remember what we send to device
static	UINT	data_len;	// data address and lenght
static	UINT	data_op;	// and last operation

extern 	"C" UINT device_num;	// total device quant
extern 	"C" UINT elf_num;	// LK sensors quant
extern 	"C" BOOL WorkRegim;	// work regim flag
extern 	"C" tm *currenttime;	// current system time

extern 	"C" DeviceR 	dev[MAX_DEVICE];	// device class
extern 	"C" DeviceELF 	elf[MAX_DEVICE_ELF];	// ELF class

static 	union fnm fnum[5];

VOID 	ULOGW (CHAR* string, ...);		// log function
UINT 	baudrate (UINT baud);			// baudrate select
static  UINT CRC (const BYTE* const Data, const BYTE DataSize);
UINT 	CalcCRC(unsigned int c,unsigned int crc);
VOID 	Events (DWORD evnt, DWORD device);	// events 

static 	BOOL 	OpenCom (SHORT blok, UINT speed);	// open com function
static	BOOL 	LoadELFConfig();			// load all irp configuration
//static	VOID	StartSimulate(UINT type); 
static	BOOL 	ParseData (UINT type, UINT dv, BYTE* data, UINT len, UINT prm);

static	BOOL 	StoreData (UINT dv, UINT prm, UINT type, UINT status, FLOAT value, UINT pipe, BYTE* data);
static  BOOL 	StoreData (UINT dv, UINT prm, UINT status, FLOAT value, UINT pipe);
//-----------------------------------------------------------------------------
void * elfmDeviceThread (void * devr)
{
 int devdk=*((int*) devr);			// DK identificator
 dbase.sqlconn("","root","");			// connect to database
 LoadELFmConfig();

 sleep (5);
 while (WorkRegim)
    {
     for (UINT r=0;r<elf_numm;r++)
        {
         if (debug>3) ULOGW ("[elf] elf[%d/%d].ReadLastArchive ()",r,elf_num);
	 elfm[r].ReadData (0);
	 if (currenttime->tm_min<10) 	elf[r].ReadData (1);
	 if (currenttime->tm_hour<12)	elf[r].ReadData (2);
	 if (currenttime->tm_mday<10)	elf[r].ReadData (3);
	 sleep (3);
	}
     sleep (1);     
    }
 dbase.sqldisconn();
}

//--------------------------------------------------------------------------------------
BOOL LoadELFmConfig()
{
 sprintf (query,"SELECT * FROM dev_elf");
 res=dbase.sqlexec(query); 
 UINT nr=mysql_num_rows(res);
 for (UINT r=0;r<nr;r++)
    {
     row=mysql_fetch_row(res);
     elfm[elf_numm].idelf=atoi(row[0]);
     elfm[elf_numm].device=atoi(row[1]);     
     for (UINT d=0;d<device_num;d++)
        {
         if (dev[d].idd==elfm[elf_numm].device)
            {
    	     elfm[elf_numm].iddev=dev[d].id;
	     elfm[elf_numm].SV=dev[d].SV;
    	     elfm[elf_numm].interface=dev[d].interface;
	     elfm[elf_numm].protocol=dev[d].protocol;
	     elfm[elf_numm].port=dev[d].port;
	     elfm[elf_numm].speed=dev[d].speed;
	     elfm[elf_numm].adr=dev[d].adr;
	     elfm[elf_numm].type=dev[d].type;
	     strcpy(elfm[elf_numm].number,dev[d].number);
	     elfm[elf_numm].flat=dev[d].flat;
	     elfm[elf_numm].akt=dev[d].akt;
	     strcpy(elfm[elf_numm].lastdate,dev[d].lastdate);
	     elfm[elf_numm].qatt=dev[d].qatt;
	     elfm[elf_numm].qerrors=dev[d].qerrors;
	     elfm[elf_numm].conn=dev[d].conn;
	     strcpy(elfm[elf_numm].devtim,dev[d].devtim);
	     elfm[elf_numm].chng=dev[d].chng;
	     elfm[elf_numm].req=dev[d].req;
	     elfm[elf_numm].source=dev[d].source;
	     strcpy(elfm[elf_num].name,dev[d].name);
	    }
	}
     elfm_num++;
    } 
 if (res) mysql_free_result(res);
 if (debug>0) ULOGW ("[elf] total %d ELF throw ModBus add to list",elf_numm);
}
//---------------------------------------------------------------------------------------------------
// ReadLastArchive - read last archive and store to db
int DeviceModbus::ReadData (UINT type)
{
 return 0;
}
//---------------------------------------------------------------------------------------------------
int DeviceModbus::read_modbus (BYTE* dat)
{
 return 0;
}
//---------------------------------------------------------------------------------------------------
bool DeviceModbus::send_modbus (UINT op, UINT reg, UINT nreg, UCHAR* dat)
{
 if (debug>3) ULOGW ("[elf] send_elf (%d, %d, %d)",op,reg,nreg);
 return true;
}
//---------------------------------------------------------------------------------------------------
BOOL StoreData (UINT dv, UINT prm, UINT status, FLOAT value, UINT pipe)
{
 sprintf (query,"SELECT * FROM prdata WHERE type=0 AND prm=%d AND device=%d AND pipe=%d",prm,dv,pipe);
 res=dbase.sqlexec(query); 
 if (row=mysql_fetch_row(res))
     sprintf (query,"UPDATE prdata SET value=%f,status=%d WHERE type=0 AND prm=%d AND device=%d AND pipe=%d",value,status,prm,dv,pipe);
 else sprintf (query,"INSERT INTO prdata(device,prm,type,value,status,pipe) VALUES('%d','%d','0','%f','%d','%d')",dv,prm,value,status,pipe);
 if (debug>3) ULOGW ("%s [%d]",query,row);
 if (res) mysql_free_result(res); 
 res=dbase.sqlexec(query); 
 if (res) mysql_free_result(res);
 return true;
}
//---------------------------------------------------------------------------------------------------
BOOL StoreData (UINT dv, UINT prm, UINT type, UINT status, FLOAT value, UINT pipe, BYTE* data)
{
 CHAR date[30];
 if (type==0 || type==1) sprintf (date,"%04d%02d%02d%02d0000",currenttime->tm_year+1900,data[1],data[0],data[2]);
 if (type==2) sprintf (date,"%04d%02d%02d000000",data[2]+2000,data[1],data[0]);
 if (debug>3)  ULOGW ("[elf][%d] date=%s (%f)",prm,date,value);
 
 if (type==1 && (data[1]>12)) return false;
 if (type==2 && (data[2]<9)) return false;
 if (value>10000000) return false;
 
 //if (type==1) if (data[3]<108 || data[3]>128) return false;
 //if (type==2) if (data[7]<108 || data[7]>128) return false;
 sprintf (query,"SELECT * FROM prdata WHERE type=%d AND prm=%d AND device=%d AND date=%s AND pipe=%d",type,prm,dv,date,pipe);
 if (debug>3) ULOGW ("[elf] %s",query);
 res=dbase.sqlexec(query); 
 if (row=mysql_fetch_row(res))
    {
     //if (value>0) sprintf (query,"UPDATE prdata SET value=%f,status=%d,date=date WHERE type='%d' AND prm=%d AND device='%d' AND date='%s' AND pipe='%d'",value,status,type,prm,dv,date,pipe);
     //else
     if (1)
        {
	 if (res) mysql_free_result(res); 
	 return true;
	}     
    }
 else sprintf (query,"INSERT INTO prdata(device,prm,type,value,status,date,pipe) VALUES('%d','%d','%d','%f','%d','%s','%d')",dv,prm,type,value,status,date,pipe);

 if (res) mysql_free_result(res); 
 res=dbase.sqlexec(query); 
 if (res) mysql_free_result(res);
 return true;
}
