//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "irp.h"
#include "db.h"
#include "func.h"
//-----------------------------------------------------------------------------
static	db	dbase;
static 	INT	fd;
static 	termios 	tio;

static	 MYSQL_RES *res;
static	 MYSQL_ROW row;
static	 CHAR   	query[500];

static	UINT	data_adr;	// for test we remember what we send to device
static	UINT	data_len;	// data address and lenght
static	UINT	data_op;	// and last operation

extern 	"C" UINT device_num;	// total device quant
extern 	"C" UINT irp_num;	// LK sensors quant
extern 	"C" BOOL WorkRegim;	// work regim flag
extern 	"C" tm *currenttime;	// current system time

extern 	"C" DeviceR 	dev[MAX_DEVICE];	// device class
extern 	"C" DeviceIRP 	irp[MAX_DEVICE_IRP];	// IRP class
extern  "C" DeviceDK	dk;

extern  "C" BOOL 	irp_thread;
extern  "C" UINT	debug;

static 	union fnm fnum[5];

float	irpData	[MAX_DEVICE_IRP][6];

VOID 	ULOGW (const CHAR* string, ...);		// log function
UINT 	baudrate (UINT baud);			// baudrate select
UCHAR 	Crc8 (BYTE* data, UINT lent);		// crc checksum function
//VOID 	Events (DWORD evnt, DWORD device);		// events 
//VOID 	Events (DWORD evnt, DWORD device, FLOAT value);	// events 

BOOL  hz=false;

static 	BOOL 	OpenCom (SHORT blok, UINT speed);	// open com function
static	BOOL 	LoadIRPConfig();			// load all irp configuration
static	VOID	StartSimulate(UINT type); 
static	BOOL 	ParseData (UINT type, UINT dv, BYTE* data, UINT len, UINT prm);

//static	BOOL 	StoreData (UINT dv, UINT prm, UINT type, UINT status, FLOAT value, UINT pipe, BYTE* data);
//static  BOOL 	StoreData (UINT dv, UINT prm, UINT status, FLOAT value, UINT pipe);
static  BOOL 	StoreData (UINT dv, BYTE* data, CHAR* text);
//-----------------------------------------------------------------------------
void * irpDeviceThread (void * devr)
{
 int devdk=*((int*) devr);			// DK identificator
 dbase.sqlconn("dk","root","");			// connect to database
 // StartSimulate(1); 
 // load from db all irp devices > store to class members
 LoadIRPConfig();
 // open port for work
 if (irp_num>0)
    {
     BOOL rs=OpenCom (irp[0].port, irp[0].speed);
    }
 else return (0);

 for (UINT r=0;r<irp_num;r++)
    {
     if (debug>1) ULOGW ("[irp] irp[%d/%d].ReadLastArchive0 ()",r,irp_num);
     irp[r].ReadLastArchive (25);
     irp[r].ReadDataCurrent ();
    }

 sleep (5);
 while (WorkRegim)
    {
     for (UINT r=0;r<irp_num;r++)
        {
         if (debug>4) ULOGW ("[irp] irp[%d/%d].ReadLastArchive ()",r,irp_num);
	 irp[r].ReadLastArchive (10);
	 sleep (1);
	}
     for (UINT r=0;r<irp_num;r++)
        {
         if (debug>4) ULOGW ("[irp] irp[%d/%d].ReadDataCurrent ()",r,irp_num);
	 //if (irp[r].akt==1 || currenttime->tm_min<20)
	 irp[r].ReadDataCurrent ();
	 sleep (1);
	}
     //if (currenttime->tm_min==0) StartSimulate(1);
     if (0)
     if (!dk.pth[TYPE_IRP])
        {
	 if (debug>0) ULOGW ("[irp] irp thread stopped");
	 //dbase.sqldisconn();
	 //pthread_exit ();	 
	 irp_thread=false;
	 pthread_exit (NULL);
	 return 0;	 
	}     
     sleep (1);     
     //dbase.sqldisconn();
     //dbase.sqlconn("dk","root","");			// connect to database
    }
 dbase.sqldisconn();
 if (debug>0) ULOGW ("[irp] irp thread end");
 irp_thread=false;
 pthread_exit (NULL);
}
//--------------------------------------------------------------------------------------
// load all IRP configuration from DB
BOOL LoadIRPConfig()
{
 sprintf (query,"SELECT * FROM dev_irp");
 res=dbase.sqlexec(query); 
 UINT nr=mysql_num_rows(res);
 for (UINT r=0;r<nr;r++)
    {
     row=mysql_fetch_row(res);
     irp[irp_num].idirp=atoi(row[0]);
     irp[irp_num].device=atoi(row[1]);     
     for (UINT d=0;d<device_num;d++)
        {
         if (dev[d].idd==irp[irp_num].device)
            {
    	     irp[irp_num].iddev=dev[d].id;
	     irp[irp_num].SV=dev[d].SV;
    	     irp[irp_num].interface=dev[d].interface;
	     irp[irp_num].protocol=dev[d].protocol;
	     irp[irp_num].port=dev[d].port;
	     irp[irp_num].speed=dev[d].speed;
	     irp[irp_num].adr=dev[d].adr;
	     irp[irp_num].type=dev[d].type;
	     strcpy(irp[irp_num].number,dev[d].number);
	     irp[irp_num].flat=dev[d].flat;
	     irp[irp_num].akt=dev[d].akt;
	     strcpy(irp[irp_num].lastdate,dev[d].lastdate);
	     irp[irp_num].qatt=dev[d].qatt;
	     irp[irp_num].qerrors=dev[d].qerrors;
	     irp[irp_num].conn=dev[d].conn;
	     strcpy(irp[irp_num].devtim,dev[d].devtim);
	     irp[irp_num].chng=dev[d].chng;
	     irp[irp_num].req=dev[d].req;
	     irp[irp_num].source=dev[d].source;
	     strcpy(irp[irp_num].name,dev[d].name);
	    }
	}
     irp[irp_num].adr=atoi(row[2]);
     irp[irp_num].strut=atoi(row[3]);
     irp[irp_num].stype=atoi(row[4]);
     irp[irp_num].tcp1=atoi(row[5]);
     irp[irp_num].tcp2=atoi(row[6]);
     irp[irp_num].tcp_p1=atof(row[7]);
     irp[irp_num].tcp_p2=atof(row[8]);
     irp[irp_num].imp1=atoi(row[9]);
     irp[irp_num].imp2=atoi(row[10]);
     irp[irp_num].pabs1=atof(row[11]);
     irp[irp_num].pabs2=atof(row[12]);
     irp[irp_num].tsour=atof(row[13]);
     irp[irp_num].patm=atof(row[14]);
     irp[irp_num].qimp=atoi(row[15]);
     irp[irp_num].dhour=atoi(row[16]);
     irp[irp_num].nhour=atoi(row[17]);
     irp_num++;
    } 
 if (res) mysql_free_result(res);
 if (debug>0) ULOGW ("[irp] total %d IRP add to list",irp_num);
}
//---------------------------------------------------------------------------------------------------
// RWCommands
/*int DeviceIRP::RWOperation (UINT type)
{
 switch (type)
    {
     case SV_NUMBER:
     case DEVICE_TYPE:
     case TIME_PRODUCT:
     case SERIAL_NUMBER:
     case SPEED:
     case ADR:
     case STRUT:
     case STYPE:
     case TCP1:
     case TCP2:
     case TCP_P1:
     case TCP_P2:
     case IMP1:
     case IMP2:
     case PABS1:
     case PABS2:
     case TSOUR:
     case PATM:
     case QIMP:
     case DHOUR:
     case NHOUR:
     case ERR:
     case CURRENT_TIME: break;
    }  
}*/
//---------------------------------------------------------------------------------------------------
// ReadLastArchive - read last archive and store to db
int DeviceIRP::ReadLastArchive (UINT gl)
{
 int 	rs;
 BYTE	data[8000];
 
 //float	t1,t2,v1,m1,q1,w1,w2; 
 this->qatt++;	// attempt

 rs = this->send_irp(READ_DATA,0x4c,0x14,(UCHAR*)"");
 usleep(300000);
 if (rs) rs = this->read_irp(data); 
 else
    {
     usleep(200000);
     rs = this->read_irp(data); 
    }
if (!rs)
    {
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>5) this->akt=0; // !!! 5 > normal quant from DB
     if (this->qerrors==6) Events (dbase,(3*0x10000000)|(TYPE_IRP*0x1000000)|SEND_PROBLEM,this->device);
    }
 if (rs>6)
    {
     UINT ahour=data[0]+data[1]*256;
     UINT nhour=data[2]+data[3]*256;
     UINT aday=data[4]+data[5]*256;  
     UINT nday=data[6]+data[7]*256;
     UINT amonth=data[8]+data[9]*256;  
     UINT nmonth=data[10]+data[11]*256;
     UINT atech=data[16]+data[17]*256;  
     UINT ntech=data[18]+data[19]*256;
     if (debug>2) ULOGW ("[irp %d] ahr=[0x%x] nhr=[%d] ady=[0x%x] ndy=[%d] ath=[0x%x] nth=[%d]",this->adr,ahour,nhour,aday,nday,atech,ntech);

     if (ahour<HOUR_ADR || ahour>DAY_ADR) { Events (dbase,(3*0x10000000)|(TYPE_IRP*0x1000000)|WRONG_MARKER,this->device,ahour); return 0; }
     if (aday<DAY_ADR || aday>MONTH_ADR) { Events (dbase,(3*0x10000000)|(TYPE_IRP*0x1000000)|WRONG_MARKER,this->device,aday); return 0; }
     if (atech<TECH_ADR || atech>0x7fff) { Events (dbase,(3*0x10000000)|(TYPE_IRP*0x1000000)|WRONG_MARKER,this->device,atech); return 0; }

/*     if (nhour)
     for (UINT df=1; df<=gl;df++)
     if ((ahour-0x11*(df*6))>HOUR_ADR)
        {                  
         rs = this->send_irp(READ_DATA,ahour-0x11*(df*6),0x11*6,(UCHAR*)"");
         if (rs) usleep(500000);
         if (rs) rs = this->read_irp(data);
         if (rs) ParseData (1,this->device,data,rs-10,11);
	}
     else
        {
         rs = this->send_irp(READ_DATA,HOUR_ADR,ahour-HOUR_ADR,(UCHAR*)"");
	 if (rs) usleep(200000);
         if (rs) rs = this->read_irp(data);
	 if (rs) ParseData (1,this->device,data,rs-10,11);
         if (rs) rs = this->send_irp(READ_DATA,DAY_ADR-(ahour-HOUR_ADR),0x11*(df*6)-(ahour-HOUR_ADR),(UCHAR*)"");
	 if (rs) usleep(200000);
         if (rs) rs = this->read_irp(data);
	 if (rs) ParseData (1,this->device,data,rs-10,11);	
	}*/
     if (nday)
     for (UINT df=1; df<=gl;df++)
     if ((aday-0x10*(df*3))>DAY_ADR)
        {
         rs = this->send_irp(READ_DATA,aday-0x10*(df*3),0x10*3,(UCHAR*)"");
	 usleep(400000);
         if (rs) rs = this->read_irp(data);
	 if (rs>20) ParseData (2,this->device,data,0x10*3,11);
	}
     else
        {
         //rs = this->send_irp(READ_DATA,DAY_ADR,aday-DAY_ADR,(UCHAR*)"");
	 //usleep(200000);
         //if (rs) rs = this->read_irp(data);
	 //if (rs) ParseData (2,this->device,data,0x10*3,11);
         //if (rs) rs = this->send_irp(READ_DATA,DAY_ADR-(aday-DAY_ADR),0x10*(df*3)-(aday-DAY_ADR),(UCHAR*)"");
	 //usleep(200000);
         //if (rs) rs = this->read_irp(data);
	 //if (rs) ParseData (2,this->device,data,0x10*3,11);
	}

     if (ntech)
     for (UINT df=1; df<=gl;df++)
     if ((atech-0xe*(df*4))>TECH_ADR)
        {
	 //if (debug>0) ULOGW ("[irp] ADR! %x %x %x",atech,atech-0xe*(df*4),0xe*4);
         rs = this->send_irp(READ_DATA,atech-0xe*(df*4),0xe*4,(UCHAR*)"");
	 usleep(400000); data[0]=0;
         if (rs) rs = this->read_irp(data);
	 if (rs>20) ParseData (2,this->device,data,rs-10,1);
	}
     else
        {
         //rs = this->send_irp(READ_DATA,DAY_ADR,atech-DAY_ADR,(UCHAR*)"");
	 //usleep(400000);
         //if (rs) rs = this->read_irp(data);
	 //if (rs) ParseData (2,this->device,data,0xe*4,1);
	 //if (debug>0) ULOGW ("[irp] ADR %x %x %x",atech,DAY_ADR-(atech-DAY_ADR),0xe*(4*df)-(atech-DAY_ADR));
         //if (rs) rs = this->send_irp(READ_DATA,DAY_ADR-(atech-DAY_ADR),0xe*(4*df)-(atech-DAY_ADR),(UCHAR*)"");
	 //usleep(200000);
         //if (rs) rs = this->read_irp(data);
	 //if (rs>50) ParseData (2,this->device,data,0xe*4,1);
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
// ReadDataCurrent - read single device. Readed data will be stored in DB
int DeviceIRP::ReadDataCurrent ()
{
 int 	rs,status=0;
 BYTE	data[100];
 CHAR	text[150];
 float	t1,t2,v1,m1,q1,w1,w2,dt,h1,h2,dh,v2,m2; 

 this->qatt++;	// attempt
 if (0)
    {
     hz=true;
     data[0]=0xc0; data[1]=0x0;
     rs = this->send_irp(WRITE_REGISTRY,0x46,0x1,(UCHAR *)data);
     data[0]=3; data[1]=0x0;
     rs = this->send_irp(WRITE_REGISTRY,0x4d,0x1,(UCHAR *)data);
     data[0]=4; data[1]=0x0;
     rs = this->send_irp(WRITE_REGISTRY,0x4e,0x1,(UCHAR *)data);
     data[0]=60; data[1]=0x0;
     rs = this->send_irp(WRITE_REGISTRY,0x4b,0x1,(UCHAR *)data);
     usleep(100000);
     if (rs) rs = this->read_irp(data); 
    }

 rs = this->send_irp(READ_DATA,0,0x2c,(UCHAR*)"");
 usleep(200000);
 if (rs) rs = this->read_irp(data); 
 else
    {
     // send problem
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>5) this->akt=0; // !!! 5 > normal quant from DB
     //if (this->qerrors==6) Events ((3*0x10000000)|(TYPE_IRP*0x1000000)|SEND_PROBLEM,this->device);
    }    
 if (rs)
    {
     this->akt=1;
     this->qerrors=0;
     this->conn=0;
     if (debug>3) ULOGW ("[irp] read2 [%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15]); 

     sprintf (this->lastdate,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,1+currenttime->tm_mon,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);
     // parse input
     t1=*(float*)(data);
     t2=*(float*)(data+4);
     dt=*(float*)(data+8);
     h1=*(float*)(data+12);
     h2=*(float*)(data+16);
     dh=*(float*)(data+20);
     v1=*(float*)(data+24);
     m1=*(float*)(data+28);
     v2=*(float*)(data+32);
     m2=*(float*)(data+36);
     q1=*(float*)(data+40);
     if (t1<=0 || t2<=0 || h1<=0 || h2<=0 || v1<0 || v2<0 || t1>120 || t2>120 || h1>1000 || h2>1000 || v1>100000 || v1<0 || m1>10000000 || m2<0 || q1>10000000 || q1<0) 
        {
	 status=0;
         if (t1<=0) status|=0x1;
         if (t2<=0) status|=0x2;
         if (h1<=0) status|=0x4;
         if (h2<=0) status|=0x8;
         if (v1<0) status|=0x10;
         if (v2<0) status|=0x20;
         if (t1>120) status|=0x40;
         if (t2>120) status|=0x80;
         if (h1>1000) status|=0x100;
         if (h2>1000) status|=0x200;
         if (v1>100000) status|=0x400;
         if (m1>100000) status|=0x800;
	 status+=0x1000;


	 Events (dbase,(3*0x10000000)|(TYPE_IRP*0x1000000)|status,this->device,t1);
	 sprintf (text,"t1=%.2f, t2=%.2f, dt=%.2f, h1=%.2f, h2=%.2f, dh=%.2f, vl=%.3f, m1=%.3f, v2=%.3f, m2=%.3f, q1=%.4f",t1,t2,dt,h1,h2,dh,v1,m1,v2,m2,q1);
         StoreData (this->device, data, text);
	 sprintf (query,"UPDATE prdata SET status=%d WHERE type=0 AND prm=13 AND device=%d",status,this->device);	 
	 if (debug>2) ULOGW ("%s",query);
	 res=dbase.sqlexec(query); 
	 if (res) mysql_free_result(res);
         return 0;
	}

     irpData [this->strut][0]=t1;
     irpData [this->strut][1]=t2;
     irpData [this->strut][2]=v1;
     irpData [this->strut][3]=h1;
     irpData [this->strut][4]=h2;
     irpData [this->strut][5]=q1;
/*
     irpData [this->strut][0]=this->strut;
     irpData [this->strut][1]=this->strut;
     irpData [this->strut][2]=this->strut;
     irpData [this->strut][3]=this->strut;
     irpData [this->strut][4]=this->strut;
     irpData [this->strut][5]=this->strut;
*/
//     ULOGW ("[irp %d] !!! [%f %f %f %f]",this->strut,irpData [this->strut][0],irpData [this->strut][1],irpData [this->strut][2],irpData [this->strut][3]);

     if (debug>3) ULOGW ("[irp %d] t1[0x%x 0x%x 0x%x 0x%x]t2[0x%x 0x%x 0x%x 0x%x]v1[0x%x 0x%x 0x%x 0x%x]m1[0x%x 0x%x 0x%x 0x%x]q1[0x%x 0x%x 0x%x 0x%x]",this->device,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19]);
     if (debug>0) ULOGW ("[irp %d] [t1=%.2f, t2=%.2f, dt=%.2f, h1=%.2f, h2=%.2f, dh=%.2f, vl=%.3f, m1=%.3f, v2=%.3f, m2=%.3f, q1=%.4f]",this->adr,t1,t2,dt,h1,h2,dh,v1,m1,v2,m2,q1);
     sprintf (text,"t1=%.2f, t2=%.2f, dt=%.2f, h1=%.2f, h2=%.2f, dh=%.2f, vl=%.3f, m1=%.3f, v2=%.3f, m2=%.3f, q1=%.4f",t1,t2,dt,h1,h2,dh,v1,m1,v2,m2,q1);
     // select current records, if not present - create, if present - update  

     //StoreData (dbase, this->device, data, text);
     StoreData (dbase, this->device, 4, 0, 0, t1, 0);
     StoreData (dbase, this->device, 4, 1, 0, t2, 0);
     StoreData (dbase, this->device, 1, 0, 0, h1, 0);
     StoreData (dbase, this->device, 1, 1, 0, h2, 0);

     StoreData (dbase, this->device, 11, 0, 0, v1, 0);
     StoreData (dbase, this->device, 11, 1, 0, v2, 0);

     StoreData (dbase, this->device, 12, 0, 0, m1, 0);
     StoreData (dbase, this->device, 12, 1, 0, m2, 0);
     StoreData (dbase, this->device, 13, 0, 0, q1, 0);
    }
 else
    {
     this->qerrors++;
     this->conn=0;
     //if (this->qerrors==6) Events ((3*0x10000000)|(TYPE_IRP*0x1000000)|RECIEVE_PROBLEM,this->device);
    }

 if (rs) rs=1; else rs=0;
 sprintf (query,"UPDATE device SET qatt=qatt+1,qerrors=qerrors+%d,conn=1,lastdate=NULL WHERE type=7",!rs);
 res=dbase.sqlexec(query);  if (res) mysql_free_result(res); 
 sprintf (query,"UPDATE device SET qatt=qatt+1,qerrors=qerrors+%d,conn=%d,lastdate=NULL,devtim=NULL WHERE id=%d",!rs,rs,this->iddev);
 res=dbase.sqlexec(query);  if (res) mysql_free_result(res);
 return 0;
}
//---------------------------------------------------------------------------------------------------
int DeviceIRP::read_irp (BYTE* dat)
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

 ioctl (fd,FIONREAD,&nbytes); 
 //if (debug>2) ULOGW ("[irp] read (%d)",nbytes); 
 if (nbytes>0) nbytes=read (fd, &data, 220); 
 usleep (50000);
 ioctl (fd,FIONREAD,&bytes);  
 //if (debug>2) ULOGW ("[irp] read2 (%d)",bytes); 
 if (bytes>0) bytes=read (fd, &data+nbytes, 100);
 nbytes+=bytes;

 if (data[0]!=0xff)
 while (i<nbytes)
    {
     if (data[i]==0xFF && data[i+1]==0xFF && data[i+2]==0xFF)
        {
	 memcpy (data,data+i,nbytes-i);
	 break;
	}
     i++;
    }
 i=0;
 if (debug>3) ULOGW ("[irp %d] read (%d) [%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x]",this->device,nbytes,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15]); 
 
 if (nbytes>0)
    {
     while (i<nbytes)
        {
	 ok=0xFF;
	 //if (debug>2) ULOGW ("[lk] data[%d]=(0x%x)",i,data[i]); 
	 if (i<7)
	 switch (i)
	    {
		case 0:	if (data[i]==0xFF) ok=1; // (* its preambule *)
			break;
		case 1:	if (data[i]==0xff) ok=0; else ok=0; 	// (* !!compare adress here *)
			break;
		case 2: if (data[i]==0xff) ok=0; else ok=0; 	// (* !!compare adress here *)
			break;
		case 3: if (data[i]==(this->adr&0xFF)) ok=0; else ok=3; 	// (* low address not compare*)
			break;
		case 4:	if (data[i]==(this->adr/0x100)) ok=0; else ok=3;	// (* high address not compare *)
			break;
		case 5:	break;
		case 6: bytes=data[i]; break;
	    }
	 else
	    {
	     if (i==bytes+7)
	        {
    	         crc=Crc8(data+1,bytes+6);
		 if (debug>2) ULOGW ("[irp %d] rd(%d) [%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x][%x] crc=%x dt[%d]=0x%x",this->adr,nbytes,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],crc,i,data[i]);
	         //if (crc==data[i] || bytes==0x4c || bytes>100) ok=0; else ok=6; // (* compare cs *)
	         if (crc==data[i] || bytes==0x4c) ok=0; else ok=6; // (* compare cs *)
		 break;
		}
	    }
	i++;
	}
    if (ok>1)
        {
         switch (ok)
            {
	     case 0x1:	// (* no preambule *)
	     case 0x6: 	Events (dbase,(3*0x10000000)|(TYPE_IRP*0x1000000)|CRC_ERROR,this->device,crc);  // (* crc data error *)
    	     break;
	    }
	}
    }
 if (debug>3) ULOGW ("[irp] memcpy (%x,%x,%d)",dat,data+7,data_len);
 if (data_len<200) memcpy (dat,data+0x7,data_len);
 if (ok==0) return nbytes;
 else return 0;
}
//---------------------------------------------------------------------------------------------------
bool DeviceIRP::send_irp (UINT op, UINT reg, UINT nreg, UCHAR* dat)
{
 if (debug>3) ULOGW ("[irp] send_irp (%d, %d, %d)",op,reg,nreg);
 //this->adr=1;
 
 UINT	crc=0;		//(* CRC checksum *)
 UINT	nbytes = 0; 	//(* number of bytes in send packet *)
 BYTE	data[100];	//(* send sequence *)

 data[0]=0xFF;
 data[2]=this->adr/256;
 data[1]=this->adr&0xFF;
 data[3]=0xff;		// we always have zero address	
 data[4]=0xff;
 data[5]=(CHAR)op;

 switch (op)
     {
	case 0x5:	data_op=5; nbytes=3; break;	//(* READ_REGISTRY *)
	case 0x6:	data_op=6; nbytes=3; break; 	//(* READ_DATA *)
	case 0x7:	data_op=7; nbytes=nreg+2; break;//(* WRITE_REGISTRY *)
    }
 data[6]=(CHAR)nbytes;
 //write (fd,(VOID *)nbytes,1); 
 //if (debug>2) ULOGW ("[irp] write (0x%x)(nbytes)",nbytes);
 switch (op)
    {
        case 0x5: data[8]=reg/256;	// 
		  data[7]=reg&0xFF;	// 
		  data[9]=(UCHAR)nreg;	//
		  data[10] = Crc8 (data+1, 9);
		  write (fd,&data,11);
		  if (debug>2) ULOGW ("[irp %d] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",this->adr,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9]);
		  data_adr=reg;
		  data_len=nreg;
		  break;
	case 0x6: data[8]=reg/256;	// 
		  data[7]=reg&0xFF;	// 
		  data[9]=(UCHAR)nreg;	//
		  data[10] = Crc8 (data+1, 9);
		  write (fd, &data,11);
		  if (debug>2) ULOGW ("[irp %d] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",this->adr,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10]);
		  data_adr=reg;
		  data_len=nreg;
		  break;
	case 0x7: data[8]=reg/256;	// 
		  data[7]=reg&0xFF;	//
		  memcpy (data+9,dat,nreg);
		  data[nreg+9] = Crc8 (data+1, nreg+8);
		  write (fd,&data,nreg+10);
		  if (debug>2) ULOGW ("[irp %d] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",this->adr,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10]);
		  break;
    }
//if (debug>2) ULOGW ("[irp] write (0x%x) (crc)",crc);
//write (fd,(VOID *)crc,1);
return true;
}
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 blok=2;
 sprintf (devp,"/dev/ttyS%d",blok);
 //sprintf (devp,"/dev/ttyUSB0",blok);
 //sprintf (devp,"/dev/ttyM1",blok);
 speed=9600;
 if (debug>0) ULOGW("[irp] attempt open com-port %s on speed %d",devp,speed);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[irp] error open com-port %s [%s]",devp,strerror(errno)); 
     Events (dbase,(3*0x10000000)|(TYPE_IRP*0x1000000)|CRC_ERROR,0);
     return false;
    }
 else if (debug>1) ULOGW("[irp] open com-port success"); 

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
 tio.c_iflag = IGNPAR|IGNBRK;

 tio.c_iflag &= ~(INLCR | IGNCR | ICRNL);
 tio.c_iflag &= ~(IXON | IXOFF | IXANY);

 if (debug>1) ULOGW("[irp] c[0x%x] l[0x%x] i[0x%x] o[0x%x]",tio.c_cflag,tio.c_lflag,tio.c_iflag,tio.c_oflag);  
 
// tio.c_cc[VMIN] = 0;
// tio.c_cc[VTIME] = 1; //Time out in 10e-1 sec
 
 cfsetispeed(&tio, baudrate(speed));
 //fcntl(fd, F_SETFL, FNDELAY);
 fcntl(fd, F_SETFL, 0);
 tcsetattr(fd, TCSANOW, &tio);
 tcsetattr (fd,TCSAFLUSH,&tio);

 return true;
}

//---------------------------------------------------------------------------------------------------
BOOL StoreData (UINT dv, BYTE* data, CHAR* text)
{
 sprintf (query,"INSERT INTO answers(device,hex,text) VALUES('%d','%s','%s')",dv,data,text);
// for (int tt=0;tt<100;tt++) if (data[tt]==39) data[tt]=
 if (debug>3) ULOGW ("%s [%d]",query,row);
 res=dbase.sqlexec(query); 
 if (res) mysql_free_result(res);
 return true;
}
//---------------------------------------------------------------------------------------------------
BOOL ParseData (UINT type, UINT dv, BYTE* data, UINT len, UINT prm)
{
 UINT 	adr=0;
 FLOAT	q, v, m;
 CHAR date[30];

 if (debug>3) ULOGW ("[irp] ParseData (%d, %d, %d, %d)",type, dv, len, prm);
 if (len<18 || len>500) return false;
 if (dv<0 || dv>0xc0c0000) 
    {
      LoadIRPConfig();  
      return false;
    }
 
 if (type==1 && prm>10)
    {
     while (adr<len-17)
        {
         // parse input
         q=*(float*)(data+adr+5);
         v=*(float*)(data+adr+9);
         m=*(float*)(data+adr+13);
//	 if (data[adr]<25)
	 if (q>=0 && v>=0 && m>=0 && q<1000000 && v<1000000 && m<1000000)
	    {
	     if (debug>2) ULOGW ("[irp] [A1][%d] h [0x%x 0x%x 0x%x 0x%x 0x%x] Q [0x%x 0x%x 0x%x 0x%x] [%f]",adr,data[adr],data[adr+1],data[adr+2],data[adr+3],data[adr+4],data[adr+5],data[adr+6],data[adr+7],data[adr+8],q);
             if (debug>2) ULOGW ("[irp] [A1][%d] V [0x%x 0x%x 0x%x 0x%x] [%.3f] M [0x%x 0x%x 0x%x 0x%x] [%.3f]",adr+13,data[adr+9],data[adr+10],data[adr+11],data[adr+12],v,data[adr+13],data[adr+14],data[adr+15],data[adr+16],m);

	     sprintf (date,"%04d%02d%02d%02d0000",data[adr+3]+2000,data[adr+2],data[adr+1],data[adr]);

    	     StoreData (dbase, dv, 13, 0, type, data[adr+4], q, date, 0);
	     StoreData (dbase, dv, 11, 0, type, data[adr+4], v, date, 0);
	     StoreData (dbase, dv, 12, 0, type, data[adr+4], m, date, 0);
	    }
         adr+=0x11;
	}          
    }

 if (type==2 && prm==1)
    {
     while (adr<len)
        {
         // parse input
         data[adr]=data[adr+2];
         data[adr+1]=data[adr+3];
         data[adr+2]=data[adr+4];
         //data[adr+3]=data[adr+4];

         q=*(float*)(data+adr+6);
         v=*(float*)(data+adr+10);

	 if (q>=0 && v>=0 && q<1000000 && v<1000000)
	 if (data[adr+2]==currenttime->tm_year-100)
	    {
	     if (debug>3) ULOGW ("[irp] [0x%x 0x%x 0x%x 0x%x 0x%x] H1 [0x%x 0x%x 0x%x 0x%x] [%.3f] H2 [0x%x 0x%x 0x%x 0x%x] [%.3f]",data[adr],data[adr+1],data[adr+2],data[adr+3],data[adr+4],data[adr+6],data[adr+7],data[adr+8],data[adr+9],q,data[adr+10],data[adr+11],data[adr+12],data[adr+13],v);
	     sprintf (date,"%04d%02d%02d000000",data[adr+2]+2000,data[adr+1],data[adr+0]);
    	     StoreData (dbase, dv, 1, 0, type, data[adr+5], q, date, 0);
	     StoreData (dbase, dv, 1, 1, type, data[adr+5], v, date, 0);
	    }
         adr+=0xe;
	}
    }

 if (type==2 && prm>10)
    {
     while (adr<len)
        {
         // parse input
         q=*(float*)(data+adr+4);
         v=*(float*)(data+adr+8);
         m=*(float*)(data+adr+12);

         //if (dv==33882147) ULOGW ("[irp] [-A2] [0x%x 0x%x 0x%x 0x%x] Q [0x%x 0x%x 0x%x 0x%x] [%.3f] V [0x%x 0x%x 0x%x 0x%x] [%.3f] M [0x%x 0x%x 0x%x 0x%x] [%.3f]",data[adr],data[adr+1],data[adr+2],data[adr+3],data[adr+4],data[adr+5],data[adr+6],data[adr+7],q,data[adr+8],data[adr+9],data[adr+10],data[adr+11],v,data[adr+12],data[adr+13],data[adr+14],data[adr+15],m);

	 if (data[adr+2]==currenttime->tm_year-100)
	 if (q>=0 && v>=0 && m>=0 && q<1000000 && v<1000000 && m<1000000)
	    {
	     if (debug>3) ULOGW ("[irp] [A2] [0x%x 0x%x 0x%x 0x%x] Q [0x%x 0x%x 0x%x 0x%x] [%.3f] V [0x%x 0x%x 0x%x 0x%x] [%.3f] M [0x%x 0x%x 0x%x 0x%x] [%.3f]",data[adr],data[adr+1],data[adr+2],data[adr+3],data[adr+4],data[adr+5],data[adr+6],data[adr+7],q,data[adr+8],data[adr+9],data[adr+10],data[adr+11],v,data[adr+12],data[adr+13],data[adr+14],data[adr+15],m);
	     if (type==1) sprintf (date,"%04d%02d%02d%02d0000",data[adr+2]+2000,data[adr+1],data[adr+0],data[adr-1]);
	     if (type==2) sprintf (date,"%04d%02d%02d000000",data[adr+2]+2000,data[adr+1],data[adr+0]);

	     StoreData (dbase, dv, 13, 0, type, data[adr+3], q, date, 0);
	     StoreData (dbase, dv, 11, 0, type, data[adr+3], v, date, 0);
	     StoreData (dbase, dv, 12, 0, type, data[adr+3], m, date, 0);
	    }
         adr+=0x10;
	}
    }    
 if (debug>3) ULOGW ("[irp] ParseData end");
 return true;
}
