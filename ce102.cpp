//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "ce102.h"
#include "db.h"
#include "func.h"
#include <fcntl.h>
//-----------------------------------------------------------------------------
static  MYSQL_RES *res;
static  db      dbase;
static  MYSQL_ROW row;
static  CHAR    query[500];     

static  INT     fd;
static  termios tio;

extern  "C" UINT device_num;    // total device quant
extern  "C" UINT dev_num[30];  	// total device quant
extern  "C" BOOL WorkRegim;     // work regim flag
extern  "C" tm *currenttime;    // current system time

extern  "C" DeviceR     dev[MAX_DEVICE];// device class
extern  "C" DeviceCE  	ce[MAX_DEVICE_CE];
extern  "C" DeviceDK	dk;

extern  "C" BOOL	ce_thread;
extern  "C" UINT	debug;

//static  union fnm fnum[5];
//static  UINT    chan_num[MAX_DEVICE_KM]={0};

BOOL send_ce (UINT op, UINT prm, UINT frame, UINT index);
UINT read_ce (BYTE* dat, BYTE type);
static  BYTE CRC(const BYTE* const Data, const BYTE DataSize, BYTE type);
        
        VOID    ULOGW (const CHAR* string, ...);              // log function
        UINT    baudrate (UINT baud);                   // baudrate select
static  BOOL    OpenCom (SHORT blok, UINT speed);       // open com function
        VOID    Events (DWORD evnt, DWORD device);      // events 
static  BOOL    LoadCEConfig();                      // load tekon configuration
//static  BOOL    StoreData (UINT dv, UINT prm, UINT pipe, UINT status, FLOAT value);             // store data to DB
//static  BOOL    StoreData (UINT dv, UINT prm, UINT pipe, UINT type, FLOAT value, CHAR* data);
//-----------------------------------------------------------------------------

void * ceDeviceThread (void * devr)
{
 int devdk=*((int*) devr);                      // DK identificator
 dbase.sqlconn("dk","root","");                 // connect to database

 // load from db km device
 LoadCEConfig();
 // open port for work
 BOOL rs=OpenCom (ce[0].port, ce[0].speed);
 if (!rs) return (0);

 while (WorkRegim)
 for (UINT d=0;d<dev_num[TYPE_INPUTCE];d++)
    {
     if (debug>1) ULOGW ("[ce] ReadDataCurrent (%d)",d);
     ce[d].ReadDataCurrent (); 
     if (debug>1) ULOGW ("[ce] ReadDataArchive (%d)",d);
     ce[d].ReadAllArchive (10);
     if (!dk.pth[TYPE_INPUTCE])
        {
	 if (debug>0) ULOGW ("[ce] ce thread stopped");
	 //dbase.sqldisconn();
	 //pthread_exit ();	 
	 ce_thread=false;
	 pthread_exit (NULL);
	 return 0;	 
	}          
    }
 dbase.sqldisconn();
 if (debug>0) ULOGW ("[ce] CE102 thread end");
 ce_thread=false;
 pthread_exit (NULL); 
}
//-----------------------------------------------------------------------------
// ReadDataCurrent - read single device. Readed data will be stored in DB
int DeviceCE::ReadDataCurrent ()
{
 UINT   rs;
 float  fl;
 BYTE   data[400];
 CHAR   date[20];
 this->qatt++;  // attempt

 rs=send_ce (CURRENT_W, 0, 0, 0);
 if (rs)  rs = this->read_ce(data, 0);
 if (rs)
    {
     fl=((float)data[0]+(float)data[1]*256+(float)data[2]*256*256)/100;
     if (debug>2) ULOGW ("[ce][0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],fl);
     StoreData (dbase, this->device, 14, 0, 0, fl, 0);
    } 

 rs=send_ce (NAK_W, 0, 0, 1);
 if (rs)  rs = this->read_ce(data, 0);
 if (rs)
    {
     fl=((float)data[0]+(float)data[1]*256+(float)data[2]*256*256)/100;
     if (debug>2) ULOGW ("[ce][0x%x 0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],data[3],fl);
     StoreData (dbase, this->device, 2, 0, 0, fl, 0);
    } 
 rs=send_ce (NAK_W1, 0, 1, 2);
 if (rs)  rs = this->read_ce(data, 0);
 if (rs)
    {
     fl=((float)data[0]+(float)data[1]*256+(float)data[2]*256*256)/100;
     if (debug>2) ULOGW ("[ce][0x%x 0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],data[3],fl);
     StoreData (dbase, this->device, 2, 1, 0, fl, 0);
    } 
 rs=send_ce (NAK_W2, 1, 1, 2);
 if (rs)  rs = this->read_ce(data, 0);
 if (rs)
    {
     fl=((float)data[0]+(float)data[1]*256+(float)data[2]*256*256)/100;
     if (debug>2) ULOGW ("[ce][0x%x 0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],data[3],fl);
     StoreData (dbase, this->device, 2, 2, 0, fl, 0);
    } 
 return 0;
}
//-----------------------------------------------------------------------------
// ReadDataArchive - read single device. Readed data will be stored in DB
int DeviceCE::ReadAllArchive (UINT tp)
{
 bool   rs;
 BYTE   data[400];
 CHAR   date[20];
 UINT	month,year,index;
 float  fl;
 UINT   code,vsk=0;
 time_t tims;
 tims=time(&tims);
 struct tm *prevtime;		// current system time 
 this->qatt++;  // attempt

 rs=send_ce (NAK_W, 0, 0, 1);
 if (rs)  rs = this->read_ce(data, 0);
 if (rs)
    {
     fl=((float)data[0]+(float)data[1]*256+(float)data[2]*256*256+(float)data[3]*256*256*256)/100;
     if (debug>2) ULOGW ("[ce][0x%x 0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],data[3],fl);
     sprintf (this->lastdate,"%04d%02d%02d%02d0000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour);
     StoreData (dbase, this->device, 2, 0, 1, 0, fl, this->lastdate, 0);
    } 
 rs=send_ce (NAK_W1, 0, 1, 2);
 if (rs)  rs = this->read_ce(data, 0);
 if (rs)
    {
     fl=((float)data[0]+(float)data[1]*256+(float)data[2]*256*256+(float)data[3]*256*256*256)/100;
     if (debug>2) ULOGW ("[ce][0x%x 0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],data[3],fl);
     sprintf (this->lastdate,"%04d%02d%02d%02d0000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour);
     StoreData (dbase, this->device, 2, 1, 1, 0, fl, this->lastdate, 0);
    } 
 rs=send_ce (NAK_W2, 1, 1, 2);
 if (rs)  rs = this->read_ce(data, 0);
 if (rs)
    {
     fl=((float)data[0]+(float)data[1]*256+(float)data[2]*256*256+(float)data[3]*256*256*256)/100;
     if (debug>2) ULOGW ("[ce][0x%x 0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],data[3],fl);
     sprintf (this->lastdate,"%04d%02d%02d%02d0000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour);
     StoreData (dbase, this->device, 2, 2, 1, 0, fl, this->lastdate, 0);
    } 
 for (int i=0; i<tp; i++)
    {
     rs=send_ce (HOUR_W, 1, i, 3);
     if (rs)  rs = this->read_ce(data, 0);
     if (rs)
        {
	 if (data[0]==0xff && data[1]==0xff && data[2]==0xff) fl=-1;
         else fl=((float)data[0]+(float)data[1]*256+(float)data[2]*256*256)/100;
         if (debug>2) ULOGW ("[ce][0x%x 0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],data[3],fl);
         time_t tim;
	 tim=time(&tim);
         tim-=3600*i;
         struct tm tt;
         localtime_r(&tim,&tt);
         sprintf (this->lastdate,"%04d%02d%02d%02d0000",tt.tm_year+1900,tt.tm_mon+1,tt.tm_mday,tt.tm_hour);     
         if (fl>0) StoreData (dbase, this->device, 14, 0, 1, 0, fl, this->lastdate, 0);
	} 
    }
 return 0;
}
//--------------------------------------------------------------------------------------
// load all km configuration from DB
BOOL LoadCEConfig()
{
 UINT ce_num=dev_num[TYPE_INPUTCE];
 for (UINT d=0;d<device_num;d++)
 if (dev[d].type==TYPE_INPUTCE)
    {
     sprintf (query,"SELECT * FROM dev_ce WHERE device=%d",dev[d].idd);
     res=dbase.sqlexec(query); 
     UINT nr=mysql_num_rows(res);
     for (UINT r=0;r<nr;r++)
	{
         row=mysql_fetch_row(res);
	 ce[ce_num].idce=ce_num;
         if (!r)
	    {
    	     ce[ce_num].iddev=dev[d].id;
             ce[ce_num].device=dev[d].idd;
             ce[ce_num].SV=dev[d].SV;
             ce[ce_num].interface=dev[d].interface;
             ce[ce_num].protocol=dev[d].protocol;
             ce[ce_num].port=dev[d].port;
             ce[ce_num].speed=dev[d].speed;
             ce[ce_num].adr=dev[d].adr;
             ce[ce_num].type=dev[d].type;
             strcpy(ce[ce_num].number,dev[d].number);
             ce[ce_num].flat=dev[d].flat;
             ce[ce_num].akt=dev[d].akt;
             strcpy(ce[ce_num].lastdate,dev[d].lastdate);
             ce[ce_num].qatt=dev[d].qatt;
             ce[ce_num].qerrors=dev[d].qerrors;
             ce[ce_num].conn=dev[d].conn;
             strcpy(ce[ce_num].devtim,dev[d].devtim);
             ce[ce_num].chng=dev[d].chng;
             ce[ce_num].req=dev[d].req;
             ce[ce_num].source=dev[d].source;
             strcpy(ce[ce_num].name,dev[d].name);
            }    
        } 
     if (debug>0) ULOGW ("[ce] device [0x%x],adr=%d",ce[ce_num].device,ce[ce_num].adr);
     ce_num++;
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
BOOL DeviceCE::send_ce (UINT op, UINT prm, UINT index, UINT frame)
    {
     UINT       crc=0;          //(* CRC checksum *)
     UINT       nbytes = 0;     //(* number of bytes in send packet *)
     BYTE       data[100];      //(* send sequence *)

     // C0 48 	   75 00    FF 00   00 00 00 00   D2 01 30 01 00 
     data[0]=0xc0;
     data[1]=0x48;     
     data[2]=this->adr&0xff; 
     data[3]=(this->adr&0xff00)>>8;
     data[4]=0xff;
     data[5]=0x00; data[6]=0x00; data[7]=0x00; data[8]=0x00; data[9]=0x00;
     data[11]=(op&0xff00)>>8;
     data[12]=op&0xff;
     
     if (frame==0) // conf & current
        {
	 data[10]=0xd0;
         data[13]=CRC (data+1, 12, 1);
         data[14]=0xc0;
         if (debug>2) ULOGW("[ce][0] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14]);
         write (fd,&data,15);
        }     
     if (frame==1) // nak -sum
        {
	 data[10]=0xd1;
	 data[13]=0x1;
	 data[14]=CRC (data+1, 13, 1);
         data[15]=0xc0;	 
         if (debug>2) ULOGW("[ce][1] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15]);
         write (fd,&data,16);
	}
     if (frame==2) // nak - night|day
        {
	 data[10]=0xd2;
	 data[13]=prm;
	 data[14]=index;
	 data[15]=CRC (data+1, 14, 1);
         data[16]=0xc0;	 
         if (debug>2) ULOGW("[ce][2] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16]);
         write (fd,&data,17);
	}
     if (frame==3) // hours
        {
         time_t tim;
	 tim=time(&tim);
         tim-=3600*index;
         struct tm tt;
         localtime_r(&tim,&tt);
	 data[10]=0xd5;
//         sprintf (this->lastdate,"%04d%02d%02d%02d%02d%02d",tt.tm_year+1900,tt.tm_mon,tt.tm_mday,tt.tm_hour,tt.tm_min,tt.tm_sec);     
 	 data[13]=((tt.tm_mday/10)<<4)+tt.tm_mday%10;
	 data[14]=(((tt.tm_mon+1)/10)<<4)+(tt.tm_mon+1)%10; 
	 data[15]=(((tt.tm_year-100)/10)<<4)+(tt.tm_year-100)%10;
 	 //data[16]=(tt.tm_hour/10<<4)+tt.tm_hour%10;
	 data[16]=tt.tm_hour;
	 data[17]=prm;
	 data[18]=CRC (data+1, 17, 1);
         data[19]=0xc0;	 
         if (debug>2) ULOGW("[ce][3] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18]);
         write (fd,&data,20);
	}

     return true;     
    }
//-----------------------------------------------------------------------------    
UINT  DeviceCE::read_ce (BYTE* dat, BYTE type)
    {
     UINT       crc=0;		//(* CRC checksum *)
     INT        nbytes = 0;     //(* number of bytes in recieve packet *)
     INT        bytes = 0;      //(* number of bytes in packet *)
     BYTE       data[500];      //(* recieve sequence *)
     UINT       i=0;            //(* current position *)
     UCHAR      ok=0xFF;        //(* flajochek *)
     CHAR       op=0;           //(* operation *)

     //usleep (50000);
     sleep(1);
     ioctl (fd,FIONREAD,&nbytes); 
     if (debug>3) ULOGW("[ce] nbytes=%d",nbytes);
     nbytes=read (fd, &data, 75);
     if (debug>3) ULOGW("[ce] nbytes=%d %x",nbytes,data[0]);
     usleep (200000);
     ioctl (fd,FIONREAD,&bytes);  
     
     if (bytes>0 && nbytes>0 && nbytes<50) 
        {
         if (debug>3) ULOGW("[ce] bytes=%d fd=%d adr=%d",bytes,fd,&data+nbytes);
         bytes=read (fd, &data+nbytes, bytes);
         if (debug>3) ULOGW("[ce] bytes=%d",bytes);
         nbytes+=bytes;
        }

     if (nbytes>5)
        {
	 crc=CRC (data+1, nbytes-3, 1);
         if (debug>2) ULOGW("[ce] [%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x][crc][0x%x,0x%x]",nbytes,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[nbytes-2],crc);  
	 //for (UINT rr=0; rr<nbytes;rr++) if (debug>2) ULOGW("[ce] [%d][0x%x]",rr,data[rr]);
	 if (crc!=data[nbytes-2] || nbytes<12) nbytes=0;
         memcpy (dat,data+9,7);
         return nbytes;
        }
     return 0;
    }
//-----------------------------------------------------------------------------        
BYTE CRC(const BYTE* const Data, const BYTE DataSize, BYTE type)
    {
     BYTE _CRC = 0;     
     const unsigned char crc8tab[256] = {
	0x00, 0xb5, 0xdf, 0x6a, 0x0b, 0xbe, 0xd4, 0x61, 0x16, 0xa3, 0xc9, 0x7c, 0x1d, 0xa8,
	0xc2, 0x77, 0x2c, 0x99, 0xf3, 0x46, 0x27, 0x92, 0xf8, 0x4d, 0x3a, 0x8f, 0xe5, 0x50,
	0x31, 0x84, 0xee, 0x5b, 0x58, 0xed, 0x87, 0x32, 0x53, 0xe6, 0x8c, 0x39, 0x4e, 0xfb,
	0x91, 0x24, 0x45, 0xf0, 0x9a, 0x2f, 0x74, 0xc1, 0xab, 0x1e, 0x7f, 0xca, 0xa0, 0x15,
	0x62, 0xd7, 0xbd, 0x08, 0x69, 0xdc, 0xb6, 0x03, 0xb0, 0x05, 0x6f, 0xda, 0xbb, 0x0e,
	0x64, 0xd1, 0xa6, 0x13, 0x79, 0xcc, 0xad, 0x18, 0x72, 0xc7, 0x9c, 0x29, 0x43, 0xf6,
	0x97, 0x22, 0x48, 0xfd, 0x8a, 0x3f, 0x55, 0xe0, 0x81, 0x34, 0x5e, 0xeb, 0xe8, 0x5d,
	0x37, 0x82, 0xe3, 0x56, 0x3c, 0x89, 0xfe, 0x4b, 0x21, 0x94, 0xf5, 0x40, 0x2a, 0x9f,
	0xc4, 0x71, 0x1b, 0xae, 0xcf, 0x7a, 0x10, 0xa5, 0xd2, 0x67, 0x0d, 0xb8, 0xd9, 0x6c,
	0x06, 0xb3, 0xd5, 0x60, 0x0a, 0xbf, 0xde, 0x6b, 0x01, 0xb4, 0xc3, 0x76, 0x1c, 0xa9,
	0xc8, 0x7d, 0x17, 0xa2, 0xf9, 0x4c, 0x26, 0x93, 0xf2, 0x47, 0x2d, 0x98, 0xef, 0x5a,
	0x30, 0x85, 0xe4, 0x51, 0x3b, 0x8e, 0x8d, 0x38, 0x52, 0xe7, 0x86, 0x33, 0x59, 0xec,
	0x9b, 0x2e, 0x44, 0xf1, 0x90, 0x25, 0x4f, 0xfa, 0xa1, 0x14, 0x7e, 0xcb, 0xaa, 0x1f,
	0x75, 0xc0, 0xb7, 0x02, 0x68, 0xdd, 0xbc, 0x09, 0x63, 0xd6, 0x65, 0xd0, 0xba, 0x0f,
	0x6e, 0xdb, 0xb1, 0x04, 0x73, 0xc6, 0xac, 0x19, 0x78, 0xcd, 0xa7, 0x12, 0x49, 0xfc,
	0x96, 0x23, 0x42, 0xf7, 0x9d, 0x28, 0x5f, 0xea, 0x80, 0x35, 0x54, 0xe1, 0x8b, 0x3e,
	0x3d, 0x88, 0xe2, 0x57, 0x36, 0x83, 0xe9, 0x5c, 0x2b, 0x9e, 0xf4, 0x41, 0x20, 0x95,
	0xff, 0x4a, 0x11, 0xa4, 0xce, 0x7b, 0x1a, 0xaf, 0xc5, 0x70, 0x07, 0xb2, 0xd8, 0x6d,
	0x0c, 0xb9, 0xd3, 0x66 };
    _CRC = 0;
    for(int i= 0; i<DataSize; i++){
	_CRC= crc8tab[_CRC ^ Data[i]];
    }
    return _CRC;
}
