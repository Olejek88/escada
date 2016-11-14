//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "ce303.h"
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

extern  "C" UINT cem_num;     	// total device quant
extern  "C" BOOL WorkRegim;     // work regim flag
extern  "C" tm *currenttime;    // current system time

extern  "C" DeviceR     dev[MAX_DEVICE];// device class
extern  "C" DeviceCEM  	cem[MAX_DEVICE_CE];
extern  "C" DeviceDK	dk;

//extern  "C" BOOL	ce_thread;
extern  "C" UINT	debug;

extern  "C" UINT 	device_num;    // total device quant
extern  "C" UINT 	dev_num[30];  	// total device quant
extern	"C" BOOL	threads[30];

static  BYTE CRC(const uint8_t* const Data, const BYTE DataSize, BYTE type);        
static  uint16_t Crc16(const uint8_t* const Data, const uint8_t DataSize);

static  BOOL    OpenCom (SHORT blok, UINT speed);       // open com function
static  BOOL    LoadCEConfig();                      // load tekon configuration
//-----------------------------------------------------------------------------
void * cemDeviceThread (void * devr)
{
 int devdk=*((int*) devr);                      // DK identificator
 dbase.sqlconn("dk","root","");                 // connect to database
 ULOGW ("[%s303%s] CE-303 start thread",module_color,nc);
 LoadCEConfig();

 // open port for work
 BOOL rs=OpenCom (cem[0].port, cem[0].speed);
// BOOL rs=OpenTTY (fd, 2, cem[0].speed);
 if (!rs) return (0);
 
 while (WorkRegim)
 if (dk.pth[TYPE_CE303])
 for (UINT d=0;d<(dev_num[TYPE_CE303]);d++)
    {
     if (debug>1) ULOGW ("[303] ReadInfo (%d)",d);
     cem[d].ReadInfo (); 
     if (debug>1) ULOGW ("[303] ReadDataCurrent (%d)",d);
     cem[d].ReadDataCurrent (); 
    // if (debug>1) ULOGW ("[303] ReadDataArchive (%d)",d);
    // cem[d].ReadAllArchive (9);
     if (!dk.pth[TYPE_CE303])
        {
	 if (debug>0) ULOGW ("[303] ce-303 thread stopped");
	 //dbase.sqldisconn();
	 //pthread_exit ();	 
	 threads[TYPE_CE303]=FALSE;
	 pthread_exit (NULL);
	 return 0;	 
	}          
    }
 dbase.sqldisconn();
 if (debug>0) ULOGW ("[303] CE-303 thread end");
 threads[TYPE_CE303]=FALSE;
 pthread_exit (NULL); 
}
//-----------------------------------------------------------------------------
int DeviceCEM::ReadInfo ()
{
 UINT   rs,serial,soft;
 BYTE   data[400];
 CHAR   date[20]={0};
 CHAR   time[20]={0};

 rs=send_ce (SN, 0, date, 0);
 if (rs)  rs = this->read_ce(data, 0);
 if (rs)
	{
	 if (debug>2) ULOGW ("[303] %s[serial=%s]%s",bright,data,nc);
	}

 rs=send_ce (OPEN_PREV, 0, date, 0);
 if (rs)  rs = this->read_ce(data, 0);
// if (rs)  if (debug>2) ULOGW ("[303] [open channel prev: %d]",data[0]);

 rs=send_ce (OPEN_CHANNEL_CE, 0, date, 0);
 if (rs)  rs = this->read_ce(data, 0);
 if (rs)  if (debug>2) ULOGW ("[303] [open channel answer: %d]",data[0]);

 rs=send_ce (READ_DATE, 0, date, 0);
 if (rs)  rs = this->read_ce(data, 0);
 if (rs)
	{
	 memcpy (date,data+9,8);
	 rs=send_ce (READ_TIME, 0, date, 0);
	 if (rs)  rs = this->read_ce(data, 0);
	 if (rs)
		{
	    	 memcpy (time,data+6,8);
		 sprintf ((char *)data,"%s %s",date,time);
	    	 if (debug>2) ULOGW ("[303] %s[date=%s]%s",bright,data,nc);	    	 
		 sprintf (this->devtim,"20%c%c%c%c%c%c%c%c%c%c%c%c",data[6],data[7],data[3],data[4],data[0],data[1],data[9],data[10],data[12],data[13],data[15],data[16]);
                 sprintf (query,"UPDATE device SET lastdate=NULL,conn=1,devtim=%s WHERE id=%d",this->devtim,this->iddev);
    	         if (debug>2) ULOGW ("[tek] [%s]",query);
    		 //res=dbase.sqlexec(query); if (res) mysql_free_result(res); 
    		}
	}
}
//-----------------------------------------------------------------------------
// ReadDataCurrent - read single device. Readed data will be stored in DB
int DeviceCEM::ReadDataCurrent ()
{
 UINT   rs,chan;
 float  fl;
 BYTE   data[400];
 CHAR   date[20]={0};
 CHAR	param[20];
 this->qatt++;  // attempt

 rs=send_ce (CURRENT_W, 0, date, READ_PARAMETERS);
 if (rs)  rs = this->read_ce(data, 0);
 if (rs)
    {
     rs=sscanf ((const char *)data,"POWEP(%s)",param);
     ULOGW ("[303][%s] %d",data,rs);
     if (rs) 
        {
         fl=atof (param);
         if (debug>2) ULOGW ("[303][%s] %sW=[%f]%s",param,bright,fl,nc);
         chan=GetChannelNum (dbase,14,2,this->device);
	 StoreData (dbase, this->device, 14, 2, 0, fl, 0, chan);
	}
    } 
 rs=send_ce (CURRENT_U, 0, date, READ_PARAMETERS);
 if (rs)  rs = this->read_ce(data, 0);
 if (rs)
    {
     rs=sscanf ((const char *)data,"VOLTA(%s)",param);
     if (rs) 
        {
         fl=atof (param);
         if (debug>2) ULOGW ("[303][%s] %sV=[%f]%s",param,bright,fl,nc);
	 chan=GetChannelNum (dbase,14,13,this->device);
	 StoreData (dbase, this->device, 14, 13, 0, fl, 0, chan);
        }
    } 
 rs=send_ce (CURRENT_F, 0, date, READ_PARAMETERS);
 if (rs)  rs = this->read_ce(data, 0);
 if (rs)
    {
     rs=sscanf ((const char *)data,"FREQU(%s)",param);
     if (rs) 
        {
         fl=atof (param);
         if (debug>2) ULOGW ("[303][%s] %sF=[%f]%s",param,bright,fl,nc);
         chan=GetChannelNum (dbase,14,44,this->device);
	 StoreData (dbase, this->device, 14, 44, 0, fl, 0, chan);
	}
    } 
 rs=send_ce (CURRENT_I, 0, date, READ_PARAMETERS);
 if (rs)  rs = this->read_ce(data, 0);
 if (rs)
    {
     rs=sscanf ((const char *)data,"ET0PE(%s)",param);
     if (rs) 
        {
         fl=atof (param);
         if (debug>2) ULOGW ("[303][%s] %sI=[%f]%s",param,bright,fl,nc);
         chan=GetChannelNum (dbase,14,2,this->device);
	 StoreData (dbase, this->device, 14, 2, 0, fl, 0, chan);
	}
    } 

 return 0;
}
//-----------------------------------------------------------------------------
// ReadDataArchive - read single device. Readed data will be stored in DB
int DeviceCEM::ReadAllArchive (UINT tp)
{
 bool   rs;
 BYTE   data[400];
 CHAR   date[20],param[20];
 UINT	month,year,index,chan;
 float  fl;
 UINT   code,vsk=0;
 time_t tims,tim;
 tims=time(&tims);
 tims=time(&tims);
 struct tm *tt=(tm *)malloc (sizeof (struct tm));
 this->qatt++;  // attempt

 tim=time(&tim);
 localtime_r(&tim,tt);
 for (int i=0; i<tp; i++)
    {
     if (tt->tm_mon==0) { tt->tm_mon=12; tt->tm_year--; }
     sprintf (date,"EAMPE(%d.%d)",tt->tm_mon+1,tt->tm_year-100);		// ddMMGGtt
     rs=send_ce (ARCH_MONTH, 0, date, 1);
     if (rs)  rs = this->read_ce(data, 0);
     if (rs)
	    {
	     rs=sscanf ((const char *)data,"EAMPE(%s)",param);
             fl=atof (param);
	     sprintf (this->lastdate,"%04d%02d01000000",tt->tm_year+1900,tt->tm_mon); 
	     if (debug>2) ULOGW ("[303]%s[0x%x 0x%x 0x%x 0x%x] [%f] [%s]%s",bright,data[0],data[1],data[2],data[3],fl,this->lastdate,nc);
	     //UpdateThreads (dbase, TYPE_MERCURY230-1, 1, 1, 14, this->device, 1, 1, this->lastdate);
	     StoreData (dbase, this->device, 14, 0, 4, 0, fl,this->lastdate, 0, chan);
	    }

     sprintf (date,"ENMPE(%d.%d)",tt->tm_mon+1,tt->tm_year-100);	// ddMMGGtt
     rs=send_ce (ARCH_MONTH, 0, date, 1);
     if (rs)  rs = this->read_ce(data, 0);
     if (rs)
	    {
	     rs=sscanf ((const char *)data,"ENMPE(%s)",param);
             fl=atof (param);
	     sprintf (this->lastdate,"%04d%02d01000000",tt->tm_year+1900,tt->tm_mon); 
	     if (debug>2) ULOGW ("[303][0x%x 0x%x 0x%x 0x%x] [%f] [%s]",bright,data[0],data[1],data[2],data[3],fl,this->lastdate,nc);
	     //UpdateThreads (dbase, TYPE_MERCURY230-1, 1, 1, 14, this->device, 1, 1, this->lastdate);
	     StoreData (dbase, this->device, 14, 2, 4, 0, fl,this->lastdate, 0, chan);
	    }
     tt->tm_mon--;
    } 

 tim=time(&tim);
 localtime_r(&tim,tt);
 for (int i=0; i<tp; i++)
    {
//     sprintf (date,"%02d%02d%02d%02d",tt->tm_mday,tt->tm_mon+1,tt->tm_year,0);	// ddMMGGtt
     sprintf (date,"EADPE(%d.%d.%d)",tt->tm_mday,tt->tm_mon+1,tt->tm_year-100);	// ddMMGGtt
     rs=send_ce (ARCH_DAYS, 0, date, 1);
     if (rs)  rs = this->read_ce(data, 0);
     if (rs)
        {
         fl=((float)data[0]+(float)data[1]*256+(float)data[2]*256*256)/100;
         if (debug>2) ULOGW ("[303][0x%x 0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],data[3],fl);
         sprintf (this->lastdate,"%04d%02d%02d%02d0000",tt->tm_year+1900,tt->tm_mon+1,tt->tm_mday,tt->tm_hour);     
         if (fl>0) StoreData (dbase,this->device, 14, 0, 2, 0, fl, this->lastdate,0,chan);
	}
     sprintf (date,"ENDPE(%d.%d.%d)",tt->tm_mday,tt->tm_mon+1,tt->tm_year-100);	// ddMMGGtt
     rs=send_ce (ARCH_DAYS, 0, date, 1);
     if (rs)  rs = this->read_ce(data, 0);
     if (rs)
        {
         fl=((float)data[0]+(float)data[1]*256+(float)data[2]*256*256)/100;
         if (debug>2) ULOGW ("[303][0x%x 0x%x 0x%x 0x%x] [%f]",data[0],data[1],data[2],data[3],fl);
         sprintf (this->lastdate,"%04d%02d%02d%02d0000",tt->tm_year+1900,tt->tm_mon+1,tt->tm_mday,tt->tm_hour);     
         if (fl>0) StoreData (dbase,this->device, 14, 2, 2, 0, fl, this->lastdate,0,chan);
	}

     tim-=3600*i;
     localtime_r(&tim,tt);
    }
 free (tt);
 return 0;
}
//--------------------------------------------------------------------------------------
// load all km configuration from DB
BOOL LoadCEConfig()
{
 cem_num=0;
 UINT	nr=0;

 for (UINT d=0;d<device_num;d++)
 if (dev[d].type==TYPE_CE303)
 if (dev[d].ust)
    {
     cem[cem_num].idce=dev[d].id;
     cem[cem_num].iddev=dev[d].id;
     cem[cem_num].device=dev[d].idd;
     cem[cem_num].SV=dev[d].SV;
     cem[cem_num].interface=dev[d].interface;
     cem[cem_num].protocol=dev[d].protocol;
     cem[cem_num].port=dev[d].port;
     cem[cem_num].speed=dev[d].speed;
     cem[cem_num].adr=dev[d].adr;
     cem[cem_num].type=dev[d].type;
     strcpy(cem[cem_num].number,dev[d].number);
     cem[cem_num].flat=dev[d].flat;
     cem[cem_num].akt=dev[d].akt;
     strcpy(cem[cem_num].lastdate,dev[d].lastdate);
     cem[cem_num].qatt=dev[d].qatt;
     cem[cem_num].qerrors=dev[d].qerrors;
     cem[cem_num].conn=dev[d].conn;
     strcpy(cem[cem_num].devtim,dev[d].devtim);
     cem[cem_num].chng=dev[d].chng;
     cem[cem_num].req=dev[d].req;
     cem[cem_num].source=dev[d].source;
     strcpy(cem[cem_num].name,dev[d].name);

     sprintf (query,"SELECT * FROM dev_mer WHERE device=%d",cem[cem_num].device);
     //ULOGW ("%s",query);
     res=dbase.sqlexec(query); 
     nr=mysql_num_rows(res);
     for (UINT r=0;r<nr;r++)
    	{
         row=mysql_fetch_row(res);
         //mer[cem_num].adr_dest=atoi(row[3]);
         cem[cem_num].adrr[0]=atoi(row[4]);
         cem[cem_num].adrr[1]=atoi(row[5]);
         cem[cem_num].adrr[2]=atoi(row[6]);
         cem[cem_num].adrr[3]=atoi(row[7]);
         cem[cem_num].adrr[4]=atoi(row[8]);
         cem[cem_num].adrr[5]=atoi(row[9]);
         cem[cem_num].adrr[6]=atoi(row[10]);
         cem[cem_num].adrr[7]=atoi(row[11]);
         //mer[cem_num].Kn=atof(row[13]);
         //mer[cem_num].Kt=atof(row[14]);
         //mer[cem_num].A=atof(row[15]);
    	}
     if (res) mysql_free_result(res);

     sprintf (query,"SELECT * FROM dev_ce WHERE device=%d",cem[cem_num].device);
     //ULOGW ("%s",query);
     res=dbase.sqlexec(query); 
     nr=mysql_num_rows(res);
     for (UINT r=0;r<nr;r++)
    	{
         row=mysql_fetch_row(res);
         strcpy(cem[cem_num].pass,row[3]); 
        }
     if (res) mysql_free_result(res);
 
     if (debug>0) ULOGW ("[303] device [0x%x],adr=%d,pass=%s",cem[cem_num].device,cem[cem_num].adr,cem[cem_num].pass);
     cem_num++;
    }
// if (res) mysql_free_result(res);

 dev_num[TYPE_CE303]=cem_num;
}
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
// sprintf (devp,"/dev/ttyS%d",blok);
 speed=9600;
 sprintf (devp,"/dev/ttyr00",blok);

// sprintf (devp,"/dev/ttyUSB2");
// sprintf (devp,"/dev/ttyS2",blok);
 if (debug>0) ULOGW("[303] attempt open com-port %s on speed %d",devp,speed);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[303] error open com-port %s [%s]",devp,strerror(errno)); 
     return FALSE;
    }
 else if (debug>1) ULOGW("[303] open com-port success"); 

 tcflush(fd, TCIOFLUSH); //Clear send & recive buffers
 tcgetattr(fd, &tio);
 cfsetospeed(&tio, baudrate(speed));
 tio.c_cflag =0;
 tio.c_cflag |= CREAD|CLOCAL|baudrate(speed);
 
 tio.c_cflag &= ~(CSIZE|PARENB|PARODD);
 tio.c_cflag &= ~CSTOPB;
// tio.c_cflag |=CS8|PARENB|PARODD;
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
 fcntl(fd, F_SETFL, FNDELAY);
 //fcntl(fd, F_SETFL, 0);
 tcsetattr(fd, TCSANOW, &tio);
 tcsetattr (fd,TCSAFLUSH,&tio);
 
 return TRUE;
}
//-----------------------------------------------------------------------------
BOOL DeviceCEM::send_ce (UINT op, UINT prm, CHAR* request, UINT frame)
    {
     UINT       crc=0;          //(* CRC checksum *)
     UINT       ht=0,nr=0,len=0,nbytes = 0;     //(* number of bytes in send packet *)
     BYTE      	data[100];      //(* send sequence *)
     CHAR	path[100]={0};

     if (op==SN) // open/close session
	    len=5;
     if (op==0x4) // time
    	    len=13;
     if (op==OPEN_PREV)
	    len=6;
     if (op==OPEN_CHANNEL_CE)
	    len=14;
     if (op==READ_DATE || op==READ_TIME || op==READ_PARAMETERS)
	    len=13;
     if (op==ARCH_MONTH || op==ARCH_DAYS)
	    len=strlen(request)+6;

     if (this->protocol==13)
	{
	 for (ht=0;ht<8;ht++)
	    if (!this->adrr[ht]) break;
	    else nr++;

	 data[0]=1;	// redunancy
	 data[1]=6+3*nr+len;
	 data[2]=16+(nr-1);
	 data[3]=0; data[4]=0; data[5]=0;	// comp address
	 for (ht=0;ht<nr;ht++)
	     {
	      data[6+ht*3]=this->adrr[ht]/0x10000; data[7+ht*3]=(this->adrr[ht]&0xff00)/0x100; data[8+ht*3]=this->adrr[ht]&0xff;	// PPL-N address
	      sprintf (path,"%s[%d.%d.%d]",path,data[6+ht*3],data[7+ht*3],data[8+ht*3]);
	     }
	 data[6+nr*3]=0x40;
	 //data[6+nr*3]=0xf0;			// commands
	 ht=7+nr*3;
    	 if (debug>2) ULOGW("[303] [ht] open channel [%s] wr[0x%x 0x%x 0x%x, 0x%x 0x%x 0x%x, 0x%x 0x%x 0x%x, 0x%x 0x%x 0x%x, 0x%x]",path,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
	}

     data[ht+0]=this->adr&0xff; 
     data[ht+1]=op;

     if (op==SN)
        {
         data[ht+0]=START;
	 data[ht+1]=REQUEST;
	 data[ht+2]=0x21;
	 data[ht+3]=CR;
	 data[ht+4]=LF;
         if (debug>2) ULOGW("[303][SN] wr[0x%x,0x%x,0x%x,0x%x,0x%x]",data[ht+0],data[ht+1],data[ht+2],data[ht+3],data[ht+4]);

         crc=Crc16 (data+1, 4+ht);
         data[ht+5]=crc%256;
         data[ht+6]=crc/256;
         if (debug>2) ULOGW("[303] wr crc [0x%x,0x%x]",data[ht+5],data[ht+6]);
	 ht+=2;
         for (len=0; len<ht+5;len++)   ULOGW("[303] %d=%x(%d)",len,data[len],data[len]);
         write (fd,&data,5+ht);
	}

     if (op==OPEN_PREV)
        {
         data[ht+0]=0x6;
	 data[ht+1]=0x30;
	 data[ht+2]=0x35;
	 data[ht+3]=0x31;
	 data[ht+4]=CR;
	 data[ht+5]=LF;
         if (debug>2) ULOGW("[303][OC] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[ht+0],data[ht+1],data[ht+2],data[ht+3],data[ht+4],data[ht+5]);
         write (fd,&data,6+ht);
	}         

     if (op==OPEN_CHANNEL_CE)
        {
         data[ht+0]=0x1;
	 data[ht+1]=0x50;
	 data[ht+2]=0x31;
	 data[ht+3]=STX;
	 data[ht+4]=0x28;
	 // default password
	 data[ht+5]=0x37; data[ht+6]=0x37; data[ht+7]=0x37;
	 data[ht+8]=0x37; data[ht+9]=0x37; data[ht+10]=0x37;
	 // true password
	 memcpy (data+5,this->pass,6);
	 data[ht+11]=0x29;
	 data[ht+12]=ETX;
         data[ht+13]=CRC (data+ht, 13, 1);
         if (debug>2) ULOGW("[303][OPEN] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[ht+0],data[ht+1],data[ht+2],data[ht+3],data[ht+4],data[ht+5],data[ht+6],data[ht+7],data[ht+8],data[ht+9],data[ht+10],data[ht+11],data[ht+12],data[ht+13]);

         crc=Crc16 (data+1, 13+ht);
         data[ht+14]=crc%256;
         data[ht+15]=crc/256;
	 ht+=2;
         write (fd,&data,ht+14);
	}         
     if (op==READ_DATE || op==READ_TIME)
        {
         data[ht+0]=0x1;
	 data[ht+1]=0x52; data[ht+2]=0x31; data[ht+3]=STX;
	 if (op==READ_DATE) sprintf ((char *)data+4,"DATE_()");
	 if (op==READ_TIME) sprintf ((char *)data+4,"TIME_()");
	 data[ht+11]=ETX;
         data[ht+12]=CRC (data+ht, 12, 1);
         if (debug>2) ULOGW("[303][DATE|TIME] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[ht+0],data[ht+1],data[ht+2],data[ht+3],data[ht+4],data[ht+5],data[ht+6],data[ht+7],data[ht+8],data[ht+9],data[ht+10],data[ht+11],data[ht+12]);
         write (fd,&data,13);
	}         
     if (frame==READ_PARAMETERS)
        {
         data[ht+0]=0x1;
	 data[ht+1]=0x52; data[ht+2]=0x31; data[ht+3]=STX;
	 if (op==CURRENT_W) sprintf ((char *)data+4,"POWEP()");
	 //if (op==CURRENT_I) sprintf (data+4,"CURRE()");
	 if (op==CURRENT_I) sprintf ((char *)data+4,"ET0PE()");
	 if (op==CURRENT_F) sprintf ((char *)data+4,"FREQU()");
	 if (op==CURRENT_U) sprintf ((char *)data+4,"VOLTA()");
	 data[11]=ETX;
         data[12]=CRC (data+ht, 12, 1);
         if (debug>2) ULOGW("[303][CURR] wr[%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",13,data[ht+0],data[ht+1],data[ht+2],data[ht+3],data[ht+4],data[ht+5],data[ht+6],data[ht+7],data[ht+8],data[ht+9],data[ht+10],data[ht+11],data[ht+12]);
         write (fd,&data,13);
	}         
     if (op==ARCH_MONTH || op==ARCH_DAYS)
        {
         data[ht+0]=0x1;
	 data[ht+1]=0x52; data[ht+2]=0x31; data[ht+3]=STX;
	 sprintf ((char *)data+ht+4,"%s",request);
	 len=strlen(request)+3;
	 data[ht+len+1]=ETX;
         data[ht+len+2]=CRC (data+ht, len+2, 1);
         //crc=CRC (data+ht, 7, 0);
         if (debug>2) ULOGW("[303][ARCH] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[ht+0],data[ht+1],data[ht+2],data[ht+3],data[ht+4],data[ht+5],data[ht+6],data[ht+7],data[ht+8],data[ht+9],data[ht+10],data[ht+11],data[ht+12],data[ht+13],data[ht+14],data[ht+15],data[ht+16],data[ht+17]);
         write (fd,&data,len+3);
	}         

     return true;     
    }
//-----------------------------------------------------------------------------    
UINT  DeviceCEM::read_ce (BYTE* dat, BYTE type)
    {
     UINT       crc=0;		//(* CRC checksum *)
     INT        nbytes = 0;     //(* number of bytes in recieve packet *)
     INT        bytes = 0;      //(* number of bytes in packet *)
     BYTE       data[500];      //(* recieve sequence *)
     UINT       i=0;            //(* current position *)
     UCHAR      ok=0xFF;        //(* flajochek *)
     CHAR       op=0;           //(* operation *)

     usleep (250000);
     //sleep(1);
     ioctl (fd,FIONREAD,&nbytes); 
     if (debug>2) ULOGW("[303] nbytes=%d",nbytes);
     nbytes=read (fd, &data, 75);
     //data[0]=0x2;
     //sprintf (data+1,"POWEP(%.5f)",8.14561);
     //data[13]=0x3; data[14]=CRC (data+1,13,1); nbytes=15;
     //if (debug>2) ULOGW("[303] nbytes=%d %x",nbytes,data[0]);
     usleep (200000);
     ioctl (fd,FIONREAD,&bytes);  
     
     if (bytes>0 && nbytes>0 && nbytes<50) 
        {
         if (debug>2) ULOGW("[ce] bytes=%d fd=%d adr=%d",bytes,fd,&data+nbytes);
         bytes=read (fd, &data+nbytes, bytes);
         if (debug>2) ULOGW("[ce] bytes=%d",bytes);
         nbytes+=bytes;
        }
     if (nbytes==1)
	{
         if (debug>2) ULOGW("[ce] [%d][0x%x]",nbytes,data[0]);
         dat[0]=data[0];
         return nbytes;
        }
     if (nbytes>5)
        {
	 crc=CRC (data+1, nbytes-2, 1);
         if (debug>2) ULOGW("[ce] [%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x][crc][0x%x,0x%x]",nbytes,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[nbytes-1],crc);
	 //for (UINT rr=0; rr<nbytes;rr++) if (debug>2) ULOGW("[ce] [%d][0x%x]",rr,data[rr]);
	 if (data[nbytes-1]!=0xa && data[nbytes-2]!=0xd)
    	    if (crc!=data[nbytes-1] || nbytes<8) nbytes=0;

	 if (nbytes<100 && nbytes>7)
	    {
             if (nbytes>16)
        	    memcpy (dat,data+11,nbytes-11);
             else 
		    {
		     if (nbytes==12 && (data[9]==0xf0 || data[9]==0x40))
			{
			 ULOGW("[303] HTC answer: no counter answer present [%d]",data[9]);
			 Events (dbase, 2, this->device, 2, 0, 2, 0);
			}
		     if (nbytes==16 && (data[11]&0xf==0x5))
			 ULOGW("[303] channel not open correctly [%d]",data[11]);
		     memcpy (dat,data+11,nbytes-11);
		    }

             memcpy (dat,data+1,nbytes-3);
             dat[nbytes-3]=0;
            }
         else dat[0]=0;
         return nbytes;
        }
     return 0;
    }
//-----------------------------------------------------------------------------        
BYTE CRC(const uint8_t* const Data, const BYTE DataSize, BYTE type)
    {
     BYTE _CRC = 0;     
     for(int i= 0; i<DataSize; i++){
	if (Data[i]%2==1) _CRC += (Data[i]|0x80);
	else _CRC += ((Data[i])&0x7f);
    }
    if (_CRC>0x80) _CRC-=0x80;
    return _CRC;
     
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
//-----------------------------------------------------------------------------        
uint16_t Crc16(const uint8_t* const Data, const uint8_t DataSize)
{
    uint8_t	p=0,w=0,d=0,q=0;
    uint8_t 	sl=0,sh=0;
    for(p=0;p<DataSize;p++)
	{
	 d=Data[p];
         for(w=0;w<8;++w)
	    {
	     q=0;
	     if(d&1)++q;
	     d>>=1; 	// d - байт данных
	     if(sl&1)++q;
	     sl>>=1; 	// sl - младший байт контрольной суммы
	     if(sl&8)++q;
	     if(sl&64)++q;
	     if(sh&2)++q; 	// sh - старший байт контрольной суммы
	     if(sh&1)sl|=128;
	     sh>>=1;
	     if(q&1)sh|=128;
	    }
	}
//    sh|=128;
    return sl+sh*256;
}