//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
//#include "mdm.h"
#include "modbus/modbus.h"
#include "tsrv.h"
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
extern 	"C" UINT tsrv_num;	// LK sensors quant
extern 	"C" BOOL WorkRegim;	// work regim flag
extern 	"C" tm *currenttime;	// current system time

extern 	"C" DeviceR 	dev[MAX_DEVICE];// device class
extern 	"C" DeviceTSRV 	tsrv[2];	// ELF class
extern  "C" UINT	debug;

static 	union fnm fnum[5];

VOID 	ULOGW (CHAR* string, ...);		// log function
UINT 	baudrate (UINT baud);			// baudrate select
static  UINT CRC (const BYTE* const Data, const BYTE DataSize);
UINT 	CalcCRC(unsigned int c,unsigned int crc);
//VOID 	Events (DWORD evnt, DWORD device);	// events 
unsigned short Crc16( unsigned char *pcBlock, unsigned short len );

static 	BOOL 	OpenCom (SHORT blok, UINT speed);	// open com function
static	BOOL 	LoadTSRVConfig();			// load all irp configuration
static	BOOL 	ParseData (UINT type, UINT dv, BYTE* data, UINT len, UINT prm);

static	BOOL 	StoreData (UINT dv, UINT prm, UINT type, UINT status, FLOAT value, UINT pipe, BYTE* data);
static  BOOL 	StoreData (UINT dv, UINT prm, UINT status, FLOAT value, UINT pipe, UINT channel);
//-----------------------------------------------------------------------------
static	modbus_param_t mb_param;
//-----------------------------------------------------------------------------
void * tsrvDeviceThread (void * devr)
{
 int devdk=*((int*) devr);			// DK identificator
 dbase.sqlconn("","root","");			// connect to database
 // fill archive with fixed values
 // load from db all irp devices > store to class members
 LoadTSRVConfig();
 // open port for work
 if (tsrv_num>0)
    {
     BOOL rs=OpenCom (tsrv[0].port, tsrv[0].speed);
    }
 else return (0);

 for (UINT r=0;r<tsrv_num;r++)
    {
     if (debug>3) ULOGW ("[tsrv] tsrv[%d/%d].ReadLastArchive ()",r,tsrv_num);
     tsrv[r].ReadTime ();
     tsrv[r].ReadData (TYPE_HOURS,100,0);
     tsrv[r].ReadData (TYPE_DAYS,20,0);
     tsrv[r].ReadData (TYPE_MONTH,3,0);
     tsrv[r].ReadData (TYPE_HOURS,100,1);
     tsrv[r].ReadData (TYPE_DAYS,20,1);
     tsrv[r].ReadData (TYPE_MONTH,3,1);

     sleep (3);
    }

 while (WorkRegim)
    {
     for (UINT r=0;r<tsrv_num;r++)
        {
         if (debug>3) ULOGW ("[tsrv] tsrv[%d/%d].ReadLastArchive ()",r,tsrv_num);
	 tsrv[r].ReadTime ();
//	 tsrv[r].ReadCurrent ();

	 tsrv[r].ReadData (TYPE_HOURS,5,0);
	 tsrv[r].ReadData (TYPE_DAYS,2,0);
	 tsrv[r].ReadData (TYPE_MONTH,2,0);

	 sleep (3);
	}
     sleep (1);     
    }
 dbase.sqldisconn();
 modbus_close(&mb_param);
}
//--------------------------------------------------------------------------------------
// load all IRP configuration from DB
BOOL LoadTSRVConfig()
{
 sprintf (query,"SELECT * FROM dev_tsrv");
 res=dbase.sqlexec(query); 
 UINT nr=mysql_num_rows(res);
 for (UINT r=0;r<nr;r++)
    {
     row=mysql_fetch_row(res);
     tsrv[tsrv_num].id=atoi(row[0]);
     tsrv[tsrv_num].device=atoi(row[1]);     
     for (UINT d=0;d<device_num;d++)
        {
         if (dev[d].idd==tsrv[tsrv_num].device)
            {
    	     tsrv[tsrv_num].iddev=dev[d].id;
	     tsrv[tsrv_num].SV=dev[d].SV;
    	     tsrv[tsrv_num].interface=dev[d].interface;
	     tsrv[tsrv_num].protocol=dev[d].protocol;
	     tsrv[tsrv_num].port=dev[d].port;
	     tsrv[tsrv_num].speed=dev[d].speed;
	     tsrv[tsrv_num].adr=dev[d].adr;
	     tsrv[tsrv_num].type=dev[d].type;
	     strcpy(tsrv[tsrv_num].number,dev[d].number);
	     tsrv[tsrv_num].flat=dev[d].flat;
	     tsrv[tsrv_num].akt=dev[d].akt;
	     strcpy(tsrv[tsrv_num].lastdate,dev[d].lastdate);
	     tsrv[tsrv_num].qatt=dev[d].qatt;
	     tsrv[tsrv_num].qerrors=dev[d].qerrors;
	     tsrv[tsrv_num].conn=dev[d].conn;
	     strcpy(tsrv[tsrv_num].devtim,dev[d].devtim);
	     tsrv[tsrv_num].chng=dev[d].chng;
	     tsrv[tsrv_num].req=dev[d].req;
	     tsrv[tsrv_num].source=dev[d].source;
	     strcpy(tsrv[tsrv_num].name,dev[d].name);
	    }
	}
     tsrv_num++;
    } 
 if (res) mysql_free_result(res);
 if (debug>0) ULOGW ("[tsrv] total %d tsrv add to list",tsrv_num);
}
//---------------------------------------------------------------------------------------------------
bool DeviceTSRV::ReadVersion ()
{
 int 	rs,result=0;
 /*
 BYTE	data[8000];
 BYTE	data_in[300];

 result = read_input_registers(&mb_param, this->adr, ADR_NUMBER, 4, (uint16_t *)data);
 if (result>=0)
    {     
     sprintf (this->version,"%d%d%d%d%d%d%d%d",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
     if (debug>0) ULOGW ("[tsrv] tsrv [%s] software",this->version);
    }
 else ULOGW ("[tsrv] no tsrv found");    */
 return true;
}
//---------------------------------------------------------------------------------------------------
bool DeviceTSRV::ReadTime ()
{
 int 	rs,result=0;
 BYTE	data[8000];
 BYTE	data_in[300];

 unsigned long	ttim;
 time_t entime;
 struct tm *time_cur=(tm *)malloc (sizeof *time_cur);

 result = read_holding_registers(&mb_param, this->adr, ADR_TIME, 2, (uint16_t *)data);
 if (result>=0)
    {     
     ttim=(unsigned char)data[1]*256*256*256+(unsigned char)data[0]*256*256+(unsigned char)data[3]*256+(unsigned char)data[2];
     entime=(time_t)ttim;
     localtime_r(&entime,time_cur); 	    	
     //if (debug>2) ULOGW ("[tsrv] time [%02d-%02d-%02d %02d:00:00]",time_cur->tm_year+1900,time_cur->tm_mon+1,time_cur->tm_mday,time_cur->tm_hour);
     sprintf (this->devtim,"%d%02d%02d%02d%02d%02d",time_cur->tm_year+1900,time_cur->tm_mon+1,time_cur->tm_mday,time_cur->tm_hour,time_cur->tm_min,time_cur->tm_sec);
     if (debug>0) ULOGW ("[tsrv] tsrv time [%s]",this->devtim);

     sprintf (query,"UPDATE device SET lastdate=NULL,conn=1,devtim=%s WHERE id=%d",this->devtim,this->iddev);
     if (debug>2) ULOGW ("[tsrv] [%s]",query);
     res=dbase.sqlexec(query); if (res) mysql_free_result(res); 
    }
 else ULOGW ("[tsrv] no tsrv found");
 free (time_cur);
 return true;
}


//---------------------------------------------------------------------------------------------------
int DeviceTSRV::ReadCurrent ()
{
 int 	rs,result=0;
 BYTE	data[8000];
 BYTE	data_in[4];
 FLOAT	q, q2, t1, t2, v1, v2;

// result = read_holding_registers(&mb_param, this->adr, INPUT_DATA2, 2, (uint16_t *)data);    
 result = read_holding_registers(&mb_param, this->adr, INPUT_DATA, 100, (uint16_t *)data);    
// result = read_input_registers(&mb_param, this->adr, INPUT_DATA, 24, (uint16_t *)data);    

 this->qatt++;	// attempt
 if (result<0)
	{
         this->qerrors++;
         this->conn=0;
	 if (this->qerrors>5) this->akt=0; // !!! 5 > normal quant from DB
         if (this->qerrors==6) Events (dbase,(3*0x10000000)|(TYPE_IRP*0x1000000)|SEND_PROBLEM,this->device);
        }
 else
	{
         t1=*(float*)(data+2);
	 t2=*(float*)(data+6);
         q=*(float*)(data+26);
	 q2=*(float*)(data+30);
	 v1=*(float*)(data+50);
         v2=*(float*)(data+54);

         if (debug>2) ULOGW ("[tsrv] [%02x:%02x:%02x] q1=%f, q2=%f, t1=%f, t2=%f, v1=%f, v2=%f",data[0],data[1],data[2],q,q2,t1,t2,v1,v2);
    
         //StoreData (this->device, 13, 0, q, 0);
         //StoreData (this->device, 11, 0, v1, 0);
         //StoreData (this->device, 11, 0, v2, 1);
         //StoreData (this->device, 4, 0, t1, 0);
         //StoreData (this->device, 4, 0, t2, 1);

         this->akt=1;
	 this->qerrors=0;
         this->conn=0;
        }
 return 0;
}

//---------------------------------------------------------------------------------------------------
// ReadLastArchive - read last archive and store to db
int DeviceTSRV::ReadData (UINT	type, UINT deep, UINT secd)
{
 int 	rs,result=0,chan,frst=0;
 BYTE	data[8000];
 BYTE	data_in[4];
 unsigned long	ttim;
 long 	tmp;
 char 	tmm[20],date[20];
 FLOAT	q, q2, value, prfl, v1=0;

 time_t tim,entime;
 struct tm *time_cur=(tm *)malloc (sizeof *time_cur);
 tim=time(&tim);
 localtime_r(&tim,time_cur);
 tim-=3600;
 
 while (deep)
	{
	 localtime_r(&tim,time_cur);
	 v1=0;
	 if (type==TYPE_HOURS) result = read_archives (&mb_param, this->adr, ReadRecordsByTime, 0, 1, time_cur->tm_hour, time_cur->tm_mday-1, time_cur->tm_mon+1, time_cur->tm_year, (uint16_t *)data);
	 //if (type==TYPE_HOURS) result = read_archives (&mb_param, this->adr, ReadRecordsByTime, 0, 1, 1, 1, 1, 0x72, (uint16_t *)data);
	 //if (type==TYPE_HOURS) result = read_archives (&mb_param, this->adr, ReadRecordsByTime, 0, 1, 1, 1, 1, 0x72, (uint16_t *)data);
	 if (type==TYPE_DAYS)  result = read_archives (&mb_param, this->adr, ReadRecordsByTime, 1, 1, 0, time_cur->tm_mday, time_cur->tm_mon+1, time_cur->tm_year, (uint16_t *)data);
	 if (type==TYPE_MONTH) result = read_archives (&mb_param, this->adr, ReadRecordsByTime, 2, 1, 0, 1, time_cur->tm_mon+1, time_cur->tm_year, (uint16_t *)data);

	 this->qatt++;	// attempt
	 if (result<0)
		{
	         this->qerrors++;
    		 this->conn=0;
		 if (this->qerrors>5) this->akt=0; // !!! 5 > normal quant from DB
	         //if (this->qerrors==6) Events ((3*0x10000000)|(TYPE_IRP*0x1000000)|SEND_PROBLEM,this->device);
	        }
    	 else
		{
		 ttim=(unsigned char)data[1]*256*256*256+(unsigned char)data[0]*256*256+(unsigned char)data[3]*256+(unsigned char)data[2];
		 //for (rs=0;rs<40;rs++)  	        	ULOGW ("[tsrv] ttim[%d]=[%x]",rs,data[rs]);
	         //if (debug>2) ULOGW ("[tsrv] ttim[%d]=%ld [%x %x %x %x]",ttim,data[0],data[1],data[2],data[3]);
		 ttim=ttim+1;
		 entime=(time_t)ttim;
	         //if (debug>2) ULOGW ("[tsrv] ttim=%u",entime);
	    	 localtime_r(&entime,time_cur); 	    	
	         //if (debug>2) ULOGW ("[tsrv] time [%02d-%02d-%02d %02d:00:00]",time_cur->tm_year+1900,time_cur->tm_mon+1,time_cur->tm_mday,time_cur->tm_hour);

	         //q=*(unsigned long*)(data+4)/1000;
	         //q2=*(unsigned long*)(data+8)/1000;
		 tmp=((unsigned char)data[17]*256*256*256+(unsigned char)data[16]*256*256+(unsigned char)data[19]*256+(unsigned char)data[18]);

		 sprintf (tmm,"%u",tmp);
		 sscanf (tmm,"%f",&v1);
	         v1/=100;
	         if (debug>3) ULOGW ("[tsrv] tm=%u v1=%f [%x %x %x %x]",tmp,v1,data[16],data[17],data[18],data[19]);
		 
		 data[3]=time_cur->tm_mday;
		 data[0]=time_cur->tm_mon+1;
		 data[1]=time_cur->tm_year;
		 data[2]=time_cur->tm_hour;		 
		 if (type==TYPE_DAYS || type==TYPE_MONTH) data[2]=0;
		 if (type==TYPE_MONTH) data[3]=1;
	         //if (debug>2) ULOGW ("[tsrv] [%02d-%02d-%d %02d:00:00] v1=%f",data[3],data[0],data[1]+1900,data[2],v1);
	         StoreData (this->device, 11, type, 0, v1, 2,(BYTE *)data);

	         //if (debug>2) ULOGW ("[tsrv] [%d %d %d",type,secd,frst);
	         if (type==TYPE_HOURS) 
		    {
		     if (secd==0 && frst==0)
			{
			 chan=GetChannelNum (dbase,11,2,this->device);
		         StoreData (this->device, 11, 0, v1, 2, chan);
		         sprintf (date,"%d%02d%02d%02d%02d00",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,0);
	    	    	 StoreData (dbase, this->device, 11, 2, 9, 0, v1, date,  0, chan);
			 frst++;
			}
		    }

                 //StoreData (this->device, 4,  type, 0, t1,0,(BYTE *)data);
    	         //StoreData (this->device, 11, type, 0, v2, 1,(BYTE *)data);
                 //StoreData (this->device, 4,  type, 0, t2,1,(BYTE *)data);

		 if (secd)
			{
		         sprintf (date,"%d%02d%02d%02d0000",data[1]+1900,data[0],data[3],data[2]);
			 if (type==TYPE_HOURS) sprintf (query,"SELECT value FROM hours WHERE type=%d AND device=%d AND prm=%d AND pipe=2 AND date<%s ORDER BY date DESC",type,this->device,11,date);
			 else sprintf (query,"SELECT value FROM prdata WHERE type=%d AND device=%d AND prm=%d AND pipe=2 AND date<%s ORDER BY date DESC",type,this->device,11,date);
			 //ULOGW ("%s",query);
	    		 res=dbase.sqlexec(query); 
			 if (row=mysql_fetch_row(res)) 
			    {
			     prfl=atof(row[0]);
			     //ULOGW ("[tsrv] [%d] %f %f",type,v1,prfl);
			     if (prfl>0 && (v1-prfl)>0 && (v1-prfl)<900000)
				{
	    			 chan=GetChannelNum (dbase,11,0,this->device);
	    		         //StoreData (this->device, 11, 0, v1, 0, chan);
			         //ULOGW ("[tsrv] [%d] %d %d %d",type,11,0,this->device,chan);
		    	    	 StoreData (dbase, this->device, 11, 0, type, 0, (v1-prfl), date,  0, chan);
			         //if (type==TYPE_HOURS) StoreData (this->device, 11, 0, (v1-prfl), 0);
		
	    		         if (frst==0 && type==TYPE_HOURS)
					{
				         StoreData (this->device, 11, 0, (v1-prfl), 0, chan);
				         sprintf (date,"%d%02d%02d%02d%02d00",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,0);
			    	    	 StoreData (dbase, this->device, 11, 0, 9, 0, (v1-prfl), date,  0, chan);
					 frst++;
					}
				}
			    }
			}
                }

         if (type==TYPE_HOURS) tim-=3600; 
         if (type==TYPE_DAYS) tim-=24*3600;
         if (type==TYPE_MONTH) tim-=31*24*3600;
         this->akt=1;
	 this->qerrors=0;
	 this->conn=0;
         deep--;
    }
 free (time_cur);
 return 0;
}
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 speed=4800;
 sprintf (devp,"/dev/ttyr03",blok);
 modbus_init_rtu(&mb_param, devp, speed, "none", 8, 1);
 modbus_set_debug(&mb_param, FALSE);
 if (modbus_connect(&mb_param) == -1) 
    {
     if (debug>0) ULOGW("[tsrv] error open com-port %s",devp); 
     return FALSE;
    }
 else if (debug>1) ULOGW("[tsrv] open com-port success"); 
 return TRUE;
}
//---------------------------------------------------------------------------------------------------
BOOL StoreData (UINT dv, UINT prm, UINT status, FLOAT value, UINT pipe, UINT channel)
{
 sprintf (query,"SELECT * FROM prdata WHERE type=0 AND prm=%d AND device=%d AND pipe=%d AND channel=%d",prm,dv,pipe,channel);
 res=dbase.sqlexec(query); 
 if (row=mysql_fetch_row(res))
     sprintf (query,"UPDATE prdata SET value=%f,status=%d WHERE type=0 AND prm=%d AND device=%d AND pipe=%d AND channel=%d",value,status,prm,dv,pipe,channel);
 else sprintf (query,"INSERT INTO prdata(device,prm,type,value,status,pipe) VALUES('%d','%d','0','%f','%d','%d','%d')",dv,prm,value,status,pipe,channel);
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
 int 	chan=0;
 if (type==0) sprintf (date,"%04d%02d%02d%02d0000",currenttime->tm_year+1900,currenttime->tm_mon,currenttime->tm_mday,data[3],data[0],data[1]);
 if (type>0) 
    {
     sprintf (date,"%04d%02d%02d%02d0000",data[1]+1900,data[0],data[3],data[2]);
     if (data[1]<109 || data[1]>120) return false;
     if (data[0]>12) return false;
    }
 //if (debug>2)  ULOGW ("[tsrv][%d] date=%s (%f)",prm,date,value);
 // if (type==1 && (data[1]>12)) return false;
 if (value>1000000000) return false;

 chan=GetChannelNum (dbase,prm,pipe,dv); 
 if (type==TYPE_HOURS) sprintf (query,"SELECT * FROM hours WHERE type=%d AND prm=%d AND device=%d AND date=%s AND pipe=%d AND channel=%d",type,prm,dv,date,pipe,chan);
 else sprintf (query,"SELECT * FROM prdata WHERE type=%d AND prm=%d AND device=%d AND date=%s AND pipe=%d AND channel=%d",type,prm,dv,date,pipe,chan);
 // if (debug>2) ULOGW ("[tsrv] %s",query);
 res=dbase.sqlexec(query); 
 if (row=mysql_fetch_row(res))
    {
     if (type==TYPE_HOURS) 
        if (value>0) sprintf (query,"UPDATE hours SET value=%f,status=%d,date=date WHERE type='%d' AND prm=%d AND device='%d' AND date='%s' AND pipe='%d'",value,status,type,prm,dv,date,pipe);
     else if (value>0) sprintf (query,"UPDATE prdata SET value=%f,status=%d,date=date WHERE type='%d' AND prm=%d AND device='%d' AND date='%s' AND pipe='%d'",value,status,type,prm,dv,date,pipe);
     else
     //if (1)
        {
	 if (res) mysql_free_result(res); 
	 return true;
	}     
    }
 else 
    {
     if (type==TYPE_HOURS) sprintf (query,"INSERT INTO hours(device,prm,type,value,status,date,pipe,channel) VALUES('%d','%d','%d','%f','%d','%s','%d','%d')",dv,prm,type,value,status,date,pipe,chan);
     else sprintf (query,"INSERT INTO prdata(device,prm,type,value,status,date,pipe,channel) VALUES('%d','%d','%d','%f','%d','%s','%d','%d')",dv,prm,type,value,status,date,pipe,chan);
    }
 if (debug>3) ULOGW ("[tsrv] %s",query);

 if (res) mysql_free_result(res); 
 res=dbase.sqlexec(query); 
 if (res) mysql_free_result(res);
 return true;
}

//--------------------------------------------------------------------------------------------------------------------
UINT CRC (const BYTE* const Data, const BYTE DataSize)
{
 UINT _CRC = 0;
 BYTE* _Data = (BYTE*)Data;
 for(unsigned int i = 0; i < DataSize; i++) 
    {
     //ULOGW ("!%d [0x%x] > [%x]",i,*_Data,(UINT)_CRC);
     _CRC = CalcCRC(*_Data, _CRC);
     _Data++;
     //ULOGW ("@%d [0x%x] > [%u]",i,*_Data,(UINT)_CRC);
    }
 return _CRC;
}
//--------------------------------------------------------------------------------------------------------------------
unsigned int CalcCRC(unsigned int c,unsigned int crc)
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
unsigned short Crc16( unsigned char *pcBlock, unsigned short len )
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
