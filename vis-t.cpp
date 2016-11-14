//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "vis-t.h"
#include "db.h"
#include "func.h"
#include "modbus/modbus.h"
//-----------------------------------------------------------------------------
static	db		dbase;
static 	INT		fd;
static 	termios 	tio;

static	UINT	data_adr;	// for test we remember what we send to device
static	UINT	data_len;	// data address and lenght
static	UINT	data_op;	// and last operation

extern 	"C" UINT device_num;	// total device quant
extern 	"C" BOOL WorkRegim;	// work regim flag
extern 	"C" tm *currenttime;	// current system time

extern 	"C" DeviceR 	dev[MAX_DEVICE];	// device class
extern 	"C" DeviceVIST 	vist[MAX_DEVICE_NSP];	// VIS-T class

extern	"C" 	uint16_t	dev_num[30];
static 	union 	fnm fnum[5];
extern	UINT	debug;
extern	BOOL 	threads[20];

VOID 	ULOGW (const CHAR* string, ...);	// log function
UINT 	baudrate (UINT baud);			// baudrate select
unsigned short Crc16( unsigned char *pcBlock, unsigned short len );

static 	BOOL 	OpenCom (SHORT blok, UINT speed);	// open com function
static 	BOOL	OpenComModbus (SHORT blok, UINT speed);

static	BOOL 	LoadVISTConfig();			// load all irp configuration
//-----------------------------------------------------------------------------
static	modbus_param_t mb_param;
static 	uint16_t crc16(uint8_t *buffer, uint16_t buffer_length);
//-----------------------------------------------------------------------------
void * vistDeviceThread (void * devr)
{
 int devdk=*((int*) devr);			// DK identificator
 bool	rs=0;
 dbase.sqlconn("","root","");			// connect to database

 LoadVISTConfig();

 // open port for work
 // if (dev_num[TYPE_VIST]>0) rs=OpenCom (vist[0].port, vist[0].speed);
 // else return (0);

 for (UINT r=0;r<dev_num[TYPE_VIST];r++)
    {
     if (debug>0) ULOGW ("[vis-t] vist[%d/%d].ReadLastArchive ()",r,dev_num[TYPE_VIST]);
     vist[r].ReadVersion ();
     vist[r].ReadTime ();
    }

 sleep (5);
 while (WorkRegim)
    {
     for (UINT r=0;r<dev_num[TYPE_VIST];r++)
        {
         if (debug>3) ULOGW ("[vis-t] vist[%d/%d].ReadLastArchive ()",r,dev_num[TYPE_VIST]);
	 rs=OpenComModbus (vist[0].port, vist[0].speed);
	 if (rs)
	    {
             vist[r].ReadTime ();
    	     vist[r].ReadData (TYPE_CURRENTS);
    	     vist[r].ReadData (TYPE_INCREMENTS);
	     modbus_close(&mb_param);
	    }
	 rs=OpenCom (vist[0].port, vist[0].speed);
	    {
    	     if (currenttime->tm_min<10) 	vist[r].ReadData (TYPE_HOURS);
	     if (currenttime->tm_hour<12)	vist[r].ReadData (TYPE_DAYS);
	     if (currenttime->tm_mday<10)	vist[r].ReadData (TYPE_MONTH);
	    }
	 sleep (3);
	}
     sleep (1);
    }
 dbase.sqldisconn();
 if (debug>0) ULOGW ("[vis-t] vis-t thread end");
 threads[TYPE_VIST]=false;
 pthread_exit (NULL);
}
//--------------------------------------------------------------------------------------
BOOL LoadVISTConfig()
{
 int vist_num=0;
 for (UINT d=0;d<device_num;d++)
 if (dev[d].type==TYPE_VIST)
    {
     vist[vist_num].idvist=d;
     vist[vist_num].device=dev[d].idd;
     vist[vist_num].iddev=dev[d].idd;
     vist[vist_num].SV=dev[d].SV;
     vist[vist_num].interface=dev[d].interface;
     vist[vist_num].protocol=dev[d].protocol;
     vist[vist_num].port=dev[d].port;
     vist[vist_num].speed=dev[d].speed;
     vist[vist_num].adr=dev[d].adr;
     vist[vist_num].type=dev[d].type;
     strcpy(vist[vist_num].number,dev[d].number);
     vist[vist_num].flat=dev[d].flat;
     vist[vist_num].akt=dev[d].akt;
     strcpy(vist[vist_num].lastdate,dev[d].lastdate);
     vist[vist_num].qatt=dev[d].qatt;
     vist[vist_num].qerrors=dev[d].qerrors;
     vist[vist_num].conn=dev[d].conn;
     strcpy(vist[vist_num].devtim,dev[d].devtim);
     vist[vist_num].chng=dev[d].chng;
     vist[vist_num].req=dev[d].req;
     vist[vist_num].source=dev[d].source;
     strcpy(vist[vist_num].name,dev[d].name);
         vist_num++;
    }
 if (debug>0) ULOGW ("[vis-t] total %d vis-t add to list",vist_num);
}
//---------------------------------------------------------------------------------------------------
bool DeviceVIST::ReadVersion ()
{
 int 		rs;
 uint8_t	data[800];
 rs=read_holding_registers(&mb_param, this->addr, 4, 63, (uint16_t *)data);
 if (rs)
    {
     snprintf (this->dev_name,21,"%s",data);
     snprintf (this->version,21,"%s",data+21);
     snprintf (this->serial,21,"%s",data+42);
     if (debug>0) ULOGW ("[vis-t] VIS-T [%s] [%s] [%s]",this->device,this->version,this->serial);
    }
 else ULOGW ("[vist-t] no VIS-T found");
 return true;
}
//---------------------------------------------------------------------------------------------------
bool DeviceVIST::ReadTime ()
{
 int 		rs;
 uint8_t	data[800];
 rs=read_input_registers(&mb_param, this->addr, DATETIME, 3, (uint16_t *)data);
 if (rs)
    {
     sprintf (this->devtim,"%04d%02d%02d%02d%02d%02d",data[5],data[4],data[3],data[0],data[1],data[2]);
     if (debug>0) ULOGW ("[vis-t] VIS-T [time=%s]",this->devtim);
    }
 return true;
}

//---------------------------------------------------------------------------------------------------
int DeviceVIST::ReadData (UINT	type)
{
 int 	rs;
 BYTE	meas[10];
 BYTE	data[8000];
 CHAR	date[50];
 FLOAT	q, t1, t2, t3, v1, v2, v3, m1, m2, m3, p1, p2, p3;
 BYTE	v1_t,v2_t,v3_t,q_t;

 this->qatt++;	// attempt
 if (type==TYPE_CURRENTS)
    {
     rs=read_input_registers(&mb_param, this->addr, 0x202, 4, (uint16_t *)data);
     if (rs) 
	{
         v1_t=data[0];
	 v2_t=data[1];
         v3_t=data[2];
	 q_t=data[3];
        }

     rs=read_input_registers(&mb_param, this->addr, 0x200, 2, (uint16_t *)meas);
     if (rs)
	{
         rs=read_input_registers(&mb_param, this->addr, 0x306, 12, (uint16_t *)data);
	 if (rs && meas[0]&0x1) { v1=(float)((uint32_t)data/pow(10,v1_t)); StoreData (dbase, this->device, 11, 0, v1, 0, 0); }
         if (rs && meas[0]&0x2) { v2=(float)((uint32_t)(data+4)/v2_t); StoreData (dbase, this->device, 11, 1, v2, 0, 0); }
	 if (rs && meas[0]&0x4) { v3=(float)((uint32_t)(data+8)/v3_t); StoreData (dbase, this->device, 11, 2, v3, 0, 0); }
         if (rs && meas[0]&0x8) { m1=(float)((uint32_t)(data+12)/v1_t); StoreData (dbase, this->device, 12, 0, m1, 0, 0); }
	 if (rs && meas[0]&0x10) { m2=(float)((uint32_t)(data+16)/v2_t); StoreData (dbase, this->device, 12, 1, m2, 0, 0); }
         if (rs && meas[0]&0x20) { m3=(float)((uint32_t)(data+20)/v3_t); StoreData (dbase, this->device, 12, 2, m3, 0, 0); }

	 rs=read_input_registers(&mb_param, this->addr, 0x206, 3, (uint16_t *)data);
         if (rs && meas[0]&0x40) { t1=(float)((data[0]*256+data[1])/100); StoreData (dbase, this->device, 4, 0, t1, 0, 0); }
	 if (rs && meas[0]&0x80) { t2=(float)((data[2]*256+data[3])/100); StoreData (dbase, this->device, 4, 1, t2, 0, 0); }
         if (rs && meas[0]&0x100) { t3=(float)((data[4]*256+data[5])/100); StoreData (dbase, this->device, 4, 2, t3, 0, 0); }

	 rs=read_input_registers(&mb_param, this->addr, 0x222, 3, (uint16_t *)data);
         if (rs && meas[0]&0x200) { p1=(float)((data[0]*256+data[1])/10); StoreData (dbase, this->device, 16, 0, p1, 0, 0); }
	 if (rs && meas[0]&0x400) { p2=(float)((data[2]*256+data[3])/10); StoreData (dbase, this->device, 16, 1, p2, 0, 0); }
         if (rs && meas[0]&0x800) { p3=(float)((data[4]*256+data[5])/10); StoreData (dbase, this->device, 16, 2, p3, 0, 0); }

	 rs=read_input_registers(&mb_param, this->addr, 0320, 2, (uint16_t *)data);
         if (rs) { q=(float)((uint32_t)(data)/q_t); StoreData (dbase, this->device, 13, 0, q, 0, 0); }
         if (debug>2) ULOGW ("[vis-t] [currents] q=%f, t1=%f, t2=%f, v1=%f, v2=%f, m1=%f, m2=%f, p1=%f, p2=%f",q,t1,t2,v1,v2,m1,m2,p1,p2);
	}
    }
 else
    {
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>5) this->akt=0;
     if (this->qerrors==6) Events (dbase,(3*0x10000000)|(TYPE_VIST*0x1000000)|SEND_PROBLEM,this->device);
    }

 if (type==TYPE_INCREMENTS)
    {
     rs=read_input_registers(&mb_param, this->addr, 0x402, 4, (uint16_t *)data);
     if (rs) 
	{
         v1_t=data[0];
	 v2_t=data[1];
         v3_t=data[2];
	 q_t=data[3];
        }

     rs=read_input_registers(&mb_param, this->addr, 0x400, 2, (uint16_t *)meas);
     if (rs)
	{
         sprintf (date,"%04d%02d%02d000000",currenttime->tm_year+1900,currenttime->tm_mon,currenttime->tm_mday);
         rs=read_input_registers(&mb_param, this->addr, 0x412, 12, (uint16_t *)data);
	 if (rs && meas[0]&0x2) { v1=(float)((uint32_t)data/pow(10,v1_t)); StoreData (dbase, this->device, 11, type, 0, v1, 0, date, 0); }
         if (rs && meas[0]&0x4) { v2=(float)((uint32_t)(data+4)/v2_t); StoreData (dbase, this->device, 11, type, 1, v2, 0, date, 0); }
	 if (rs && meas[0]&0x8) { v3=(float)((uint32_t)(data+8)/v3_t); StoreData (dbase, this->device, 11, type, 2, v3, 0, date, 0); }
         if (rs && meas[0]&0x10) { m1=(float)((uint32_t)(data+12)/v1_t); StoreData (dbase, this->device, 12, type, 0, m1, 0, date, 0); }
	 if (rs && meas[0]&0x20) { m2=(float)((uint32_t)(data+16)/v2_t); StoreData (dbase, this->device, 12, type, 1, m2, 0, date, 0); }
         if (rs && meas[0]&0x40) { m3=(float)((uint32_t)(data+20)/v3_t); StoreData (dbase, this->device, 12, type, 2, m3, 0, date, 0); }

	 rs=read_input_registers(&mb_param, this->addr, 0x434, 4, (uint16_t *)data);
         if (rs) { q=(float)((uint32_t)(data)/q_t); StoreData (dbase, this->device, 13, type, 0, q, 0, date, 0); }
         if (rs) { t1=(float)((uint32_t)(data)/2); StoreData (dbase, this->device, 21, type, 0, t1, 0, date, 0); }

         if (debug>2) ULOGW ("[vis-t] [increments] q=%f, v1=%f, v2=%f, m1=%f, m2=%f, t=%f",q,v1,v2,m1,m2,t1);
	}
    }
 else
    {
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>5) this->akt=0;
     //if (this->qerrors==6) Events (dbase,(3*0x10000000)|(TYPE_VIST*0x1000000)|SEND_PROBLEM,this->device);
    }

 float	inc_vol[3];
 float	inc_mas[3];
 float	inc_time[3];
 float	inc_q[3];
 BYTE	point[4];

 if (type==TYPE_HOURS || type==TYPE_DAYS || type==TYPE_MONTH)
    {
     send_vist (0x14, 0, 48, data);
     point[0]=*(data+0x3a); point[1]=*(data+0x3b);
     point[2]=*(data+0x3c); point[3]=*(data+0x3d);
     inc_vol[0]=(float)((uint32_t)(data+0x16)/point[0]);
    }
 return 0;
}
//---------------------------------------------------------------------------------------------------
bool DeviceVIST::send_vist (UINT op, UINT reg, UINT nreg, UCHAR* dat)
{
 if (debug>3) ULOGW ("[vis-t] send_vist (%d, %d, %d)",op,reg,nreg);
 
 UINT	crc=0;		//(* CRC checksum *)
 UINT	nbytes = 0; 	//(* number of bytes in send packet *)
 UINT	bytes = 0; 	//(* number of bytes in send packet *)
 BYTE	data[100];	//(* send sequence *)

 data[0]=this->adr;
 data[1]=op;
 data[2]=reg/256;
 data[3]=reg%256;
 data[4]=nreg/256;
 data[5]=nreg%256;
 crc = crc16 (data, 6);
 data[6] = crc/256;
 data[7] = crc%256;
 write (fd,&data,7);
 if (debug>2) ULOGW ("[vis-t %d] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",this->adr,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
 usleep (300000);
 ioctl (fd,FIONREAD,&nbytes); 
 if (debug>2) ULOGW ("[vis-t] read (%d)",nbytes);
 nbytes=read (fd, &data, 200);
 if (debug>2) ULOGW ("[vis-t] read (%d)",nbytes);
 if (bytes<200) memcpy (dat,data,nbytes);
 return true;
}
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 sprintf (devp,"/dev/ttyS%d",blok);
 if (debug>0) ULOGW("[vis-t] attempt open com-port %s on speed %d",devp,speed);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[vis-t] error open com-port %s [%s]",devp,strerror(errno)); 
     return false;
    }
 else if (debug>1) ULOGW("[vis-t] open com-port success"); 

 tcflush(fd, TCIOFLUSH); //Clear send & recive buffers
 tcgetattr(fd, &tio);
 cfsetospeed(&tio, baudrate(speed));

 tio.c_cflag |= CREAD|CLOCAL|baudrate(speed); 
 tio.c_cflag &= ~(CSIZE|PARENB|PARODD);
 tio.c_cflag &= ~CSTOPB;
 tio.c_cflag |=CS8;
 tio.c_cflag &= ~CRTSCTS;
 tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
 tio.c_oflag &= ~(ONLCR);

 tio.c_iflag = 0;

 tio.c_iflag &= ~(INLCR | IGNCR | ICRNL);
 tio.c_iflag &= ~(IXON | IXOFF | IXANY);

 if (debug>1) ULOGW("[vis-t] c[0x%x] l[0x%x] i[0x%x] o[0x%x]",tio.c_cflag,tio.c_lflag,tio.c_iflag,tio.c_oflag);  
 
 tio.c_cc[VMIN] = 0;
 tio.c_cc[VTIME] = 10; //Time out in 10e-1 sec
 
 //cfsetispeed(&tio, baudrate(speed));
 //fcntl(fd, F_SETFL, FNDELAY);
 fcntl(fd, F_SETFL, 0);
 tcsetattr(fd, TCSANOW, &tio);
 tcsetattr (fd,TCSAFLUSH,&tio);

 return true;
}
//---------------------------------------------------------------------------------------------------
BOOL OpenComModbus (SHORT blok, UINT speed)
{
 CHAR devp[50];
 sprintf (devp,"/dev/ttyS%d",blok);
 modbus_init_rtu(&mb_param, devp, speed, "even", 8, 1);
 modbus_set_debug(&mb_param, TRUE);
 if (modbus_connect(&mb_param) == -1) 
    {
     if (debug>0) ULOGW("[vis-t] error open com-port %s",devp); 
     return FALSE;
    }
 else if (debug>1) ULOGW("[vis-t] open com-port success"); 
 return TRUE;
}
//--------------------------------------------------------------------------------------------------------------------
/* Table of CRC values for high-order byte */
static uint8_t table_crc_hi[] = {
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
        0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
        0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
        0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
        0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
        0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
        0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
        0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
        0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
        0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
        0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
        0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
        0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
        0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
        0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
        0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

/* Table of CRC values for low-order byte */
static uint8_t table_crc_lo[] = {
        0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 
        0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 
        0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 
        0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 
        0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4, 
        0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3, 
        0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 
        0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 
        0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 
        0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 
        0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 
        0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26, 
        0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 
        0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 
        0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 
        0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 
        0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 
        0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5, 
        0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 
        0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 
        0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 
        0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 
        0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 
        0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C, 
        0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 
        0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

static uint16_t crc16(uint8_t *buffer, uint16_t buffer_length)
{
        uint8_t crc_hi = 0xFF; /* high CRC byte initialized */
        uint8_t crc_lo = 0xFF; /* low CRC byte initialized */
        unsigned int i; /* will index into CRC lookup */

        /* pass through message buffer */
        while (buffer_length--) {
                i = crc_hi ^ *buffer++; /* calculate the CRC  */
                crc_hi = crc_lo ^ table_crc_hi[i];
                crc_lo = table_crc_lo[i];
        }

        return (crc_hi << 8 | crc_lo);
}
