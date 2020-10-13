//-----------------------------------------------------------------------------
#define HART_LOG	"hart.log"
//-----------------------------------------------------------------------------
#include "types.h"
#include "main.h"
#include "hart.h"
//-----------------------------------------------------------------------------
static INT	fd;
static termios 	tio;
static INT  	tag_num=0;
UCHAR	DId[80];
//-----------------------------------------------------------------------------
struct DeviceData 
	{
	 CHAR	manufacturerId;	// manufacturer ID
	 UCHAR	devicetypeId;	// device type ID
	 UCHAR	preambleNo;	// number of preambles
	 UCHAR	uncomrevNo;	// universal command rev.No
	 UCHAR	dccrevNo;	// device-specific command rev.No
	 UCHAR	softRev;	// software revision
	 UCHAR	hardRev;	// hardware revision
	 UCHAR	deviceId[3];	// device identification
	 UCHAR	hUnitCodePV;	// HART unit code of PV
	 UCHAR	PV[4];		// primary process variable
	 UCHAR	hUnitCodeSV;	// HART unit code of SV
	 UCHAR	SV[4];		// secondary process variable
	 UCHAR	hUnitCodeTV;	// HART unit code of TV
	 UCHAR	TV[4];		// third process variable
	 UCHAR	hUnitCodeFV;	// HART unit code of FV
	 UCHAR	FV[4];		// fourth process variable
	 UCHAR	hAC[4];		// actual current of PV
	 UCHAR	hPrecent[4];	// percentage of measuring range
	 UCHAR	message[25];	// user message
	};
struct	DeviceData DData;
//-----------------------------------------------------------------------------
INT  	devBus=0;
UINT 	startid=0, ok=0, nok=0;
BOOL 	DTRHIGH = FALSE;		// DTR on Write Low or High
DWORD 	dwBytesWritten=0,dwBytesRead=0;
SHORT 	allsuccess=0;
INT 	status,options,result;
CHAR	deviceName[50];
DOUBLE  PV[4];
//-----------------------------------------------------------------------------
VOID 	ULOGW (const CHAR* string, ...);
DOUBLE 	cIEEE754toFloat (UCHAR *Data);
UINT 	hPollDevice(INT device, INT devn, SHORT blok);
BOOL 	hOpenCom (SHORT blok, UINT dev);
UINT 	baudrate (UINT baud);

extern	UINT	debug;
extern 	"C" DeviceR 	dev[MAX_DEVICE];	// device class
extern 	"C" DeviceHART 	hart[MAX_DEVICE_HART];	// HART class
//-----------------------------------------------------------------------------
void * hDeviceThread (void * devr)
{
 INT rt=0;
 int devdk=*((int*) devr);			// DK identificator
 dbase.sqlconn("dk","root","");			// connect to database
 LoadHartConfig();
 // open port for work
 if (!hOpenCom (hart[0].port,hart[0].speed)) return -1;
 for (UINT r=0;r<hart_num;r++)
    {
     hScanBus (hart[r].adr, hart[r].port, hart[r].speed);
    }

 while (WorkRegim)
 for (UINT r=0;r<hart_num;r++)
    {
     if (debug>1) ULOGW ("[hart] hart[%d/%d].ReadHart ()",r,hart_num);
     //irp[r].ReadLastArchive (15);
     hPollDevice(INT devc, INT devn, SHORT blok);
    }
 dbase.sqldisconn();
 if (debug>0) ULOGW ("[hart] hart thread end");
 hart_thread=FALSE;
 pthread_exit (NULL);
}
//-----------------------------------------------------------------------------
void * hReadDevice (void * devr)
{
 int dev=*((int*) devr);
// ULOGW("hScanBus [%d][%d|%d|%d]",dev,device[dev].device, device[dev].com, device[dev].speed);
 INT rt=hScanBus (device[dev].device, device[dev].com, device[dev].speed);
 if (rt<0) 
    {
     if (debug>0) ULOGW("[hart] error scan hart bus [%d]",rt);
     //return NULL;
    }
 while (1)
    {
     hPollDevice(dev, device [dev].device, device [dev].com);
     // store channal to channal
    }
 close (fd);
}
//-----------------------------------------------------------------------------
INT hScanBus (SHORT adr, UINT com, UINT speed)
{
//Events (TYPE_INFORMATION + MODULE_HART + EVENTS_SCANINIT);
devBus=0;
UCHAR  Out[] = {0xff,0xff,0xff,0xff,0xff,0x2,0x80,0x0,0x0,0x82,0x0,0x0};
UCHAR sBuf1[40]; UINT ii;
for (INT nump=0;nump<=4;nump++)
    {
     for (ii=0;ii<30;ii++) sBuf1[ii]=0;
     Out[6]=0x80+adr; Out[7]=0; Out[9]=Out[5]^Out[6]^Out[7]^Out[8]; Out[10]=0;
     if (debug>2) ULOGW("[hart] out=[0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]",(UCHAR)Out[0],(UCHAR)Out[1],(UCHAR)Out[2],(UCHAR)Out[3],(UCHAR)Out[4],(UCHAR)Out[5],(UCHAR)Out[6],(UCHAR)Out[7],(UCHAR)Out[8],(UCHAR)Out[9]);
     tcsetattr (fd,TCSAFLUSH,&tio);
     ioctl (fd,TIOCMGET,&status);
     status |= TIOCM_RTS;
     status &= ~TIOCM_DTR;
     ioctl (fd,TIOCMSET,&status);
     result=write (fd,&Out,10);
     usleep(300000);
     ioctl (fd,TIOCMGET,&status);
     status &= ~TIOCM_RTS;
     status |= TIOCM_DTR;
     ioctl (fd,TIOCMSET,&status);
     usleep(300000);
     INT bytes=read (fd, &sBuf1, 38);

     if (debug>2) ULOGW ("[hart] b.read=%d [wr=%d]",bytes,result);
     if (debug>2) for (ii=0;ii<=bytes;ii++) ULOGW("[hart] [%d] = 0x%x",ii,sBuf1[ii]);
     BOOL bcFF_OK = FALSE;	BOOL bcFF_06 = FALSE; UINT chff=0;
     for (UINT cnt=0;cnt<36;cnt++)
	{
	 if (sBuf1[cnt]==0xff) {chff++; if (chff>3) bcFF_OK = TRUE;} else chff=0;
    	    if (bcFF_OK)
		if (sBuf1[cnt]==0x6)
	    	    if (sBuf1[cnt+1]==(0x80+adr))
			if (sBuf1[cnt+2]==0)
			    { bcFF_06 = TRUE; startid=cnt+4;}
	}
     if (bcFF_06)
	{
	 DId[adr*5] = 0x80+ sBuf1[startid+3];
	 DId[adr*5+1] = sBuf1[startid+4];
	 DId[adr*5+2] = sBuf1[startid+11];
	 DId[adr*5+3] = sBuf1[startid+12];
	 DId[adr*5+4] = sBuf1[startid+13];
	 DData.manufacturerId = sBuf1[startid+3];
	 DData.devicetypeId = sBuf1[startid+4];
	 DData.preambleNo = sBuf1[startid+5];
	 DData.uncomrevNo = sBuf1[startid+6];
	 DData.dccrevNo = sBuf1[startid+7];
	 DData.softRev = sBuf1[startid+8];
	 DData.hardRev = sBuf1[startid+9];
	 DData.deviceId[0] = sBuf1[startid+11];
	 DData.deviceId[1] = sBuf1[startid+12];
	 DData.deviceId[2] = sBuf1[startid+13];
	 switch (sBuf1[startid+4])
    	    {
	     case 4:  strcpy (deviceName,"EJA110A"); break;
	     case 81: strcpy (deviceName,"EJA430A"); break;
    	     case 14: strcpy (deviceName,"Cerabar M"); break;
	     case 80: strcpy (deviceName,"Promass80"); break;
	     case 83: strcpy (deviceName,"Promass40"); break;
	     case 200: strcpy (deviceName,"TMT182"); break;
	     case 201: strcpy (deviceName,"TMT122"); break;
	     case 202: strcpy (deviceName,"TMT162"); break;
	     default:  strcpy (deviceName,"Udentifined");
	    }
	 if (debug>0) ULOGW("[hart] device %s found on address %d",deviceName,adr);
	 return 1;
	}
    }
 return 0;
}
//---------------------------------------------------------------------------------------------------
UINT tcount=0;
UINT mBuf[100];
//---------------------------------------------------------------------------------------------------
UINT hPollDevice(INT devc, INT devn, SHORT blok)
{
 UINT c0m=0,chff=0,num_bytes=0,startid=0;
 UCHAR  Out[] = {0xff,0xff,0xff,0xff,0xff,0x82,0x0,0x0,0x0,0x0,0x0,0x3,0x0,0x81,0x0};
 UCHAR sBuf1[100]; UCHAR Data[30]; UINT i,cnt; INT bytes=0;
 BOOL bcFF_OK = FALSE;
 BOOL bcFF_06 = FALSE;
 //Out[6]=0x80+devn; Out[7]=0; Out[9]=Out[5]^Out[6]^Out[7]^Out[8]; Out[10]=0;
 Out[5]=0x82; Out[6]=DId[0]; Out[7]=DId[1];  Out[8]=DId[2]; Out[9]=DId[3];
 Out[10]=DId[4]; Out[11]=0x3;  Out[12]=0x0;   Out[13]=Out[5]^Out[6]^Out[7]^Out[8]^Out[9]^Out[10]^Out[11]^Out[12];  Out[14]=0;
 if (debug>2) ULOGW("[hart] out=[0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]",Out[5],Out[6],Out[7],Out[8],Out[9],Out[10],Out[11],Out[12],Out[13]);

 tcsetattr (fd,TCSAFLUSH,&tio);
 ioctl (fd,TIOCMGET,&status);
 status |= TIOCM_RTS;
 status &= ~TIOCM_DTR;
 ioctl (fd,TIOCMSET,&status);
 result=write (fd,&Out,14);
 usleep(300000);
 ioctl (fd,TIOCMGET,&status);
 status &= ~TIOCM_RTS;
 status |= TIOCM_DTR;
 ioctl (fd,TIOCMSET,&status);
 usleep(400000);
 ioctl (fd,FIONREAD,&bytes);
 bytes=read (fd, &sBuf1, 58);
 if (debug>2) ULOGW ("[hart] b.read = %d[%d]",bytes,result);
 if (debug>2) for (i=0;i<bytes;i++) ULOGW("[hart] [%d] = 0x%x",i,sBuf1[i]);
 bcFF_OK = FALSE;
 bcFF_06 = FALSE; chff=0;
 if (bytes)
 for (cnt=0;cnt<26;cnt++)
    {
     if (sBuf1[cnt]==0xff) {chff++; if (chff>3) bcFF_OK = TRUE;} else chff=0;
	 if (bcFF_OK)
	     if (sBuf1[cnt]==0x86)	
		 if (sBuf1[cnt+1]==DId[0])
		    if (sBuf1[cnt+2]==DId[1])
		    	{ bcFF_06 = TRUE; num_bytes = sBuf1[cnt+7]; startid=cnt+8; break;}
    }
 if (bcFF_06)
    {
     UCHAR ks=0; CHAR buffer[12];
     for (i=startid-8;i<startid+num_bytes;i++) ks=ks^sBuf1[i];
     if (ks!=sBuf1[i])	{ bcFF_06=FALSE; if (debug>1) ULOGW ("[hart] ks wrong 0x%x | 0x%x",ks,sBuf1[i]); }
     for (cnt=0;cnt<30;cnt++) Data[cnt]=0;
     for (cnt=0;cnt<num_bytes;cnt++) Data[cnt]=sBuf1[startid+cnt];
     DData.hAC[0] = Data[2];
     DData.hAC[1] = Data[3];
     DData.hAC[2] = Data[4];
     DData.hAC[3] = Data[5];
     DData.hUnitCodePV = Data[6];
     DData.PV[0] = Data[7];
     DData.PV[1] = Data[8];
     DData.PV[2] = Data[9];
     DData.PV[3] = Data[10];
     DData.hUnitCodeSV = Data[11];
     DData.SV[0] = Data[12];
     DData.SV[1] = Data[13];
     DData.SV[2] = Data[14];
     DData.SV[3] = Data[15];
     DData.hUnitCodeTV = Data[16];
     DData.TV[0] = Data[17];
     DData.TV[1] = Data[18];
     DData.TV[2] = Data[19];
     DData.TV[3] = Data[20];
     DData.hUnitCodeFV = Data[21];
     DData.FV[0] = Data[22];
     DData.FV[1] = Data[23];
     DData.FV[2] = Data[24];
     DData.FV[3] = Data[25];
     PV[0]=cIEEE754toFloat(DData.PV);
     PV[1]=cIEEE754toFloat(DData.SV);
     PV[2]=cIEEE754toFloat(DData.TV);
     PV[3]=cIEEE754toFloat(DData.FV);
     if(debug>2) ULOGW ("[hart] PV=[0x%x 0x%x 0x%x 0x%x]",DData.PV[0],DData.PV[1],DData.PV[2],DData.PV[3]);
     if(debug>1) ULOGW ("[hart] PV=[%f] SV=[%f] TV=[%f] FV=[%f]",PV[0],PV[1],PV[2],PV[3]);
     //PV[0]=10+20*((DOUBLE)(random())/RAND_MAX);
     //PV[1]=10+20*((DOUBLE)(random())/RAND_MAX);
     //PV[2]=10+20*((DOUBLE)(random())/RAND_MAX);
     //PV[3]=10+20*((DOUBLE)(random())/RAND_MAX);
     ks=0;
    }
 int ks=0;
 for (cnt=0;cnt<MAX_CHANNELS;cnt++)
    {
     if (chan[cnt].device==device[devc].id)
        {
         if (ks>4) 
            {
	     if (debug>1) ULOGW ("[hart] error, channals number of this device is too big");
	     break;
	    }
	 chd[cnt].current=PV[ks];
	 //
	 chd[cnt].minuts=chd[cnt].minuts+PV[ks];
	 chd[cnt].nminuts++;
	 chd[cnt].hours=chd[cnt].hours+PV[ks];
	 chd[cnt].nhours++;
	 chd[cnt].days=chd[cnt].days+PV[ks];
	 chd[cnt].ndays++;
	 if (bcFF_06) chd[cnt].sts_cur=0;
	 else chd[cnt].sts_cur=4;	// no answer
	 //chd[cnt].month=chd[cnt].month+PV[ks];
	 //chd[cnt].nmonth++;
	 ks++;
	}
     if (chan[cnt].target==0) break;
    }
 return 0;
}
//---------------------------------------------------------------------------------------------------
DOUBLE cIEEE754toFloat (UCHAR *Data)
{
BOOL sign;
DOUBLE res=0,zn=0.5, tmp;
UCHAR mask;
if (*(Data+0)&0x80) sign=TRUE; else sign=FALSE;
CHAR exp = ((*(Data+0)&0x7f)*2+(*(Data+1)&0x80)/0x80)-127;
for (int j=1;j<=3;j++)
	{
	 mask = 0x80;
	 for (INT i=0;i<=7;i++)
		{
		 if (j==1&&i==0) {res = res+1; mask = mask/2;}
		 else {
		 res = (*(Data+j)&mask)*zn/mask + res;
		 mask = mask/2; zn=zn/2; }
		}
	}
res = res * pow (2,exp);
tmp = 1*pow (10,-15);
if (res<tmp) res=0;
if (sign) res = -res;
return res;
}
//---------------------------------------------------------------------------
BOOL hOpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 if (blok<10)  sprintf (devp,"/dev/ttyS%d",blok);
 if (blok>9)  sprintf (devp,"/dev/ttyr0%d",blok-10);
 if (debug>0) ULOGW("[hart] attempt open com-port %s",devp);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[hart] error open com-port %s [%s]",devp,strerror(errno)); 
     return FALSE;
    }
 else if (debug>1) ULOGW("[hart] open com-port success"); 
 
 tcflush(fd, TCIOFLUSH); //Clear send & recive buffers
 tcgetattr(fd, &tio);
 cfsetospeed(&tio, B1200);
 tio.c_cflag |= CREAD|CLOCAL|PARENB|PARODD;
 tio.c_cflag &= ~CSIZE;
 tio.c_cflag &= ~CSTOPB;
 tio.c_cflag |=CS8;
 tio.c_cflag &= ~CRTSCTS;
// tio.c_cflag |=CS8;
 //options.c_cflag&=~(PARENB|CSTOPB|CRTSCTS);
 tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
 tio.c_iflag &= ~(INLCR | IGNCR | ICRNL);
 tio.c_iflag &= ~(IXON | IXOFF | IXANY);
// tio.c_cc[VMIN] = 0;
// tio.c_cc[VTIME] = 1; //Time out in 10e-1 sec
 cfsetispeed(&tio, B1200);
 fcntl(fd, F_SETFL, /*FNDELAY*/0);
 tcsetattr(fd, TCSANOW, &tio);
 return TRUE;
}
//--------------------------------------------------------------------------------------
// load all HART configuration from DB
BOOL LoadHartConfig()
{
 sprintf (query,"SELECT * FROM dev_hart");
 res=dbase.sqlexec(query);
 hart_num=0;
 UINT nr=mysql_num_rows(res);
 for (UINT r=0;r<nr;r++)
    {
     row=mysql_fetch_row(res);
     hart[hart_num].idhart=atoi(row[0]);
     hart[hart_num].device=atoi(row[1]);     
     for (UINT d=0;d<device_num;d++)
        {
         if (dev[d].idd==hart[hart_num].device)
            {
    	     hart[hart_num].iddev=dev[d].id;
	     hart[hart_num].SV=dev[d].SV;
    	     hart[hart_num].interface=dev[d].interface;
	     hart[hart_num].protocol=dev[d].protocol;
	     hart[hart_num].port=dev[d].port;
	     hart[hart_num].speed=dev[d].speed;
	     hart[hart_num].adr=dev[d].adr;
	     hart[hart_num].type=dev[d].type;
	     strcpy(hart[hart_num].number,dev[d].number);
	     hart[hart_num].flat=dev[d].flat;
	     hart[hart_num].akt=dev[d].akt;
	     strcpy(hart[hart_num].lastdate,dev[d].lastdate);
	     hart[hart_num].qatt=dev[d].qatt;
	     hart[hart_num].qerrors=dev[d].qerrors;
	     hart[hart_num].conn=dev[d].conn;
	     strcpy(hart[hart_num].devtim,dev[d].devtim);
	     hart[hart_num].chng=dev[d].chng;
	     hart[hart_num].req=dev[d].req;
	     hart[hart_num].source=dev[d].source;
	     strcpy(hart[hart_num].name,dev[d].name);
	    }
	}
     hart_num++;
    } 
 if (res) mysql_free_result(res);
 if (debug>0) ULOGW ("[hart] total %d add to list",hart_num);
}
