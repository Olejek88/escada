//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
//#include "mdm.h"
#include "modbus/modbus.h"
#include "elf2.h"
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
extern 	"C" UINT elf_num;	// LK sensors quant
extern 	"C" BOOL WorkRegim;	// work regim flag
extern 	"C" tm *currenttime;	// current system time
extern  "C" BOOL 	elf_thread;

extern 	"C" DeviceR 	dev[MAX_DEVICE];	// device class
extern 	"C" DeviceELF 	elf[MAX_DEVICE_ELF];	// ELF class
extern  "C" UINT	debug;

static 	union fnm fnum[5];

VOID 	ULOGW (const CHAR* string, ...);		// log function
UINT 	baudrate (UINT baud);			// baudrate select
static  UINT CRC (const BYTE* const Data, const BYTE DataSize);
static  UINT 	CalcCRC(unsigned int c,unsigned int crc);
//VOID 	Events (DWORD evnt, DWORD device);	// events 
static	unsigned short Crc16( unsigned char *pcBlock, unsigned short len );

static 	BOOL 	OpenCom (SHORT blok, UINT speed);	// open com function
static	BOOL 	LoadELFConfig();			// load all irp configuration
//static	VOID	StartSimulate(UINT type); 
static	BOOL 	ParseData (UINT type, UINT dv, BYTE* data, UINT len, UINT prm);

//static	BOOL 	StoreData (UINT dv, UINT prm, UINT type, UINT status, FLOAT value, UINT pipe, BYTE* data);
//static  BOOL 	StoreData (UINT dv, UINT prm, UINT status, FLOAT value, UINT pipe);
//-----------------------------------------------------------------------------
modbus_param_t mb_param;
//-----------------------------------------------------------------------------
void * elfDeviceThread (void * devr)
{
 int devdk=*((int*) devr);			// DK identificator
 dbase.sqlconn("","root","");			// connect to database
 // fill archive with fixed values
 // load from db all irp devices > store to class members
 LoadELFConfig();
 // open port for work
 if (elf_num>0)
    {
     BOOL rs=OpenCom (elf[0].port, elf[0].speed);
    }
 else return (0);

 for (UINT r=0;r<elf_num;r++)  //elf_num
    {
     if (debug>0) ULOGW ("[elf] elf[%d/%d].ReadLastArchive ()",r,elf_num);
     elf[r].ReadVersion ();
     elf[r].ReadTime ();
    }
// for (UINT r=0;r<19;r++) elf[0].ReadTime ();

 sleep (5);
 while (WorkRegim)
    {
     for (UINT r=0;r<elf_num;r++)
     if (elf[r].adr==1 || elf[r].adr==2 || elf[r].adr==4 || elf[r].adr==5)
//     if (elf[r].adr==5)
        {
         if (debug>3) ULOGW ("[elf] elf[%d/%d].ReadLastArchive ()",r,elf_num);
	 elf[r].ReadData (CURRENT_ARCHIVE,1);
//	 elf[r].ReadData (INT_ARCHIVE,100);
	 elf[r].ReadData (HOUR_ARCHIVE,5);
	 elf[r].ReadData (DAY_ARCHIVE,90);
	 //if (currenttime->tm_mday<15)	elf[r].ReadData (MONTH_ARCHIVE,2);
	 sleep (1);
	}
     sleep (1);
    }
 dbase.sqldisconn();
 modbus_close(&mb_param);
 elf_thread=FALSE;
}

//--------------------------------------------------------------------------------------
// load all IRP configuration from DB
BOOL LoadELFConfig()
{
 sprintf (query,"SELECT * FROM dev_elf");
 res=dbase.sqlexec(query); 
 UINT nr=mysql_num_rows(res);
 for (UINT r=0;r<nr;r++)
    {
     row=mysql_fetch_row(res);
     elf[elf_num].idelf=atoi(row[0]);
     elf[elf_num].device=atoi(row[1]);     
     for (UINT d=0;d<device_num;d++)
        {
         if (dev[d].idd==elf[elf_num].device)
            {
    	     elf[elf_num].iddev=dev[d].id;
	     elf[elf_num].SV=dev[d].SV;
    	     elf[elf_num].interface=dev[d].interface;
	     elf[elf_num].protocol=dev[d].protocol;
	     elf[elf_num].port=dev[d].port;
	     elf[elf_num].speed=dev[d].speed;
	     elf[elf_num].adr=dev[d].adr;
	     elf[elf_num].type=dev[d].type;
	     strcpy(elf[elf_num].number,dev[d].number);
	     elf[elf_num].flat=dev[d].flat;
	     elf[elf_num].akt=dev[d].akt;
	     strcpy(elf[elf_num].lastdate,dev[d].lastdate);
	     elf[elf_num].qatt=dev[d].qatt;
	     elf[elf_num].qerrors=dev[d].qerrors;
	     elf[elf_num].conn=dev[d].conn;
	     strcpy(elf[elf_num].devtim,dev[d].devtim);
	     elf[elf_num].chng=dev[d].chng;
	     elf[elf_num].req=dev[d].req;
	     elf[elf_num].source=dev[d].source;
	     strcpy(elf[elf_num].name,dev[d].name);
	    }
	}
     elf_num++;
    } 
 if (res) mysql_free_result(res);
 if (debug>0) ULOGW ("[elf] total %d ELF add to list",elf_num);
}
//---------------------------------------------------------------------------------------------------
bool DeviceELF::ReadVersion ()
{
 int 	rs,result=0;
 BYTE	data[8000];
 BYTE	data_in[300];

 result = read_input_registers(&mb_param, this->adr, ADR_NUMBER, 4, (uint16_t *)data);
 if (result>=0)
    {     
     sprintf (this->version,"%d%d%d%d%d%d%d%d",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
     if (debug>0) ULOGW ("[elf] ELF [%s] software",this->version);
    }
 else ULOGW ("[elf] no ELF found");    
 return true;
}
//---------------------------------------------------------------------------------------------------
bool DeviceELF::ReadTime ()
{
 int 	rs,result=0;
 BYTE	data[8000];
 BYTE	data_in[300];

 result = read_input_registers(&mb_param, this->adr, ADR_TIME, 3, (uint16_t *)data);
 if (result>=0)
    {     
     sprintf (this->devtim,"%d%d%d%d%d%d%d%d",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
     if (debug>0) ULOGW ("[elf] ELF time [%02d-%02d-%d %02d:%02d:00]",data[3],data[0],data[1]+2000,data[2],data[5]);
    }
 else ULOGW ("[elf] no ELF found");    
 return true;
}

//---------------------------------------------------------------------------------------------------
// ReadLastArchive - read last archive and store to db
int DeviceELF::ReadData (UINT	type, UINT deep)
{
 int 	rs,result=0;
 BYTE	data[8000];
 BYTE	data_in[4],typ=0;
 CHAR	date[20];
 FLOAT	q, q2, t1, t2, v1, v2,v3;

// struct tm *currenttime2;
 struct tm *currenttime2=(tm *)malloc (sizeof (struct tm));

 BYTE	hour=0;
 time_t tim;
 time(&tim);
 if (type==HOUR_ARCHIVE) tim-=3600*2;
 if (type==DAY_ARCHIVE) tim-=3600*24;
 //currenttime2=localtime(&tim);
// localtime_r(&tim,currenttime2);

 while (deep)
    {
     //1.	Установить в регистр Архив(40001 + 3) значение соответствующее требуемому архиву.
     rs=type;
     data_in[12]=0; data_in[13]=0;
     data_in[10]=0; data_in[11]=0;
     data_in[8]=0; data_in[9]=0;
     data_in[6]=rs; data_in[7]=0;
     data_in[4]=0; data_in[5]=0;
     data_in[2]=0; data_in[3]=0;
     data_in[0]=0; data_in[1]=0;
     //result = preset_single_register(&mb_param, this->adr, ADR_ARCHTYPE, rs);
     if (type>9)
        {
         //2.	Заполните поля ГОД, МАСЯЦ, ДЕНЬ, ЧАС(40001+0 - 40001+1) значениями, соответствующими дате требуемой архивной записи. 
	 localtime_r(&tim,currenttime2);
         //rs=(currenttime2->tm_year-100)*256+currenttime2->tm_mon+1;
	 data_in[0]=currenttime2->tm_mon+1; data_in[1]=(currenttime2->tm_year-100);
         //result = preset_single_register(&mb_param, this->adr, ADR_YEAR, rs);
	 hour=currenttime2->tm_hour;
	 if (type==DAY_ARCHIVE || type==INT_ARCHIVE) hour=0;
//         else rs=(currenttime2->tm_mday*256)+hour;
	 data_in[2]=hour; data_in[3]=(currenttime2->tm_mday);
         ULOGW ("[elf] request(%d) [%d] [%02d-%02d-%02d %02d:00:00] [%x]",deep,type,currenttime2->tm_mday,currenttime2->tm_mon+1,currenttime2->tm_year-100,hour);
	 //result = preset_single_register(&mb_param, this->adr, ADR_YEAR+1, rs);
         //result = preset_multiple_registers(&mb_param, this->adr, ADR_YEAR, 2, (uint16_t *)data_in);
	 if (type==HOUR_ARCHIVE) tim=tim-3600;
	 if (type==DAY_ARCHIVE) tim-=3600*24;
	 if (type==INT_ARCHIVE) tim-=3600*24;
         // if (type==MONTH_ARCHIVE && currenttime2->tm_mon>0) ;
        }
//     result = preset_multiple_registers(&mb_param, this->adr, ADR_ARCHTYPE, 4, (uint16_t *)data_in);
     rs=3;
     while (rs)
        {
         result = preset_multiple_registers(&mb_param, this->adr, ADR_YEAR, 7, (uint16_t *)data_in);
         if (result>=0) break;
         rs--;
        }
     //3.	Установить биты AId, Id в регистре Статус(40001 + 6) в значение 0, а биты  Dbd, AIc в 1. 
     //if (type>9) rs=0x6A;
     //else rs=0;
     //data_in[0]=rs;
     //result = preset_multiple_registers(&mb_param, this->adr, ADR_STATUS, 1, (uint16_t *)data_in);
     //result = preset_single_register(&mb_param, this->adr, ADR_STATUS, rs);
     sleep (1);
     //4.	Считать таблицу измерительных параметров. Регистры имеют формат данных Тип1.
     rs=3;
     while (rs)
        {
         result = read_input_registers(&mb_param, this->adr, ADR_DATA, 0x50, (uint16_t *)data);
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
	 data_in[0]=data[16]; data_in[3]=data[15]; data_in[2]=data[14]; data_in[1]=data[13];
         q=*(float*)(data_in);
	 data_in[0]=data[20]; data_in[3]=data[19]; data_in[2]=data[18]; data_in[1]=data[17];
	 q2=*(float*)(data_in);
	 data_in[0]=data[24]; data_in[3]=data[23]; data_in[2]=data[22]; data_in[1]=data[21];
	 v1=*(float*)(data_in);
	 data_in[0]=data[28]; data_in[3]=data[27]; data_in[2]=data[26]; data_in[1]=data[25];
         v2=*(float*)(data_in);
	 data_in[0]=data[32]; data_in[3]=data[31]; data_in[2]=data[30]; data_in[1]=data[29];
         t1=*(float*)(data_in);
	 data_in[0]=data[36]; data_in[3]=data[35]; data_in[2]=data[34]; data_in[1]=data[33];
	 t2=*(float*)(data_in);
	 data_in[0]=data[64]; data_in[3]=data[63]; data_in[2]=data[62]; data_in[1]=data[61];
	 v3=*(float*)(data_in);
	 data_in[0]=data[64]; data_in[3]=data[63]; data_in[2]=data[62]; data_in[1]=data[61];
//for (rs=33;rs<70;rs++) ULOGW("[%d]=%x",rs,data[rs]);

         if (type==9 || type==25) if (debug>2) ULOGW ("[elf] [%02d:%02d:%02d] q1=%f, q2=%f, t1=%f, t2=%f, v1=%f, v2=%f %f",data[3],data[0]+1,data[1],q,q2,t1,t2,v1,v2,v3);
         if (type==27) data[2]=0;
         if (type>25) if (debug>2) ULOGW ("[elf][%d] [%02d-%02d-%02d %02d:00:00] q=%f, t1=%f, t2=%f, v1=%f, v2=%f %f",type,data[3],data[0],data[1]+2000,data[2],q,t1,t2,v1,v2,v3);
	 if (type==9) typ=0;
	 if (type==25) typ=9;
	 if (type==26) typ=1;
	 if (type==27) typ=2;
	 if (type==28) typ=4;
         if (typ)
	    {
    	     if (typ==0) sprintf (date,"%04d%02d%02d%02d0000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,data[3],data[0],data[1]);
	     if (typ>0)
		    {
    		     if (typ==9) sprintf (date,"%04d%02d%02d000000",data[1]+2000,data[0],data[3]);
    		     if (typ==1) sprintf (date,"%04d%02d%02d%02d0000",data[1]+2000,data[0],data[3],data[2]);
    		     if (typ==2) sprintf (date,"%04d%02d%02d000000",data[1]+2000,data[0],data[3],data[2]);
    		     if (typ==4) sprintf (date,"%04d%02d01000000",data[1]+2000,data[0],data[3],data[2]);
    		     //printf ("date=%s\n",date);
    		     if (data[1]<9 || data[1]>20) { deep--; continue; }
    		     //if (data[0]>12) continue;
		    }
	     if (typ==1 && (data[1]>20)) {deep--; continue; }
	     //if (type==2 && (data[2]<9)) continue;
		// Heat jiloi
	     if (this->adr==1)
	        {
                 StoreData (dbase,this->device, 13, 4, typ, 0, q, date, 1);
	         StoreData (dbase,this->device, 11, 3, typ, 0, v1, date, 1);
	         StoreData (dbase,this->device, 12, 3, typ, 0, v1, date, 1);
                 StoreData (dbase,this->device, 4,  3, typ, 0, t1, date, 1);
    	         StoreData (dbase,this->device, 11, 4, typ, 0, v2, date, 1);
    	         StoreData (dbase,this->device, 12, 4, typ, 0, v2, date, 1);
                 StoreData (dbase,this->device, 4,  4, typ, 0, t2, date, 1);
		}
		// Heat vvod
	     if (this->adr==2)
	        {
                 StoreData (dbase,this->device, 13, 0, typ, 0, q, date, 1);
                 StoreData (dbase,this->device, 13, 3, typ, 0, q, date, 1);
	         StoreData (dbase,this->device, 11, 0, typ, 0, v1, date, 1);
	         StoreData (dbase,this->device, 12, 0, typ, 0, v1, date, 1);
                 StoreData (dbase,this->device, 4,  0, typ, 0, t1, date, 1);
    	         StoreData (dbase,this->device, 11, 1, typ, 0, v2, date, 1);
    	         StoreData (dbase,this->device, 12, 1, typ, 0, v2, date, 1);
                 StoreData (dbase,this->device, 4,  1, typ, 0, t2, date, 1);
		}
		// Heat vstroiki
	     if (this->adr==4)
	        {
                 StoreData (dbase,this->device, 13, 5, typ, 0, q, date, 1);
	         StoreData (dbase,this->device, 11, 5, typ, 0, v1, date, 1);
	         StoreData (dbase,this->device, 12, 5, typ, 0, v1, date, 1);
                 StoreData (dbase,this->device, 4,  5, typ, 0, t1, date, 1);
    	         StoreData (dbase,this->device, 11, 6, typ, 0, v2, date, 1);
    	         StoreData (dbase,this->device, 12, 6, typ, 0, v2, date, 1);
                 StoreData (dbase,this->device, 4,  6, typ, 0, t2, date, 1);
		}
		// gvs
	     if (this->adr==5)
	        {
	         StoreData (dbase,this->device, 11, 8, typ, 0, v1, date, 1);
	         StoreData (dbase,this->device, 11, 9, typ, 0, v3, date, 1);
		}
	    }
         else
	    {
	     if (this->adr==1)
	        {
		 StoreData (dbase,this->device, 13, 4, 0, q, 0);
    	         StoreData (dbase,this->device, 11, 3, 0, v1, 0);
    		 StoreData (dbase,this->device, 11, 4, 0, v2, 0);
    	         StoreData (dbase,this->device, 12, 3, 0, v1, 0);
        	 StoreData (dbase,this->device, 12, 4, 0, v2, 0);
	         StoreData (dbase,this->device, 4,  3, 0, t1, 0);
    		 StoreData (dbase,this->device, 4,  4, 0, t2, 0);
    		}
	     if (this->adr==2)
	        {
        	 StoreData (dbase,this->device, 13, 0, 0, q, 0);
    	         StoreData (dbase,this->device, 11, 0, 0, v1, 0);
    		 StoreData (dbase,this->device, 11, 1, 0, v2, 0);
    	         StoreData (dbase,this->device, 12, 0, 0, v1, 0);
        	 StoreData (dbase,this->device, 12, 1, 0, v2, 0);
	         StoreData (dbase,this->device, 4,  0, 0, t1, 0);
    		 StoreData (dbase,this->device, 4,  1, 0, t2, 0);
    		}
	     if (this->adr==4)
	        {
        	 StoreData (dbase,this->device, 13, 5, 0, q, 0);
    	         StoreData (dbase,this->device, 11, 5, 0, v1, 0);
    		 StoreData (dbase,this->device, 11, 6, 0, v2, 0);
    	         StoreData (dbase,this->device, 12, 5, 0, v1, 0);
        	 StoreData (dbase,this->device, 12, 6, 0, v2, 0);
	         StoreData (dbase,this->device, 4,  5, 0, t1, 0);
    		 StoreData (dbase,this->device, 4,  6, 0, t2, 0);
    		}
	     if (this->adr==5)
	        {
    	         StoreData (dbase,this->device, 11, 8, 0, v1, 0);
    		 StoreData (dbase,this->device, 11, 9, 0, v3, 0);
    		}
	    }
	}
     this->akt=1;
     this->qerrors=0;
     this->conn=0;
     deep--;
     //sleep (5);
    }
// ULOGW ("[elf]free");
 free (currenttime2);
 return 0;
}
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 sprintf (devp,"/dev/ttyS%d",blok);
// sprintf (devp,"/dev/ttyUSB0",blok);
 modbus_init_rtu(&mb_param, devp, speed, "none", 8, 1);
 modbus_set_debug(&mb_param, FALSE);
 if (modbus_connect(&mb_param) == -1) 
    {
     if (debug>0) ULOGW("[elf] error open com-port %s",devp); 
     return FALSE;
    }
 else if (debug>1) ULOGW("[elf] open com-port %s success",devp); 
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
