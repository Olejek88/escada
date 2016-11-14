//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "vkt.h"
//#include "modbus/modbus.h"
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

extern 	"C" UINT device_num;	// total device quant
extern 	"C" UINT vkt_num;	// LK sensors quant
extern 	"C" BOOL WorkRegim;	// work regim flag
extern 	"C" tm *currenttime;	// current system time

extern 	"C" DeviceR 	dev[MAX_DEVICE];	// device class
extern 	"C" DeviceVKT 	vkt[MAX_DEVICE_VKT];	// VKT class
extern  "C" DeviceDK	dk;
extern  "C" UINT	debug;

static 	union fnm fnum[5];

VOID    ULOGW (const CHAR* string, ...);              // log function

UINT 	baudrate (UINT baud);			// baudrate select
VOID 	Events (DWORD evnt, DWORD device);	// events 

#define VKT_PORT		5001	// http-port
#define COMM_BUFFER_SIZE 	1024
bool	ConnectVKT (char* addr);
static  INT s;			// socket
struct 	hostent *he;		// 
//static modbus_param_t mb_param;

int  read_input_registers(UINT adr,UINT start_reg, UINT nreg, UCHAR *dat, UINT protocol);
int  write_input_registers(UINT adr,UINT start_reg, UINT nreg, UCHAR* data, UINT protocol);
WORD Crc16(BYTE *Data, WORD size);

static 	BOOL 	OpenCom (SHORT blok, UINT speed);	// open com function
static	BOOL 	LoadVKTConfig();			// load all vkt configuration
//static	VOID	StartSimulate(UINT type); 
static	BOOL 	ParseData (UINT type, UINT dv, BYTE* data, UINT len, UINT prm);
static	BOOL 	StoreData (UINT dv, UINT prm, UINT type, UINT status, FLOAT value, UINT pipe, BYTE* data);
static  BOOL 	StoreData (UINT dv, UINT prm, UINT status, FLOAT value, UINT pipe);
//-----------------------------------------------------------------------------
void * vktDeviceThread (void * devr)
{
 int devdk=*((int*) devr);			// DK identificator
 dbase.sqlconn("","root","");			// connect to database
 // fill archive with fixed values
 // load from db all irp devices > store to class members
 LoadVKTConfig();
 // open port for work
 if (vkt_num>0)
    {
     //BOOL rs=OpenCom (vkt[0].port, vkt[0].speed);
     BOOL rs=0;
     if (vkt[0].protocol==2)
	{
	 ConnectVKT ((char *)"192.168.1.6");
	} 
     else
	 BOOL rs=OpenCom (1, 9600);
    }
 else return (0);

 for (UINT r=0;r<vkt_num;r++)  //vkt_num
    {
     //if (debug>0) ULOGW ("[vkt] vkt[%d/%d].ReadLastArchive ()",r,vkt_num);
     vkt[r].ReadVersion ();
     vkt[r].ReadTime ();
     //vkt[r].ReadData (TOTAL_VALUES,1);
     //vkt[r].ReadData (TOTALIZER,1);
     //vkt[r].ReadData (HOUR_ARCHIVE,25);
     vkt[r].ReadData (DAY_ARCHIVE,5);     
    }
// if (s) close(s);

// for (UINT r=0;r<19;r++) elf[0].ReadTime ();

 sleep (1);
 while (WorkRegim)
    {
     for (UINT r=0;r<vkt_num;r++)
        {
         if (debug>3) ULOGW ("[vkt] vkt[%d/%d].ReadLastArchive ()",r,vkt_num);
         //vkt[r].ReadData (TOTAL_VALUES,1);
         vkt[r].ReadData (HOUR_ARCHIVE,10);
         vkt[r].ReadData (DAY_ARCHIVE,2);
         //vkt[r].ReadData (MONTH_ARCHIVE,3);
         vkt[r].ReadData (TOTALIZER,1);
         sleep (3);
	}
     sleep (1);
    }
 dbase.sqldisconn();
 pthread_exit (NULL);
}
//--------------------------------------------------------------------------------------
// load all VKT configuration from DB
BOOL LoadVKTConfig()
{
 sprintf (query,"SELECT * FROM dev_vkt");
 res=dbase.sqlexec(query); 
 UINT nr=mysql_num_rows(res);
 for (UINT r=0;r<nr;r++)
    {
     row=mysql_fetch_row(res);
     vkt[vkt_num].idvkt=atoi(row[0]);
     vkt[vkt_num].device=atoi(row[1]);     
     for (UINT d=0;d<device_num;d++)
        {
         //ULOGW ("[vkt] [%d][%d]",dev[d].idd,vkt[vkt_num].device);
         if (dev[d].idd==vkt[vkt_num].device)
            {
    	     vkt[vkt_num].iddev=dev[d].id;
	     vkt[vkt_num].SV=dev[d].SV;
    	     vkt[vkt_num].interface=dev[d].interface;
	     vkt[vkt_num].protocol=dev[d].protocol;
	     vkt[vkt_num].port=dev[d].port;
	     vkt[vkt_num].speed=dev[d].speed;
	     vkt[vkt_num].adr=dev[d].adr;
	     vkt[vkt_num].type=dev[d].type;
	     strcpy(vkt[vkt_num].number,dev[d].number);
	     strcpy(vkt[vkt_num].name,dev[d].name);
	    }
	}
//     vkt[vkt_num].adr=0;
     ULOGW ("[vkt][%d](%d) SP=%d adr=%d",vkt_num,vkt[vkt_num].device,vkt[vkt_num].speed,vkt[vkt_num].adr);
     vkt_num++;
    } 
 if (res) mysql_free_result(res);
 if (debug>0) ULOGW ("[vkt] total %d vkt add to list",vkt_num);
}
//---------------------------------------------------------------------------------------------------
int  read_input_registers(UINT adr,UINT start_reg, UINT nreg, UCHAR *dat, UINT protocol)
{
 WORD crc=0;
 BYTE data_in[300];
 BYTE data[300];
 INT	nbytes = 0; 	//(* number of bytes in recieve packet *)
 INT	bytes = 0;

 data[0]=0xff;
 data[1]=0xff;
 if (protocol==2)  crc=write (s,&data,2);
 else crc=write (fd,&data,2);

 data[0]=adr;		// adr
 data[1]=0x3;		// func
 data[2]=start_reg/256;	// start reg
 data[3]=start_reg&0xff;// start reg
 data[4]=nreg/256;	// 
 data[5]=nreg&0xff;	// nreg
 crc = Crc16 (data, 6);
 data[6]=crc&0xff;
 data[7]=crc/256;
 if (protocol==2) crc=write(s,&data,8);
 else crc=write (fd,&data,8);
 if (debug>2) ULOGW ("[vkt] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
 sleep (1);

 if (protocol==2) 
	{
	 bytes=read (s, &data_in, 255);
	}
 else	{
	 ioctl (fd,FIONREAD,&nbytes);
	 bytes=read (fd, &data_in, nbytes);
	}

 if (debug>2) ULOGW ("[vkt] read (%d)(%d) bytes",bytes, nbytes);

 if (bytes>3)
    {
     ULOGW ("[vkt] A[%x] (%x)[%d] %x %x (0x%x,0x%x)",data_in[0],data_in[1],data_in[2],data_in[3],data_in[4],data_in[data_in[2]+3],data_in[data_in[2]+4]);
     if (bytes>0) if (debug>3) ULOGW ("[vkt] memcpy (%x,%x,%d)",dat,data_in+3,data_in[2]);
     if (bytes>0) memcpy (dat,data_in+3,data_in[2]);
     return bytes;
    }
 return 0;
}
//---------------------------------------------------------------------------------------------------
int  write_input_registers(UINT adr,UINT start_reg, UINT nreg, UCHAR* dat, UINT protocol)
{
 WORD 	crc=0;
 BYTE 	data_in[300];
 BYTE 	data[300];
 CHAR	query[300];
 INT	nbytes = 0; 	//(* number of bytes in recieve packet *)
 INT	bytes = 0;

 data[0]=0xff;
 data[1]=0xff;
 if (protocol==2)  crc=write (s,&data,2);
 else crc=write (fd,&data,2);

 data[0]=adr;		// adr
 data[1]=0x10;		// func
 data[2]=start_reg/256;	// start reg
 data[3]=start_reg&0xff;// start reg
 data[4]=0;		// 
 data[5]=0;		// 
 data[6]=nreg;		// nreg
 memcpy (data+7,dat,nreg);
 crc = Crc16 (data, 7+nreg);
 data[7+nreg]=crc&0xff;
 data[8+nreg]=crc/256;

 if (protocol==2) crc=write (s,&data,9+nreg);
 else write (fd,&data,9+nreg);

// sprintf (query,"[vkt] write (");
// for (int dd=0;dd<8+nreg;dd++) 
 if (debug>2) ULOGW ("[vkt] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19],data[20],data[21],data[22],data[23]);
 sleep (1);

 if (protocol==2) 
	{
	 bytes=read (s, &data_in, 255);
	}
 else	{
	 ioctl (fd,FIONREAD,&nbytes);
	 bytes=read (fd, &data_in, nbytes);
	}
 if (debug>2) ULOGW ("[vkt] read (%d)(%d) bytes (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",bytes, nbytes,data_in[0],data_in[1],data_in[2],data_in[3],data_in[4],data_in[5],data_in[6],data_in[7]);
 if (data_in[1]==0x90) 
     {	
      if (debug>2) ULOGW ("[vkt] error in answer (%d)",data_in[2]);
      return 0;
     }

 if (bytes>3)
    {
     ULOGW ("[vkt] A[%x] (%x)[%d] %x %x (0x%x,0x%x)",data_in[0],data_in[1],data_in[2],data_in[3],data_in[4],data_in[data_in[2]+3],data_in[data_in[2]+4]);
     if (bytes>0) if (debug>3) ULOGW ("[vkt] memcpy (%x,%x,%d)",dat,data_in+3,data_in[2]);
     if (bytes>0) memcpy (dat,data_in+3,data_in[2]);
    }
 return bytes;
}
//---------------------------------------------------------------------------------------------------
bool DeviceVKT::ReadVersion ()
{
 int 	rs,result=0;
 BYTE	data[180];
 //uint16_t data2[180];
 ULOGW ("[vkt] vkt version read [0x%x]",this->device);
 //this->adr=1;
 result = read_input_registers(this->adr,ID,8,data,2);
 if (result>2)
    {     
     sprintf (this->name,"%c%c%c%c%c%c%c%c",(CHAR)data[2],(CHAR)data[3],(CHAR)data[4],(CHAR)data[5],(CHAR)data[6],(CHAR)data[7],(CHAR)data[8],(CHAR)data[9]);
     if (debug>0) ULOGW ("[vkt] vkt id [%s]",this->name);
    }
 else ULOGW ("[vkt] no vkt found");    
 return true;
}
//---------------------------------------------------------------------------------------------------
bool DeviceVKT::ReadTime ()
{
 int 	rs,result=0;
 BYTE	data[80];
 ULOGW ("[vkt] vkt time read [0x%x]",this->device);
 this->adr=1;
 result = read_input_registers(this->adr,TIME_ADR,0,data,2);
 if (result>2)
    {     
     sprintf (this->devtim,"%02d-%02d-%04d %02d:%02d:%02d",data[0],data[1],data[2]+2000,data[3],data[4],data[5]);
     if (debug>0) ULOGW ("[vkt] vkt time [%s]",this->devtim);
    }
 else ULOGW ("[vkt] no vkt found");    

 return true;
}
//---------------------------------------------------------------------------------------------------
// ReadLastArchive - read last archive and store to db
int DeviceVKT::ReadData (UINT type, UINT deep)
{
 int 	rs,result=0;
 BYTE	data[8000];
 BYTE	data_in[4];
 CHAR	date[25];

 FLOAT	q, q2, t1, t2, t3, v1, v2, v3, q1, p1, p2, vt1, vt2;
 time_t tims;
 tims=time(&tims);
 struct	tm *prevtime;		// current system time 

 // t (1,2,3) | P (4,5) | G (9,10,11)

 if (type<3)
    {
     //data[cr*6]=cr; data[cr*6+1]=0x0; data[cr*6+2]=0x0;
     //data[cr*6+3]=0x40; data[cr*6+4]=0x4; data[cr*6+5]=0x0;
     //result = write_input_registers(this->adr,ELEMENT_LIST,84,data,2);
     int cr=0;
     data[cr*6]=3; data[cr*6+1]=0x0; data[cr*6+2]=0x0;
     data[cr*6+3]=0x40; data[cr*6+4]=0x4; data[cr*6+5]=0x0;
     cr=1;
     data[cr*6]=4; data[cr*6+1]=0x0; data[cr*6+2]=0x0;
     data[cr*6+3]=0x40; data[cr*6+4]=0x4; data[cr*6+5]=0x0;
     cr=2;
     data[cr*6]=12; data[cr*6+1]=0x0; data[cr*6+2]=0x0;
     data[cr*6+3]=0x40; data[cr*6+4]=0x4; data[cr*6+5]=0x0;
     cr=3;
     data[cr*6]=17; data[cr*6+1]=0x0; data[cr*6+2]=0x0;
     data[cr*6+3]=0x40; data[cr*6+4]=0x4; data[cr*6+5]=0x0;
     cr=4;
     data[cr*6]=18; data[cr*6+1]=0x0; data[cr*6+2]=0x0;
     data[cr*6+3]=0x40; data[cr*6+4]=0x4; data[cr*6+5]=0x0;
     cr=5;
     data[cr*6]=0; data[cr*6+1]=0x0; data[cr*6+2]=0x0;
     data[cr*6+3]=0x40; data[cr*6+4]=0x4; data[cr*6+5]=0x0;
     cr=6;
     data[cr*6]=1; data[cr*6+1]=0x0; data[cr*6+2]=0x0;
     data[cr*6+3]=0x40; data[cr*6+4]=0x4; data[cr*6+5]=0x0;
     result = write_input_registers(this->adr,ELEMENT_LIST,48,data,2);
    }

 if (type>=3)
    {
     int cr=0;
     data[cr*6]=3; data[cr*6+1]=0x0; data[cr*6+2]=0x0;
     data[cr*6+3]=0x40; data[cr*6+4]=0x4; data[cr*6+5]=0x0;
     cr=1;
     data[cr*6]=4; data[cr*6+1]=0x0; data[cr*6+2]=0x0;
     data[cr*6+3]=0x40; data[cr*6+4]=0x4; data[cr*6+5]=0x0;
     cr=2;
     data[cr*6]=12; data[cr*6+1]=0x0; data[cr*6+2]=0x0;
     data[cr*6+3]=0x40; data[cr*6+4]=0x4; data[cr*6+5]=0x0;
     cr=3;
     data[cr*6]=17; data[cr*6+1]=0x0; data[cr*6+2]=0x0;
     data[cr*6+3]=0x40; data[cr*6+4]=0x4; data[cr*6+5]=0x0;
     cr=4;
     data[cr*6]=18; data[cr*6+1]=0x0; data[cr*6+2]=0x0;
     data[cr*6+3]=0x40; data[cr*6+4]=0x4; data[cr*6+5]=0x0;
     result = write_input_registers(this->adr,ELEMENT_LIST,36,data,2);
    }

// if (this->protocol==2) result=preset_multiple_registers(&mb_param, this->adr, ELEMENT_LIST,78,(uint16_t *)data);
 data[0]=type; data[1]=0x0;
//if (this->protocol==2) result=preset_multiple_registers(&mb_param, this->adr, ELEMENT_TYPE,2,(uint16_t *)data);
 result = write_input_registers(this->adr,ELEMENT_TYPE,2,data,2);
 for (int cr=0; cr<=0xd; cr++)
    {
     data[cr*6]=0x0; data[cr*6+1]=0x0; data[cr*6+2]=0x0;
     data[cr*6+3]=0x0; data[cr*6+4]=0x0; data[cr*6+5]=0x0;
    }
 //sleep (150);
 while (deep)
    {
     //sleep (150);
     prevtime=localtime(&tims); 	// get current system time
     data[0]=prevtime->tm_mday;
     data[1]=prevtime->tm_mon+1;
     data[2]=prevtime->tm_year-100;
     data[3]=prevtime->tm_hour;

     if (type==HOUR_ARCHIVE) tims-=60*60;
     if (type==DAY_ARCHIVE) { tims-=60*60*24; prevtime->tm_hour=0; /*data[3]=prevtime->tm_hour;*/ }
     if (type==MONTH_ARCHIVE) { prevtime->tm_hour=0; if (prevtime->tm_mday%2) prevtime->tm_mday=30; else  prevtime->tm_mday=31; data[3]=0; data[0]=1; tims-=60*60*24*30; }

     if (debug>1) ULOGW ("[vkt] [At%d][%02d-%02d-%02d %02d:00]",type,data[0],data[1],data[2],data[3]);
     sprintf (date,"%02d-%02d-%02d %02d:00]",data[0],data[1],data[2],data[3]);
     data_in[1]=data[2]; data_in[0]=data[1]; data_in[2]=data[3]; data_in[3]=data[0];

     //if (this->protocol==2) result=preset_multiple_registers(&mb_param, this->adr, ELEMENT_DATE,4,(uint16_t *)data);

     if (type<3) result = write_input_registers(this->adr,ELEMENT_DATE,4,data,2);
     else result=1; 
     if (result) 
	{
	 //if (this->protocol==2) result = read_holding_registers(&mb_param, this->adr, DATA, 56, (uint16_t *)data);
	 result = read_input_registers(this->adr,DATA,56,data,2); 
	}
     //int read_input_registers(modbus_param_t *mb_param, int slave,int start_addr, int nb, uint16_t *dest);

     if (result)
        {
	 // q=1.564
	 // 73.2 75.5
	 // 67.67 44.35
	 int i=0;
	 //for (i=0; i<80; i++)	    ULOGW ("[vkt] [%d] 0x%x (%d)",i,data[i],data[i]);
	 //sprintf (data_in,"%d%d",data+0);
	 //ULOGW ("[vkt] [%d] 0x%x (%d)",i,data[i],data[i]);
	 if (type<3)
		{
		 v1=vt1=(float)(data[2]*256*256+data[1]*256+data[0])/100;
		 v2=vt2=(float)(data[8]*256*256+data[7]*256+data[6])/100;
		 q1=(float)(data[14]*256*256+data[13]*256+data[12])/1000;
		 t1=(float)(data[31]*256+data[30])/100;
		 t2=(float)(data[35]*256+data[34])/100;
		 //v3=(float)(data[25]*256+data[24])/100;
		 //vt1=(float)(data[1]*256+data[0])/100;
		 //vt2=(float)(data[7]*256+data[6])/100;
		 //p1=(float)(data[49]*256+data[48])/100;
		 //p2=(float)(data[53]*256+data[52])/100;
		 //q1=(float)(data[63]*256+data[62])/1000;
		 //q2=(float)(data[70]*256+data[69])/1000;
	         if (debug>2) ULOGW ("[vkt] [A%d][%s] t1=%f, t2=%f, v1=%f, v2=%f, p1=%f, p2=%f, q1=%f",type,date,t1,t2,v1,v2,p1,p2,q1);

		 StoreData (this->device, 4,  type, 0, t1,0,(BYTE *)data_in);
	         StoreData (this->device, 4,  type, 0, t2,1,(BYTE *)data_in);
	         //StoreData (this->device, 4,  type, 0, t3,2,(BYTE *)data_in);
    		 StoreData (this->device, 11,  type, 0, v1,0,(BYTE *)data_in);
	         StoreData (this->device, 11,  type, 0, v2,1,(BYTE *)data_in);
	         //StoreData (this->device, 11,  type, 0, v3,2,(BYTE *)data_in);
    		 //StoreData (this->device, 16,  type, 0, p1,0,(BYTE *)data_in);
	         //StoreData (this->device, 16,  type, 0, p2,1,(BYTE *)data_in);
	         StoreData (this->device, 13,  type, 0, q1,0,(BYTE *)data_in);
	         StoreData (this->device, 13,  type, 0, q1,2,(BYTE *)data_in);
		 q1=t1/1000;
                 StoreData (this->device, 13, 0, q1,0);
		 if (type==0)
		    {
    	             StoreData (this->device, 4,  0, t1,0);
	             StoreData (this->device, 4,  0, t2,1);
	             StoreData (this->device, 11, 0, vt1,0);
	             StoreData (this->device, 11, 0, vt2,1);
	             StoreData (this->device, 12, 0, vt1,0);
	             StoreData (this->device, 12, 0, vt2,1);
	             //StoreData (this->device, 16, 0, p1,0);
	             //StoreData (this->device, 16, 0, p2,1);
	             //StoreData (this->device, 13, 0, q1/100,0);
	             //StoreData (this->device, 13, 0, q1/100,2);
	            }
		}
	 else	{
		 v1=(float)(data[2]*256*256+data[1]*256+data[0])/100;
		 v2=(float)(data[8]*256*256+data[7]*256+data[6])/100;
		 q1=(float)(data[14]*256*256+data[13]*256+data[12])/1000;
		 t1=(float)(data[20]*256*256+data[19]*256+data[18]);
		 t2=(float)(data[26]*256*256+data[25]*256+data[24]);
	         StoreData (this->device, 12, 0, v1,11);
	         StoreData (this->device, 12, 0, v2,12);
	         StoreData (this->device, 13, 0, q1,13);
	         StoreData (this->device, 18, 0, t1,0);
	         StoreData (this->device, 18, 0, t2,1);
	         if (debug>2) ULOGW ("[vkt] [A%d][%02d-%02d-%02d %02d:00] total v1=%f, v2=%f, q1=%f, T1=%f, T2=%f",type,prevtime->tm_year+1900,prevtime->tm_mon+1,prevtime->tm_mday,prevtime->tm_hour,v1,v2,q1,t1,t2);
		}
	}
     deep--;
    }
 return 0;
}
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 blok=0;
 sprintf (devp,"/dev/ttyS%d",blok);
 if (debug>0) ULOGW("[vkt] attempt open com-port %s on speed %d",devp,speed);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[vkt] error open com-port %s [%s]",devp,strerror(errno)); 
     return FALSE;
    }
 else if (debug>1) ULOGW("[vkt] open com-port success"); 

 tcflush(fd, TCIOFLUSH); //Clear send & recive buffers
 tcgetattr(fd, &tio);
 cfsetospeed(&tio, baudrate(speed));
 tio.c_cflag = CS8|CREAD|baudrate(speed)|CLOCAL|CSTOPB;
 tio.c_lflag = 0;
 tio.c_iflag = IGNPAR| ICRNL;
 tio.c_oflag = 0;
 tio.c_cc[VMIN] = 0;
 tio.c_cc[VTIME] = 5; //Time out in 10e-1 sec
 cfsetispeed(&tio, baudrate(speed));
 fcntl(fd, F_SETFL, FNDELAY);
 tcsetattr(fd, TCSANOW, &tio);
 tcsetattr (fd,TCSAFLUSH,&tio);
 return TRUE;
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

 switch (type)
    {
     case 0: type=1; break;
     case 1: type=2; break;
     case 2: type=4; break;
     case 3: type=7; break;
     default: type=0;
    }
 sprintf (date,"%04d%02d%02d000000",data[1]+2000,data[0],data[3],data[2]);
  
 if (type==0) sprintf (date,"%04d%02d%02d%02d0000",currenttime->tm_year+1900,currenttime->tm_mon,currenttime->tm_mday,data[3],data[0],data[1]);
 if (type>0) 
    {
     if (type==1) sprintf (date,"%04d%02d%02d%02d0000",data[1]+2000,data[0],data[3],data[2]);
     if (type==2) sprintf (date,"%04d%02d%02d000000",data[1]+2000,data[0],data[3]);
     if (type==4) sprintf (date,"%04d%02d01000000",data[1]+2000,data[0]);

     //if (data[1]<9 || data[1]>20) return false;
     //if (data[0]>12) return false;
    }
// if (debug>2)  ULOGW ("[vkt][%d] date=%s (%f)",prm,date,value);
 
// if (type==1 && (data[1]>12)) return false;
// if (type==2 && (data[2]<9)) return false;
 if (value>10000000) return false;
 
 //if (type==1) if (data[3]<108 || data[3]>128) return false;
 //if (type==2) if (data[7]<108 || data[7]>128) return false;
 sprintf (query,"SELECT * FROM prdata WHERE type=%d AND prm=%d AND device=%d AND date=%s AND pipe=%d",type,prm,dv,date,pipe);
// if (debug>2) ULOGW ("[vkt] %s",query);
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
 else
    { 
     sprintf (query,"INSERT INTO prdata(device,prm,type,value,status,date,pipe) VALUES('%d','%d','%d','%f','%d','%s','%d')",dv,prm,type,value,status,date,pipe);
     if (debug>2) ULOGW ("[vkt] %s",query);
     if (res) mysql_free_result(res); 
     res=dbase.sqlexec(query); 
     if (res) mysql_free_result(res);
     sprintf (query,"INSERT INTO data(flat,prm,type,value,status,date,source) VALUES('0','%d','%d','%f','%d','%s','%d')",prm,type,value,status,date,pipe);
     if (debug>2) ULOGW ("[vkt] %s",query);
     if (res) mysql_free_result(res); 
     res=dbase.sqlexec(query); 
     if (res) mysql_free_result(res);
    }     
 return true;
}
//-----------------------------------------------------------------------------
WORD Crc16(BYTE *Data, WORD size)
{
 union
    {BYTE b[2]; unsigned short w;} Sum;
    char shift_cnt;
    BYTE *ptrByte;
    WORD byte_cnt = size;
    ptrByte=Data;
    Sum.w=0xffffU;
    for(; byte_cnt>0; byte_cnt--)
	{
	 Sum.w=(unsigned short)((Sum.w/256U)*256U+((Sum.w%256U)^(*ptrByte++)));
	 for(shift_cnt=0; shift_cnt<8; shift_cnt++)
	    {/*обработка байта*/
	     if((Sum.w&0x1)==1)
		Sum.w=(unsigned short)((Sum.w>>1)^0xa001U);
	     else
		Sum.w>>=1;
	    }
	}
 return Sum.w;
}

bool	ConnectVKT (char* addr)
{
 he=gethostbyname(addr);
 ULOGW ("[vkt] attempt connect to 192.168.1.6:%d",VKT_PORT);
 // IPPROTO_TCP
 s = socket(AF_INET, SOCK_STREAM, 0);
 if (s==-1)
    {
     ULOGW ("[vkt] error > can't open socket %d",VKT_PORT);
     return (0);
    }
 sockaddr_in si;
 si.sin_family = AF_INET;
 si.sin_port = htons(VKT_PORT);
 //si.sin_addr.s_addr = htonl(INADDR_ANY);
 si.sin_addr = *((struct in_addr *)he->h_addr);
 if (connect(s, (struct sockaddr *)&si, sizeof(si)) < 0)
    {
     ULOGW ("[vkt] error connect to 192.168.1.6");
     return (0);
    } 
 return (1);
}