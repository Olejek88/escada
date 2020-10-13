//--------------------------------------------------------------------
#include "types.h"
#include "main.h"
//--------------------------------------------------------------------
static INT	fd;
static termios 	tio;
static INT	tag_num=0;
//--------------------------------------------------------------------
BOOL 	aOpenCom (SHORT blok, UINT dev);
UINT 	aPollDevice(INT device, INT devn, SHORT blok);
INT  	aScanBus (SHORT adr, UINT dev, UINT speed);
DOUBLE 	aReadData (CHAR* com);
VOID 	ULOGW (CHAR* string, ...);
UINT 	baudrate (UINT baud);
//--------------------------------------------------------------------
extern  "C" DeviceR device[20];
//extern	"C" chData  chd[200];
//extern	"C" Channels  chan[200];
//--------------------------------------------------------------------
DOUBLE aReadData (CHAR* com)
{
 CHAR Out[15]; CHAR* dest;
 CHAR sBuf1[100];
 DOUBLE res=0;
 INT	bytes=0,dwbr1=0;
 strcpy (Out,com);
 ULOGW ("[adam] comm out=%s",Out);
 tcflush(fd,TCIFLUSH);
 write (fd,&Out,5);
 sleep(1);
 ioctl (fd,FIONREAD,&bytes);
 if (bytes) read (fd, &sBuf1, 55);
 ULOGW ("[adam] bytes read = %d",bytes);
 sprintf (Out,">"); // >+0.11
 if ((dest=strstr(sBuf1, Out))!=NULL)
    {
     strcpy (Out,dest+1); 
     for (UINT o=1;o<10;o++) if (Out[o]==0xd) { Out[o]=0; break; }
     res=atof(Out);
     if (debug>2) ULOGW ("[adam] answer correct (%f)",res);
    }
 return res;
}
//-----------------------------------------------------------------------------
VOID * aReadDevice (VOID * devr)
{
 INT dev=*((int*) devr);
 DOUBLE	res=0;
 CHAR 	Out[15];
 if (debug>0) ULOGW("[adam] aScanBus (%d)",device[dev].device);
 INT rt=aScanBus (device[dev].device, device[dev].com, device[dev].speed);
 if (rt<=0)
    {
     if (debug>0) ULOGW("[adam] error found adam device [%d]",rt);
     //return NULL;
    }
 while (1)
    {
      for (UINT cnt=0;cnt<MAX_CHANNELS;cnt++)
	{
	 ULOGW ("[adam] <1>[%d][%d]",chan[cnt].device,device[dev].id);
         if (chan[cnt].device==device[dev].id)
    	    {
	     int chh=chan[cnt].target%(64*chan[cnt].device);
	     sprintf (Out,"#%02d%d\r",device[dev].device,chh);
	     ULOGW ("[adam] aReadData()");
	     res=aReadData (Out);
    	     chd[cnt].current=res;
    	     //if (debug>2) ULOGW ("[adam] <h>chd[%d].current=%f",cnt,chd[cnt].current);
    	     chd[cnt].minuts=chd[cnt].minuts+res;
	     chd[cnt].nminuts++;
	     chd[cnt].hours=chd[cnt].hours+res;
	     chd[cnt].nhours++;
	     chd[cnt].days=chd[cnt].days+res;
	     chd[cnt].ndays++;
    	    }
         if (chan[cnt].target==0) break;
	}
     usleep(100000);
    }
 close (fd);
}
//-----------------------------------------------------------------------------
INT  aScanBus (SHORT adr, UINT dev, UINT speed)
{
 if (!aOpenCom (dev,speed)) return -1;
 CHAR 	Out[15];
 CHAR* 	dest;
 CHAR 	sBuf1[100];
 INT	bytes=1,dwbr1=0;

 sprintf (Out,"$%02dM\r",adr);
 if (debug>1) ULOGW ("[adam] out=%s",Out);
 write (fd,&Out,5);
 sleep(2);
 ioctl (fd,FIONREAD,&bytes);
// if (bytes) read (fd, &sBuf1, 5);
 if (debug>1) ULOGW ("[adam] b.read = %d",bytes);
 if (bytes) read (fd, &sBuf1, 10);
 for (INT i=0;i<bytes;i++) 
    {
     if (sBuf1[i]==0xd) { sBuf1[i]=0; break; }
     if (debug>1) ULOGW ("[adam] [%d] 0x%x (%c)",i,sBuf1[i],sBuf1[i]);
    }

 sprintf (Out,"!%02d",adr);
 if (bytes)
 if ((dest=strstr(sBuf1,Out))!=NULL)
    {
     if (debug>0) ULOGW ("[adam] Device ADAM-%s found on address %d",dest+3,adr);
     return 1;
    }
 return 0;
}
//---------------------------------------------------------------------------------------------------
BOOL aOpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 if (blok<10)  sprintf (devp,"/dev/ttyS%d",blok);
 if (blok>9)  sprintf (devp,"/dev/ttyr0%d",blok-10);
 if (debug>0) ULOGW("[adam] attempt open com-port %s",devp);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[adam] error open com-port %s [%s]",devp,strerror(errno)); 
     return FALSE;
    }
 else if (debug>1) ULOGW("[adam] open com-port success"); 
 //bzero (&tio,sizeof(tio));
 tcflush(fd,TCIOFLUSH);
 tcgetattr(fd, &tio);
 cfsetospeed(&tio, B9600);
 tio.c_cflag |= B9600|CREAD|CLOCAL;
 tio.c_cflag &= ~CSIZE;
 tio.c_cflag |=CS8;
 tio.c_cflag &=~(PARENB|CSTOPB|CRTSCTS|IXON|ECHO);
 tio.c_lflag &= ~ICANON;
 tio.c_cc[VMIN] = 0;
 tio.c_cc[VTIME] = 10; //Time out in 10e-1 sec
 cfsetispeed(&tio, B9600);
 fcntl(fd, F_SETFL, /*FNDELAY*/0); // 
 tcsetattr(fd, TCSANOW, &tio);
 //tio.c_cflag = baudrate(speed) | CS8 | CLOCAL | CREAD;
 //tio.c_iflag = 0;
 //tio.c_lflag = ICANON;
 tcsetattr(fd,TCSANOW,&tio);  
 fcntl (fd,F_SETFL,FNDELAY);
 return TRUE;
}
