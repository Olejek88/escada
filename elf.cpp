//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "elf.h"
#include "db.h"
#include "func.h"
//-----------------------------------------------------------------------------
static	db		dbase;
static 	INT		fd;
static 	termios 	tio;

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
extern	UINT	debug;
extern	UINT 	elf_thread;

VOID 	ULOGW (const CHAR* string, ...);	// log function
UINT 	baudrate (UINT baud);			// baudrate select
static  UINT CRC (const BYTE* const Data, const BYTE DataSize);
UINT 	CalcCRC(unsigned int c,unsigned int crc);
//VOID 	Events (DWORD evnt, DWORD device);	// events 
unsigned short Crc16( unsigned char *pcBlock, unsigned short len );

static 	BOOL 	OpenCom (SHORT blok, UINT speed);	// open com function
static	BOOL 	LoadELFConfig();			// load all irp configuration
//static	VOID	StartSimulate(UINT type); 
static	BOOL 	ParseData (UINT type, UINT dv, BYTE* data, UINT len, UINT prm);
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
     //elf[r].ReadTime ();
    }

 sleep (5);
 while (WorkRegim)
    {
     for (UINT r=0;r<elf_num;r++)
        {
         if (debug>3) ULOGW ("[elf] elf[%d/%d].ReadLastArchive ()",r,elf_num);
	 elf[r].ReadData (0);
	 if (currenttime->tm_min<10) 	elf[r].ReadData (1);
	 if (currenttime->tm_hour<12)	elf[r].ReadData (2);
	 if (currenttime->tm_mday<10)	elf[r].ReadData (3);
	 sleep (3);
	}
     sleep (1);     
    }
 dbase.sqldisconn();
 if (debug>0) ULOGW ("[elf] elf thread end");
 elf_thread=false;
 pthread_exit (NULL);
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
 int 	rs;
 BYTE	data[800];
 rs = this->send_elf(FUNC_VERSION,type,0,(UCHAR*)"");
 usleep(200000);
 if (rs) rs = this->read_elf(data);
 if (rs)
    {
     sprintf (this->version,"%d%d%d%d%d%d%d%d",data[17],data[18],data[19],data[20],data[21],data[22],data[23],data[24],data[25]);
     sprintf (this->software,"%d%d%d",data[25],data[26],data[27]);
     if (debug>0) ULOGW ("[elf] ELF [%s] software version [%s]",this->version,this->software);
    }
 else ULOGW ("[elf] no ELF found");    
 return true;
}
//---------------------------------------------------------------------------------------------------
// ReadLastArchive - read last archive and store to db
int DeviceELF::ReadData (UINT	type)
{
 int 	rs;
 BYTE	data[8000];
 CHAR	date[50];
 FLOAT	q, t1, t2, v1, v2;

 this->qatt++;	// attempt
 rs = this->send_elf(type,type,0,(UCHAR*)"");
 usleep(200000);
 if (rs) rs = this->read_elf(data);
 if (!rs)
    {
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>5) this->akt=0; // !!! 5 > normal quant from DB
     if (this->qerrors==6) Events (dbase,(3*0x10000000)|(TYPE_IRP*0x1000000)|SEND_PROBLEM,this->device);
    }
 if (rs>6)
    {
     if (debug>2) ULOGW ("[elf] AP[%x] AO[%x] C[%x] LL[%x] LH[%x] CSH[%x] CSL[%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
     if (debug>2) ULOGW ("[elf] SOH[%x] LEN[%d] FNC[%x] IDN[%x %x %x %x] typ[%x] N[%d]",data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15]);
     if (debug>2) ULOGW ("[elf] HeaD[%x %d %d %d] N1[%x %x] N2[%x %x] Q[%x %x %x %x] T1[%x %x %x %x] T2[%x %x %x %x]",data[16],data[17],data[18],data[19],data[20],data[21],data[22],data[23],data[24],data[25],data[26],data[27],data[28],data[29],data[30],data[31],data[32],data[33],data[34],data[35],data[36]);
     q=*(float*)(data+24);
     t1=*(float*)(data+28);
     t2=*(float*)(data+32);
     v1=*(float*)(data+36);
     v2=*(float*)(data+40);
     if (type==1 || type==0) if (debug>2) ULOGW ("[elf] [%02d-%02d-%04d %02d:00:00] q=%f, t1=%f, t2=%f, v1=%f, v2=%f",data[18],data[17],currenttime->tm_year,data[16],q,t1,t2,v1,v2);
     if (type==2) if (debug>2) ULOGW ("[elf] [%02d-%02d-%04d 00:00:00] q=%f, t1=%f, t2=%f, v1=%f, v2=%f",data[18],data[17],data[16],q,t1,t2,v1,v2);

     if (type)
        {
         if (type==0 || type==1) sprintf (date,"%04d%02d%02d%02d0000",currenttime->tm_year+1900,data[17],data[16],data[18]);
    	 if (type==2) sprintf (date,"%04d%02d%02d000000",data[18]+2000,data[17],data[16]);
	 //if (debug>3)  ULOGW ("[elf][%d] date=%s (%f)",prm,date,value);

         if (q<100000) StoreData (dbase, this->device, 13, type, 0, q, 0, date, 0);
	 if (v1<100000)StoreData (dbase, this->device, 11, type, 0, v1, 0, date, 0);
         if (t1<1000)StoreData (dbase, this->device, 4,  type, 0, t1,0, date, 0);
	 if (v2<100000)StoreData (dbase, this->device, 11, type, 0, v2, 1, date, 0);
         if (t2<1000)StoreData (dbase, this->device, 4,  type, 0, t2,1, date, 0);
	}
     else
        {
         StoreData (dbase, this->device, 13, 0, q, 0, 0);
	 StoreData (dbase, this->device, 11, 0, v1, 0, 0);
	 StoreData (dbase, this->device, 11, 0, v2, 1, 0);
	}
     this->akt=1;
     this->qerrors=0;
     this->conn=0;
     sprintf (this->lastdate,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,currenttime->tm_mon,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);
    }
 else
    {
     this->qerrors++;
     this->conn=0;
     if (this->qerrors==6) Events (dbase,(3*0x10000000)|(TYPE_IRP*0x1000000)|RECIEVE_PROBLEM,this->device);
    }
 return 0;
}
//---------------------------------------------------------------------------------------------------
int DeviceELF::read_elf (BYTE* dat)
{
 UINT	crc=0;		//(* CRC checksum *)
 UINT	nbytes = 0; 	//(* number of bytes in recieve packet *)
 UINT	bytes = 0; 	//(* number of bytes in packet *)
 UINT	reg = 0; 	//(* reg begin *)
 BYTE	data[500];	//(* recieve sequence *)
 UINT	i=0;		//(* current position *)
 UCHAR	ok=0xFF;	//(* flajochek *)
 CHAR	op=0;
 UINT	st=0;
 data[498]=0xff;
 
 ioctl (fd,FIONREAD,&nbytes); 
 if (debug>2) ULOGW ("[elf] read (%d)",nbytes);
 if (nbytes>0) nbytes=read (fd, &data, 10); 
 if (debug>2) ULOGW ("[elf]rd[%d] (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",bytes,data[0],data[1],data[2],data[3],data[4],data[5],data[6]); 
 if (nbytes>6)
    {    
     write (fd,data+498,1);
     usleep (100000);
     bytes+=nbytes;
     nbytes=read (fd, data+bytes, 80);
     if (debug>2) ULOGW ("[elf] read (%d)",nbytes);
     //if (debug>2) ULOGW ("[elf]rd[%d] (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",bytes,data[0],data[1],data[2],data[3],data[4],data[5],data[6]); 
     write (fd,data+498,1);
     bytes+=nbytes;     
     if (nbytes>0) { nbytes=read (fd, data+bytes, 80);  bytes+=nbytes; }
     if (debug>2) ULOGW ("[elf]rd[%d] (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",bytes,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19]);
    }     
    
 if (debug>3) ULOGW ("[elf] memcpy (%x,%x,%d)",dat,data,bytes);
 if (bytes<200) memcpy (dat,data,bytes);
 return bytes;
}
//---------------------------------------------------------------------------------------------------
bool DeviceELF::send_elf (UINT op, UINT reg, UINT nreg, UCHAR* dat)
{
 if (debug>3) ULOGW ("[elf] send_elf (%d, %d, %d)",op,reg,nreg);
 
 UINT	crc=0;		//(* CRC checksum *)
 UINT	nbytes = 0; 	//(* number of bytes in send packet *)
 UINT	bytes = 0; 	//(* number of bytes in send packet *)
 BYTE	data[100];	//(* send sequence *)
 
 data[0]=this->adr;
 data[1]=0xff;
 if (op<4) data[2]=0x18;
 else data[2]=op;
 data[3]=0;		// lenght will be add later
 data[4]=0;
 data[5]=0;		// crc will be add later
 data[6]=0;
 data[7]=0xf1; 
 data[8]=0x80;
 data[14]=0x0; 
 data[15]=0x0;

 switch (op)
    {
        case 0x0: data[9]=FUNC_DATA;
		  data[10]=0x0;
		  data[11]=0x0;
		  data[12]=0x0;
		  data[13]=0xff;
	  	  data[14]=0x2;
		  crc = CRC (data+7, 8);
		  data[15] = crc/256;
		  data[16] = crc%256;
		  data[3] = 3;		// only data frame
		  crc = CRC (data, 5);
		  data[5] = crc/256;	// crc of header
		  data[6] = crc%256;
		  data[17] = 0xf4;

		  write (fd,&data,7);
		  if (debug>2) ULOGW ("[elf %d][0] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",this->adr,data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
		  nbytes=read (fd, &data, 1); 
		  if (debug>2) ULOGW ("[elf %d][0] rd (0x%x)",this->adr,data[0]);
		  if (debug>2) ULOGW ("[elf %d][0] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",this->adr,data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16]);
		  write (fd,data+7,10);
		  nbytes=read (fd, &data, 1);
		  if (debug>2) ULOGW ("[elf %d][0] rd (0x%x)",this->adr,data[0]);
		  write (fd,data+17,1);
		  if (debug>2) ULOGW ("[elf %d][0] wr (0x%x)",this->adr,data[17]);
		  break;
        case 0x1: data[9]=FUNC_DATA;
		  data[10]=0x0;
		  data[11]=0x0;
		  data[12]=0x0;
		  data[13]=0xff;
	  	  data[14]=0x2;
		  crc = CRC (data+7, 8);
		  data[15] = crc/256;
		  data[16] = crc%256;
		  data[17] = 0xf4;
		  data[3] = 10;		// only data frame
		  crc = CRC (data, 5);
		  data[5] = crc/256;	// crc of header
		  data[6] = crc%256;
		  write (fd,&data,7);
		  if (debug>2) ULOGW ("[elf %d][1] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",this->adr,data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
		  nbytes=read (fd, &data, 1); 
		  if (debug>2) ULOGW ("[elf %d][1] rd (0x%x)",this->adr,data[0]);
		  if (debug>2) ULOGW ("[elf %d][1] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",this->adr,data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16]);
		  write (fd,&data+7,10);
		  nbytes=read (fd, &data, 1);
		  if (debug>2) ULOGW ("[elf %d][1] rd (0x%x)",this->adr,data[0]);
		  write (fd,&data+17,1);
		  if (debug>2) ULOGW ("[elf %d][1] wr (0x%x)",this->adr,data[17]);
		  break;
        case 0x2: data[9]=FUNC_DATA;
		  data[10]=0x0;
		  data[11]=0x0;
		  data[12]=0xff;
		  data[13]=0xff;
	  	  data[14]=0x2;
		  crc = CRC (data+7, 8);
		  data[15] = crc/256;
		  data[16] = crc%256;
		  data[17] = 0xf4;
		  data[3] = 10;		// only data frame
		  crc = CRC (data, 5);
		  data[5] = crc/256;	// crc of header
		  data[6] = crc%256;

		  write (fd,&data,7);
		  if (debug>2) ULOGW ("[elf %d][2] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",this->adr,data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
		  nbytes=read (fd, &data, 1); 
		  if (debug>2) ULOGW ("[elf %d][2] rd (0x%x)",this->adr,data[0]);
		  if (debug>2) ULOGW ("[elf %d][2] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",this->adr,data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16]);
		  write (fd,data+7,10);
		  nbytes=read (fd, &data, 1);
		  if (debug>2) ULOGW ("[elf %d][2] rd (0x%x)",this->adr,data[0]);
		  write (fd,data+17,1);
		  if (debug>2) ULOGW ("[elf %d][2] wr (0x%x)",this->adr,data[17]);		  
		  break;

        case FUNC_VERSION: 
		  //data[9]=FUNC_VERSION;
		  data[9]=0x4;
		  data[10]=0x3;
		  data[11]=0x45;
		  data[12]=0x4f;
		  data[13]=0x29;
		  crc = CRC (data+7, 9);
		  data[14] = crc/256;
		  data[15] = crc%256;
		  data[16] = 0xf4;
    		  data[3] = 4;		// only data frame

		  crc = CRC (data, 7);
		  if (debug>2) ULOGW ("[elf %d][crc] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x) [%u]",this->adr,data[0],data[1],data[2],data[3],data[4],data[5],data[6],crc);

		  data[5] = crc/256;	// crc of header
		  data[6] = crc%256;
		  write (fd,&data,7);
		  if (debug>2) ULOGW ("[elf %d][2] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",this->adr,data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
		  ioctl (fd,FIONREAD,&bytes);
		  nbytes=read (fd, &data, 1); 
		  if (debug>2) ULOGW ("[elf %d][%d] rd (0x%x)",this->adr,nbytes,data[0]);
		  if (debug>2) ULOGW ("[elf %d][2] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",this->adr,data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15]);
		  usleep(10000);
		  write (fd,data+7,9);
		  nbytes=read (fd, &data, 2);
		  if (debug>2) ULOGW ("[elf %d][%d] rd (0x%x)",this->adr,nbytes,data[0]);
		  usleep(10000);
		  write (fd,data+16,1);
		  usleep(10000);
		  if (debug>2) ULOGW ("[elf %d][2] wr (0x%x)",this->adr,data[16]);
		  break;
    }
return true;
}
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 sprintf (devp,"/dev/ttyS%d",blok);
 sprintf (devp,"/dev/ttyUSB0");

 //sprintf (devp,"/dev/ttyM1",blok);
 if (debug>0) ULOGW("[elf] attempt open com-port %s on speed %d",devp,speed);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[elf] error open com-port %s [%s]",devp,strerror(errno)); 
     return false;
    }
 else if (debug>1) ULOGW("[elf] open com-port success"); 

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

 if (debug>1) ULOGW("[elf] c[0x%x] l[0x%x] i[0x%x] o[0x%x]",tio.c_cflag,tio.c_lflag,tio.c_iflag,tio.c_oflag);  
 
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
