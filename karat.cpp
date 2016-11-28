//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
//#include "mdm.h"
#include "modbus/modbus.h"
#include "karat.h"
#include "db.h"
#include "func.h"
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

extern 	"C" UINT device_num;	// total device quant
extern 	"C" UINT karat_num;	// quant
extern 	"C" BOOL WorkRegim;	// work regim flag
extern 	"C" tm *currenttime;	// current system time
extern  "C" BOOL karat_thread;

extern 	"C" DeviceR 	dev[MAX_DEVICE];	// device class
extern 	"C" DeviceKarat karat[MAX_DEVICE_NSP];	// Karat class
extern  "C" UINT	debug;

static 	union fnm fnum[5];

VOID 	ULOGW (const CHAR* string, ...);	// log function
UINT 	baudrate (UINT baud);			// baudrate select
static  UINT CRC (const BYTE* const Data, const BYTE DataSize);
static  UINT 	CalcCRC(unsigned int c,unsigned int crc);
static	unsigned short Crc16( unsigned char *pcBlock, unsigned short len );

static 	BOOL 	OpenCom (SHORT blok, UINT speed);	// open com function
static	BOOL 	LoadKaratConfig();			// load all configuration
static	BOOL 	ParseData (UINT type, UINT dv, BYTE* data, UINT len, UINT prm);
//-----------------------------------------------------------------------------
static modbus_param_t mb_param;
//-----------------------------------------------------------------------------
void * karatDeviceThread (void * devr)
{
 int devdk=*((int*) devr);			// Karat identificator
 dbase.sqlconn("","root","");			// connect to database

 LoadKaratConfig();
 // open port for work
 if (karat_num>0)
    {
     BOOL rs=OpenCom (karat[0].port, karat[0].speed);
    }
 else return (0);

 for (UINT r=0;r<karat_num;r++)  //karat_num
    {
     if (debug>0) ULOGW ("[307] Karat[%d/%d].ReadLastArchive ()",r,karat_num);
     karat[r].ReadVersion ();
     karat[r].ReadTime ();
     karat[r].ReadTime ();
    }

 sleep (5);
 while (WorkRegim)
    {
     for (UINT r=0;r<karat_num;r++)
        {
         if (debug>3) ULOGW ("[307] Karat[%d/%d].ReadLastArchive ()",r,karat_num);
	 //karat[r].ReadData (CURRENT_ARCHIVE,1);
	 //karat[r].ReadData (INT_ARCHIVE,100);
	 karat[r].ReadData (HOUR_ARCHIVE,5);
	 //karat[r].ReadData (DAY_ARCHIVE,90);
	 //if (currenttime->tm_mday<15)	Karat[r].ReadData (MONTH_ARCHIVE,2);
	 sleep (1);
	}
     sleep (1);
    }
 dbase.sqldisconn();
 modbus_close(&mb_param);
 karat_thread=FALSE;
}

//--------------------------------------------------------------------------------------
// load all IRP configuration from DB
BOOL LoadKaratConfig()
{
 sprintf (query,"SELECT * FROM dev_karat");
 res=dbase.sqlexec(query);
 UINT nr=mysql_num_rows(res);
 for (UINT r=0;r<nr;r++)
    {
     row=mysql_fetch_row(res);
     karat[karat_num].idkarat=atoi(row[0]);
     karat[karat_num].device=atoi(row[1]);
     for (UINT d=0;d<device_num;d++)
        {
         if (dev[d].idd==karat[karat_num].device)
            {
    	     karat[karat_num].iddev=dev[d].id;
	     karat[karat_num].SV=dev[d].SV;
    	     karat[karat_num].interface=dev[d].interface;
	     karat[karat_num].protocol=dev[d].protocol;
	     karat[karat_num].port=dev[d].port;
	     karat[karat_num].speed=dev[d].speed;
	     karat[karat_num].adr=dev[d].adr;
	     karat[karat_num].type=dev[d].type;
	     strcpy(karat[karat_num].number,dev[d].number);
	     karat[karat_num].flat=dev[d].flat;
	     karat[karat_num].akt=dev[d].akt;
	     strcpy(karat[karat_num].lastdate,dev[d].lastdate);
	     karat[karat_num].qatt=dev[d].qatt;
	     karat[karat_num].qerrors=dev[d].qerrors;
	     karat[karat_num].conn=dev[d].conn;
	     strcpy(karat[karat_num].devtim,dev[d].devtim);
	     karat[karat_num].chng=dev[d].chng;
	     karat[karat_num].req=dev[d].req;
	     karat[karat_num].source=dev[d].source;
	     strcpy(karat[karat_num].name,dev[d].name);
	    }
	}
     karat_num++;
    } 
 if (res) mysql_free_result(res);
 if (debug>0) ULOGW ("[307] total %d karat add to list",karat_num);
}
//---------------------------------------------------------------------------------------------------
bool DeviceKarat::ReadVersion ()
{
 int 	rs,result=0;
 BYTE	data[8000];
 BYTE	data_in[300];

// result = read_holding_registers(&mb_param, this->adr, 0x708, 1, (uint16_t *)data);
 result = read_holding_registers(&mb_param, this->adr, 0x101, 27, (uint16_t *)data);
 if (result>=0)
    {
     //if (debug>0) ULOGW ("[307] Karat-307 [SN:%x %x %x %x %x %x %x %x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
     if (debug>0) ULOGW ("[307] Karat-307 [SN:%c%c%c%c%c%c%c%c]",data[0],data[3],data[2],data[5],data[4],data[7],data[6],data[9]);
    }
 else ULOGW ("[307] no Karat found");
 return true;
}
//---------------------------------------------------------------------------------------------------
bool DeviceKarat::ReadTime ()
{
 int 	rs,result=0;
 BYTE	data[8000];
 BYTE	data_in[300];

 result = read_holding_registers(&mb_param, this->adr, 0x62, 4, (uint16_t *)data);
 if (result>=0)
    {
     sprintf (this->devtim,"%d%d%d%d%d%d",data[6]*256+data[5],data[4],data[3],data[2],data[1],data[0]);
     //if (debug>0) ULOGW ("[307] Karat time [%x %x %x %x %x %x %x %x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
     //11-25 12:07:16 [307] Karat time [e 0 19 d b 5 7 e0]
     if (debug>0) ULOGW ("[307] Karat time [%02d-%02d-%d %02d:%02d:00]",data[2],data[4],data[6]*256+data[7],data[3],data[0]);
    }
 else ULOGW ("[307] no Karat found");    

 return true;
}

//---------------------------------------------------------------------------------------------------
// ReadLastArchive - read last archive and store to db
int DeviceKarat::ReadData (UINT	type, UINT deep)
{
 int 	rs,result=0,sm=0;
 BYTE	data[8000];
 BYTE	data_in[4],typ=0;
 CHAR	date[20];
 FLOAT	q1=0, q2=0, qp=0, t1, t2, v1, v2, v3, p1, p2;

// struct tm *currenttime2;
 struct tm *currenttime2=(tm *)malloc (sizeof (struct tm));

 BYTE	hour=0;
 time_t tim;
 time(&tim);
 if (type==HOUR_ARCHIVE) tim-=3600*2;
 if (type==DAY_ARCHIVE) tim-=3600*24;
// currenttime2=localtime(&tim);
// localtime_r(&tim,currenttime2);

 while (deep)
    {
     rs=type;
     //result = preset_single_register(&mb_param, this->adr, ADR_ARCHTYPE, rs);
     if (type>0)
        {
	 //   04 11 1A 0B 10
         //my 04 10 0B 1A 0B
	 localtime_r(&tim,currenttime2);
	 data_in[3]=currenttime2->tm_mon+1; data_in[2]=(currenttime2->tm_year-100);
	 hour=currenttime2->tm_hour;
	 if (type==DAY_ARCHIVE || type==INT_ARCHIVE) hour=0;
	 data_in[1]=hour; data_in[0]=(currenttime2->tm_mday);

         ULOGW ("[307] request(%d) [%d] [%02d-%02d-%02d %02d:00:00] [%x]",deep,type,currenttime2->tm_mday,currenttime2->tm_mon+1,currenttime2->tm_year-100,hour);
	 if (type==HOUR_ARCHIVE) tim=tim-3600;
	 if (type==DAY_ARCHIVE) tim-=3600*24;
	 if (type==INT_ARCHIVE) tim-=3600*24;
        }
     rs=3;
     while (rs)
        {
	 if (type>0) result = preset_multiple_registers(&mb_param, this->adr, REQUEST_DATA, 2, (uint16_t *)data_in);
         if (result>=0) break;
         rs--;
        }
     sleep (1);
     rs=3;
     while (rs)
        {
	 if (type==HOUR_ARCHIVE) result = read_holding_registers(&mb_param, this->adr, ADR_HOUR, 120, (uint16_t *)data);
	 if (type==DAY_ARCHIVE) result = read_holding_registers(&mb_param, this->adr, ADR_DAY, 120, (uint16_t *)data);
	 if (type==INT_ARCHIVE) result = read_holding_registers(&mb_param, this->adr, ADR_INT, 120, (uint16_t *)data);
	 if (type==CURRENT_ARCHIVE)  result = read_holding_registers(&mb_param, this->adr, ADR_DATA_CURRENT, 110, (uint16_t *)data);
         if (result>=0) break;
         rs--;
        }

     this->qatt++;	// attempt
     if (result<0)
	{
         this->qerrors++;
         this->conn=0;
	 if (this->qerrors>5) this->akt=0; // !!! 5 > normal quant from DB
//         if (this->qerrors==6) Events ((3*0x10000000)|(TYPE_IRP*0x1000000)|SEND_PROBLEM,this->device);
        }
     else
	{
	 if (type==CURRENT_ARCHIVE) sm=0;
	 else sm=18;
    	 data_in[0]=data[1+sm]; data_in[1]=data[0+sm]; data_in[2]=data[3+sm]; data_in[3]=data[2+sm];
         t1=*(float*)(data_in);
	 data_in[0]=data[5+sm]; data_in[1]=data[4+sm]; data_in[2]=data[7+sm]; data_in[3]=data[6+sm];
	 t2=*(float*)(data_in);
	 data_in[0]=data[13+sm]; data_in[1]=data[12+sm]; data_in[2]=data[15+sm]; data_in[3]=data[14+sm];
	 p1=*(float*)(data_in);
	 data_in[0]=data[17+sm]; data_in[1]=data[16+sm]; data_in[2]=data[19+sm]; data_in[3]=data[18+sm];
         p2=*(float*)(data_in);
	 data_in[0]=data[21+sm]; data_in[1]=data[20+sm]; data_in[2]=data[23+sm]; data_in[3]=data[22+sm];
	 v1=*(float*)(data_in);
	 data_in[0]=data[25+sm]; data_in[1]=data[24+sm]; data_in[2]=data[27+sm]; data_in[3]=data[26+sm];
         v2=*(float*)(data_in);

	 data_in[0]=data[33+sm]; data_in[1]=data[32+sm]; data_in[2]=data[35+sm]; data_in[3]=data[34+sm];
	 q1=*(float*)(data_in);
	 data_in[0]=data[37+sm]; data_in[1]=data[36+sm]; data_in[2]=data[39+sm]; data_in[3]=data[38+sm];
         q2=*(float*)(data_in);
	 data_in[0]=data[45+sm]; data_in[1]=data[44+sm]; data_in[2]=data[47+sm]; data_in[3]=data[46+sm];
         qp=*(float*)(data_in);

	 //for (rs=0;rs<70;rs++) ULOGW("[%d]=%x",rs,data[rs]);
	 // 11-26 13:48:47 [14]=1a  25
	 // 11-26 13:48:47 [15]=9   9
	 // 11-26 13:48:47 [16]=10  16 
	 // 11-26 13:48:47 [17]=b   11
         if (type==CURRENT_ARCHIVE || type==INT_ARCHIVE) if (debug>2) ULOGW ("[307] q1=%f, q2=%f, qp=%f, t1=%f, t2=%f, p1=%f, p2=%f, v1=%f, v2=%f",q1,q2,qp,t1,t2,p1,p2,v1,v2);
         if (type>CURRENT_ARCHIVE && type<INT_ARCHIVE) if (debug>2) ULOGW ("[307][%d] [%02d-%02d-%02d %02d:00:00] q=%f, t1=%f, t2=%f, v1=%f, v2=%f %f",type,data[14],data[17],data[16],data[15],qp,t1,t2,v1,v2,v3);
         if (type)
	    {
    	     if (type==0) sprintf (date,"%04d%02d%02d%02d%02d00",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min);
	     if (type>0)
		    {
    		     if (type==INT_ARCHIVE) sprintf (date,"%04d%02d%02d000000",data[16]+2000,data[17],data[14]);
    		     if (type==HOUR_ARCHIVE) sprintf (date,"%04d%02d%02d%02d0000",data[16]+2000,data[17],data[14],data[15]);
    		     if (type==DAY_ARCHIVE) sprintf (date,"%04d%02d%02d000000",data[16]+2000,data[17],data[14]);
    		     if (type==MONTH_ARCHIVE) sprintf (date,"%04d%02d01000000",data[16]+2000,data[17]);
    		     //printf ("date=%s\n",date);
    		     //if (data[1]<7 || data[1]>0) { deep--; continue; }
    		     //if (data[0]>12) continue;
		    }
	     //if (type==1 && (data[1]>20)) {deep--; continue; }
	    /*
             StoreData (dbase,this->device, 13, 0, type, 0, q1, date, 1);
             StoreData (dbase,this->device, 13, 1, type, 0, q2, date, 1);
             StoreData (dbase,this->device, 13, 2, type, 0, qp, date, 1);
             StoreData (dbase,this->device, 11, 0, type, 0, v1, date, 1);
             StoreData (dbase,this->device, 11, 1, type, 0, v2, date, 1);
	     StoreData (dbase,this->device, 4,  0, type, 0, t1, date, 1);
             StoreData (dbase,this->device, 4,  1, type, 0, t2, date, 1);
    	     StoreData (dbase,this->device, 16, 0, type, 0, p1, date, 1);
    	     StoreData (dbase,this->device, 16, 1, type, 0, p2, date, 1);
*/
	    }
         else
	    {
                 StoreData (dbase,this->device, 11, 0, 0, v1, 0);
    		 StoreData (dbase,this->device, 11, 1, 0, v2, 0);
    	         StoreData (dbase,this->device, 13, 0, 0, q1, 0);
        	 StoreData (dbase,this->device, 13, 1, 0, q2, 0);
	         StoreData (dbase,this->device, 4, 0, 0, t1, 0);
    		 StoreData (dbase,this->device, 4, 1, 0, t2, 0);
	         StoreData (dbase,this->device, 16, 0, 0, p1, 0);
    		 StoreData (dbase,this->device, 16, 1, 0, p2, 0);
		 StoreData (dbase,this->device, 13, 2, 0, qp, 0);
	    }
	}
     this->akt=1;
     this->qerrors=0;
     this->conn=0;
     deep--;
     //sleep (5);
    }
// ULOGW ("[307]free");
 free (currenttime2);
 return 0;
}
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 sprintf (devp,"/dev/ttyS%d",blok);
 sprintf (devp,"/dev/ttyS0",blok);
// sprintf (devp,"/dev/ttyUSB0",blok);
 modbus_init_rtu(&mb_param, devp, speed, "none", 8, 1);
 modbus_set_debug(&mb_param, FALSE);
 if (modbus_connect(&mb_param) == -1) 
    {
     if (debug>0) ULOGW("[307] error open com-port %s",devp); 
     return FALSE;
    }
 else if (debug>1) ULOGW("[307] open com-port %s success",devp); 
 return TRUE;
}
//--------------------------------------------------------------------------------------------------------------------
static unsigned int CalcCRC(unsigned int c,unsigned int crc)
{
 int count,flg;
 for (count=0;count<8;count++)
    {
     flg=crc&0x8000;
     crc<<=1;
     if(((c)&0x80)!=0)crc+=1;
	if(flg!=0)crc^=0x1021;
	    c<<=1;
    }
 return crc;
}
//--------------------------------------------------------------------------------------------------------------------
static unsigned short Crc16( unsigned char *pcBlock, unsigned short len )
{
 unsigned short crc = 0xFFFF;
 unsigned char i;
 while( len-- )
    {
     crc ^= *pcBlock++ << 8;
     for( i = 0; i < 8; i++ )
        crc = crc & 0x8000 ? ( crc << 1 ) ^ 0x1021 : crc << 1;
    }
 return crc;
}
