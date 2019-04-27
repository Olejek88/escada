//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "mercury230.h"
#include "db.h"
#include "func.h"
#include <fcntl.h>
//-----------------------------------------------------------------------------
static  db      dbase;
static  INT     fd;
static  termios tio;

extern  "C" UINT device_num;    // total device quant
extern  "C" UINT dev_num[30];  	// total device quant
extern  "C" BOOL WorkRegim;     // work regim flag
extern  "C" tm *currenttime;    // current system time

extern  "C" DeviceR     dev[MAX_DEVICE];// device class
extern  "C" DeviceMER  	mer[MAX_DEVICE_M230];
extern  "C" DeviceDK	dk;

extern  "C" BOOL	mer_thread;
extern  "C" UINT	debug;

BOOL send_mer (UINT op, UINT prm, UINT frame, UINT index);
UINT read_mer (uint8_t* dat,uint8_t  type);
static uint16_t CRC(const uint8_t* const Data, const uint8_t DataSize, uint8_t type);

        VOID    ULOGW (const CHAR* string, ...);              // log function
        UINT    baudrate (UINT baud);                   // baudrate select
static  BOOL    OpenCom (SHORT blok, UINT speed);       // open com function
        VOID    Events (DWORD evnt, DWORD device);      // events 
static  BOOL    LoadMercuryConfig();                      // load tekon configuration
//-----------------------------------------------------------------------------

void * merDeviceThread (void * devr)
{
 int devdk=*((int*) devr);                      // DK identificator
 dbase.sqlconn("dk","root","");                 // connect to database

 // load from db km device
 LoadMercuryConfig();
 // open port for work
 BOOL rs=OpenCom (mer[0].port, mer[0].speed);
 if (!rs) return (0);

 while (WorkRegim)
 for (UINT d=0;d<dev_num[TYPE_MERCURY230];d++)
    {
     if (debug>1) ULOGW ("[m230] ReadInfo (%d)",d);
     mer[d].ReadInfo (); 
     if (debug>1) ULOGW ("[m230] ReadDataCurrent (%d)",d);
     mer[d].ReadDataCurrent (); 

     if (debug>1) ULOGW ("[m230] ReadDataArchive (%d)",d);
     mer[d].ReadAllArchive (10);
     if (!dk.pth[TYPE_MERCURY230])
        {
	 if (debug>0) ULOGW ("[m230] mercury 230 thread stopped");
	 //dbase.sqldisconn();
	 //pthread_exit ();	 
	 mer_thread=false;
	 pthread_exit (NULL);
	 return 0;	 
	}          
    }
 dbase.sqldisconn();
 if (debug>0) ULOGW ("[mer] Mercury230 thread end");
 mer_thread=false;
 pthread_exit (NULL); 
}
//-----------------------------------------------------------------------------
int DeviceMER::ReadInfo ()
{
 UINT   rs,serial,soft;
 BYTE   data[400];
 CHAR   date[20];

 rs=send_mercury (OPEN_CHANNEL, 0, 0, 0);
 if (rs)  
	{
	 rs = this->read_mercury(data, 0);
	 rs=send_mercury (READ_PARAMETRS, 0x0, 0, 0);
	 if (rs)  rs = this->read_mercury(data, 0);
	 if (rs)
		{
		 serial=data[0]+data[1]*256+data[2]*256*256+data[3]*256*256*256;
		 if (debug>2) ULOGW ("[mer][0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x] [serial=%d (%d/%d/%d)]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],serial,data[4],data[5],data[6]);
		}
	 rs = this->read_mercury(data, 0);
	 rs=send_mercury (READ_PARAMETRS, 0x2, 0, 0);
	 if (rs)  rs = this->read_mercury(data, 0);
	 if (rs)
		{
		 soft=data[0]+data[1]*256+data[2]*256*256;
		 if (debug>2) ULOGW ("[mer][0x%x 0x%x 0x%x] [soft=%d]",data[0],data[1],data[2],soft);
		}
	 rs=send_mercury (READ_TIME_230, 0x0, 0, 0);
	 if (rs)  rs = this->read_mercury(data, 0);
	 if (rs)
		{
		 sprintf (date,"%02d-%02d-%d %02d:%02d:%02d",data[3],data[4],data[5],data[2],data[1],data[0]);
		 sprintf (this->devtim,"%04d%02d%02d%02d%02d%02d",data[3],data[4],data[5],data[2],data[1],data[0]);
		 if (debug>2) ULOGW ("[mer][0x%x 0x%x 0x%x] [date=%s]",data[0],data[1],data[2],date);
	         sprintf (query,"UPDATE device SET lastdate=NULL,conn=1,devtim=%s WHERE id=%d",this->devtim,this->iddev);
	         if (debug>3) ULOGW ("[tek] [%s]",query);
        	 res=dbase.sqlexec(query); if (res) mysql_free_result(res); 
		}
        }
}
//-----------------------------------------------------------------------------
// ReadDataCurrent - read single device. Readed data will be stored in DB
int DeviceMER::ReadDataCurrent ()
{
 UINT   rs;
 float  fl;
 BYTE   data[400];
 CHAR   date[20];
 this->qatt++;  // attempt
 // open channel
 rs=send_mercury (OPEN_CHANNEL, 0, 0, 0);
 if (rs)  rs = this->read_mercury(data, 0);
 
 rs=send_mercury (READ_PARAMETRS, 0x11, ENERGY_SUM, 0);
 if (rs)  rs = this->read_mercury(data, 0);
 if (rs)
    {
     fl=((float)(data[0]&0x3f)+(float)data[1]*256+(float)data[2]*256*256); fl=fl/100;
     if (debug>2) ULOGW ("[mer][0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],fl);
     StoreData (dbase, this->device, 14, 0, 0, fl, 0);
    } 
 rs=send_mercury (READ_PARAMETRS, 0x11, I1, 0);
 if (rs)  rs = this->read_mercury(data, 0);
 if (rs)
    {
     fl=((float)(data[0]&0x3f)+(float)data[1]*256+(float)data[2]*256*256); fl=fl/1000;
     if (debug>2) ULOGW ("[mer][0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],fl);
     StoreData (dbase, this->device, 14, 10, 0, fl, 0);
    } 
 rs=send_mercury (READ_PARAMETRS, 0x11, U1, 0);
 if (rs)  rs = this->read_mercury(data, 0);
 if (rs)
    {
     fl=((float)(data[0]&0x3f)+(float)data[1]*256+(float)data[2]*256*256); fl=fl/100;
     if (debug>2) ULOGW ("[mer][0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],fl);
     StoreData (dbase, this->device, 14, 11, 0, fl, 0);
    } 
 rs=send_mercury (READ_PARAMETRS, 0x11, P_SUM, 0);
 if (rs)  rs = this->read_mercury(data, 0);
 if (rs)
    {
     fl=(float)(data[0]&0x3f)+(float)(data[1]*256)+(float)(data[2]*256*256); fl=fl/100;
     if (debug>2) ULOGW ("[mer][0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],fl);
     StoreData (dbase, this->device, 14, 12, 0, fl, 0);
    } 

 rs=send_mercury (READ_DATA_230, 0x0, 0x0, 0);
 if (rs)  rs = this->read_mercury(data, 0);
 if (rs)
    {
     fl=((float)(data[0]&0x3f)+(float)(data[1]*256)+(float)(data[2]*256*256)+(float)(data[3]*256*256*256)); fl=fl;
     if (debug>2) ULOGW ("[mer][0x%x 0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],data[3],fl);
     StoreData (dbase, this->device, 14, 21, 0, fl, 0);
    } 
 return 0;
}
//-----------------------------------------------------------------------------
// ReadDataArchive - read single device. Readed data will be stored in DB
int DeviceMER::ReadAllArchive (UINT tp)
{
 bool   rs;
 BYTE   data[400];
 CHAR   date[20];
 UINT	month,year,index;
 float  fl;
 UINT   code,vsk=0;
 time_t tims,tim;
 tims=time(&tims);
 struct tm tt;

 this->qatt++;  // attempt

 rs=send_mercury (READ_DATA_230, 0x30, 0x0, 0);
 if (rs)  rs = this->read_mercury(data, 0);
 if (rs)
    { 
     tim=time(&tim);
     tim-=3600*24;
     localtime_r(&tim,&tt);
     fl=((float)(data[0]&0x3f)+(float)data[1]*256+(float)data[2]*256*256+(float)data[3]*256*256*256); fl=fl;
     sprintf (this->lastdate,"%04d%02d%02d000000",tt.tm_year+1900,tt.tm_mon,tt.tm_mday); 
     if (debug>2) ULOGW ("[mer][0x%x 0x%x 0x%x 0x%x] [%f][%s]",data[0],data[1],data[2],data[3],fl,this->lastdate);
     StoreData (dbase, this->device, 14, 0, 2, 0, fl,this->lastdate, 0);
    } 

 tim=time(&tim);
 localtime_r(&tim,&tt);
 for (int i=0; i<tp; i++)
    {
     if (tt.tm_mon==0) tt.tm_mon=12;
     rs=send_mercury (READ_DATA_230, 0x30+tt.tm_mon, 0x0, 0);
     if (rs)  rs = this->read_mercury(data, 0);
     if (rs)
	    {
	     fl=((float)(data[0]&0x3f)+(float)data[1]*256+(float)data[2]*256*256+(float)data[3]*256*256*256); fl=fl;
             sprintf (this->lastdate,"%04d%02d01000000",tt.tm_year+1900,tt.tm_mon); 
	     if (debug>2) ULOGW ("[mer][0x%x 0x%x 0x%x 0x%x] [%f] [%s]",data[0],data[1],data[2],data[3],fl,this->lastdate);
	     StoreData (dbase, this->device, 14, 0, 4, 0, fl,this->lastdate, 0);
	    }
     tt.tm_mon--;
    } 
 return 0;
}
//--------------------------------------------------------------------------------------
// load all km configuration from DB
BOOL LoadMercuryConfig()
{
 uint16_t mer_num=dev_num[TYPE_MERCURY230];
 mer_num=0;
 for (UINT d=0;d<device_num;d++)
 if (dev[d].type==TYPE_MERCURY230)
    {
     sprintf (query,"SELECT * FROM dev_mer WHERE device=%d",dev[d].idd);
     res=dbase.sqlexec(query); 
     UINT nr=mysql_num_rows(res);
     for (UINT r=0;r<nr;r++)
	{
         row=mysql_fetch_row(res);
	 mer[mer_num].idmer=mer_num;
         if (!r)
	    {
    	     mer[mer_num].iddev=dev[d].id;
             mer[mer_num].device=dev[d].idd;
             mer[mer_num].SV=dev[d].SV;
             mer[mer_num].interface=dev[d].interface;
             mer[mer_num].protocol=dev[d].protocol;
             mer[mer_num].port=dev[d].port;
             mer[mer_num].speed=dev[d].speed;
             mer[mer_num].adr=dev[d].adr;
             mer[mer_num].type=dev[d].type;
             strcpy(mer[mer_num].number,dev[d].number);
             mer[mer_num].flat=dev[d].flat;
             mer[mer_num].akt=dev[d].akt;
             strcpy(mer[mer_num].lastdate,dev[d].lastdate);
             mer[mer_num].qatt=dev[d].qatt;
             mer[mer_num].qerrors=dev[d].qerrors;
             mer[mer_num].conn=dev[d].conn;
             strcpy(mer[mer_num].devtim,dev[d].devtim);
             mer[mer_num].chng=dev[d].chng;
             mer[mer_num].req=dev[d].req;
             mer[mer_num].source=dev[d].source;
             strcpy(mer[mer_num].name,dev[d].name);
            }    
        } 
     if (debug>0) ULOGW ("[mer] device [0x%x],adr=%d",mer[mer_num].device,mer[mer_num].adr);
     mer_num++;
    }
 if (res) mysql_free_result(res);
}
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 sprintf (devp,"/dev/ttyS%d",blok);
// sprintf (devp,"/dev/ttyS2",blok);
 if (debug>0) ULOGW("[ce] attempt open com-port %s on speed %d",devp,speed);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[ce] error open com-port %s [%s]",devp,strerror(errno)); 
     return false;
    }
 else if (debug>1) ULOGW("[ce] open com-port success"); 

 tcflush(fd, TCIOFLUSH); //Clear send & recive buffers
 tcgetattr(fd, &tio);
 cfsetospeed(&tio, baudrate(speed));
 tio.c_cflag |= CREAD|CLOCAL|baudrate(speed);
 
 tio.c_cflag &= ~(CSIZE|PARENB|PARODD);
 tio.c_cflag &= ~CSTOPB;
 tio.c_cflag |=CS8;
 tio.c_cflag &= ~CRTSCTS;
 tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
 tio.c_iflag = 0;

 tio.c_iflag &= ~(INLCR | IGNCR | ICRNL);
 tio.c_iflag &= ~(IXON | IXOFF | IXANY);
 
// tio.c_iflag = IGNCR;
 tio.c_oflag &= ~(ONLCR);
 
 tio.c_cc[VMIN] = 0;
 tio.c_cc[VTIME] = 5; //Time out in 10e-1 sec
 cfsetispeed(&tio, baudrate(speed));
 //fcntl(fd, F_SETFL, FNDELAY);
 fcntl(fd, F_SETFL, 0);
 tcsetattr(fd, TCSANOW, &tio);
 tcsetattr (fd,TCSAFLUSH,&tio);
 
 return true;
}

//rs=send_ce (CUR_REQUEST, this->addr[sens_num], this->addr[sens_num], 0);
//-----------------------------------------------------------------------------
BOOL DeviceMER::send_mercury (UINT op, UINT prm, UINT index, UINT frame)
    {
     UINT       crc=0;          //(* CRC checksum *)
     UINT       nbytes = 0;     //(* number of bytes in send packet *)
     BYTE       data[100];      //(* send sequence *)

     data[0]=this->adr&0xff; 
     data[1]=op;
     
     if (op==0x1) // open/close session
        {
     	 data[2]=2;
     	 data[3]=2; data[4]=2; data[5]=2; data[6]=2; data[7]=2; data[8]=2;
         crc=CRC (data, 9, 0);
         data[9]=crc/256;
	 data[10]=crc%256;
         if (debug>2) ULOGW("[mer][%d][%d] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",op,prm,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10]);
         write (fd,&data,11);
        }     
     if (op==0x4) // time
        {
	 data[2]=0x0;
	 crc=CRC (data, 3, 0);
         data[3]=crc/256;
	 data[4]=crc%256;
         if (debug>2) ULOGW("[mer][%d][%d] wr[0x%x,0x%x,0x%x,0x%x,0x%x]",op,prm,data[0],data[1],data[2],data[3],data[4]);
         write (fd,&data,5);
	}
     if (op==0x5 || op==0x11 || op==0x14) // nak
        {
	 data[2]=prm;
	 data[3]=index;
	 crc=CRC (data, 4, 0);
         data[4]=crc/256;
	 data[5]=crc%256;
         if (debug>2) ULOGW("[mer][%d][%d] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",op,prm,data[0],data[1],data[2],data[3],data[4],data[5]);
         write (fd,&data,6);
	}
     if (op==0x8) // parametrs
        {
	 data[2]=prm;
	 crc=CRC (data, 3, 0);
         data[3]=crc/256;
	 data[4]=crc%256;
         if (debug>2) ULOGW("[mer][%d][%d] wr[0x%x,0x%x,0x%x,0x%x,0x%x]",op,prm,data[0],data[1],data[2],data[3],data[4]);
         write (fd,&data,5);
	}
     return true;     
    }
//-----------------------------------------------------------------------------    
UINT  DeviceMER::read_mercury (uint8_t* dat, uint8_t type)
    {
     UINT       crc=0;		//(* CRC checksum *)
     INT        nbytes = 0;     //(* number of bytes in recieve packet *)
     INT        bytes = 0;      //(* number of bytes in packet *)
     uint8_t       data[500];      //(* recieve sequence *)
     UINT       i=0;            //(* current position *)
     CHAR       op=0;           //(* operation *)
     UINT       crc_in=0;	//(* CRC checksum *)

     sleep(1);
     ioctl (fd,FIONREAD,&nbytes); 
     if (debug>3) ULOGW("[mer] nbytes=%d",nbytes);
     nbytes=read (fd, &data, 75);
     if (debug>3) ULOGW("[mer] nbytes=%d %x",nbytes,data[0]);
     usleep (200000);
     ioctl (fd,FIONREAD,&bytes);  
     
//     if (bytes>0 && nbytes>0 && nbytes<50) 
//        {
//         if (debug>3) ULOGW("[ce] bytes=%d fd=%d adr=%d",bytes,fd,&data+nbytes);
//         bytes=read (fd, &data+nbytes, bytes);
//         if (debug>3) ULOGW("[ce] bytes=%d",bytes);
//         nbytes+=bytes;
//        }

     if (nbytes>2)
        {
	 crc=CRC (data+1, nbytes-3, 1);
         if (debug>2) ULOGW("[ce] [%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x][crc][0x%x,0x%x]",nbytes,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],crc/256,crc%256);  
	 //for (UINT rr=0; rr<nbytes;rr++) if (debug>2) ULOGW("[ce] [%d][0x%x]",rr,data[rr]);
	 crc_in=data[nbytes-2]*256+data[nbytes-1];
	 if (crc!=crc_in || nbytes<3) nbytes=0;
         memcpy (dat,data+1,nbytes-3);
         return nbytes;
        }
     return 0;
    }
//-----------------------------------------------------------------------------        
const uint8_t srCRCHi[] = {
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40};

const uint8_t srCRCLo[] = {
0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80, 0x40}; 

uint16_t CRC(const uint8_t* const Data, const uint8_t DataSize, uint8_t type)
    {
     uint16_t InitCRC = 0xffff; 
     uint16_t _CRC; 
     uint8_t	i,p;
     uint8_t arrCRC[2];

     for (p=0; p<DataSize; p++)
	{	
	 arrCRC[0]=InitCRC%256; arrCRC[1]=InitCRC/256;
	 i=arrCRC[1] ^ Data[p];
	 arrCRC[1] = arrCRC[0] ^ srCRCHi[i];
	 arrCRC[0] = srCRCLo[i];
         InitCRC=arrCRC[0]+arrCRC[1]*256;
	}
     _CRC = arrCRC[0]+arrCRC[1]*256;
     return _CRC;
    }