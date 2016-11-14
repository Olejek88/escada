//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "tecon_19.h"
#include "db.h"
#include <fcntl.h>
#include "func.h"
//-----------------------------------------------------------------------------
static  db      dbase;
static  INT     fd;
static  termios tio;
static  CHAR 	n_pack=0;

static	 MYSQL_RES *res;
static	 MYSQL_ROW row;
static	 CHAR   	query[500];

static  INT s;			// socket
static	struct 	hostent *he;		// 
static	sockaddr_in si;

extern  "C" UINT device_num;    // total device quant
extern  "C" UINT tekon_num;     // total device quant
extern  "C" BOOL WorkRegim;     // work regim flag
extern  "C" tm *currenttime;    // current system time

extern  "C" DeviceR     dev[MAX_DEVICE];// device class
extern  "C" DeviceTekon tekon[MAX_DEVICE_TEKON];
extern  "C" DeviceDK	dk;
extern  "C" BOOL	tek_thread;
extern  "C" UINT	debug;

static  union fnm fnum[5];
UINT    chan_num[MAX_DEVICE_TEKON]={0};
        //BOOL    send_tekon (UINT op);
        UINT    read_tekon (BYTE* dat);
        BYTE    CRC(const BYTE* const Data, const BYTE DataSize);
        VOID    ULOGW (const CHAR* string, ...);              // log function
	UINT    baudrate (UINT baud);                   // baudrate select
static  BOOL    OpenCom (SHORT blok, UINT speed);       // open com function
static  BOOL    LoadTekonConfig();                      // load tekon configuration
//-----------------------------------------------------------------------------
void * tekonDeviceThread (void * devr)
{
 BOOL rs=0;
 debug=4;

 int devdk=*((int*) devr);                      // DK identificator
 dbase.sqlconn("stend5","root","");             // connect to database
 ULOGW ("[tek] LoadTekonConfig()");
 
 LoadTekonConfig();


 while (WorkRegim)
 for (UINT d=0;d<tekon_num;d++)
// if (tekon[d].device==3338)
    {
     if (tekon[d].interface!=2 && tekon[d].interface!=4)
	    {
	     sleep (5);
	     ULOGW ("[tek] unknown interface %d for device %d",tekon[d].interface,tekon[d].device);
	     continue;
	    }
     if (tekon[d].interface==2)
	{
         rs=OpenCom (tekon[d].port, tekon[d].speed);
         if (!rs) {
	     ULOGW ("[tek] error open port /ttyS%d for device %d",tekon[d].port,tekon[d].port);
	     sleep (5);
	     continue;
	    }
	}
     if (tekon[d].interface==4 && strlen (tekon[d].number)>6)
	{
	 he=gethostbyname(tekon[d].number);
	 ULOGW ("[tek] attempt connect to %s:%d",tekon[d].number,T19_PORT);
//	 s = socket(AF_INET, SOCK_STREAM, 0);
	 s = socket(AF_INET, SOCK_DGRAM, 0);
	 if (s==-1)
	    {
	     ULOGW ("[tek] error > can't open socket %d",T19_PORT);
    	     Events (dbase, 3, tekon[d].device, 3, 0, 3, 0);
	     sleep (5);
	     continue;
	    }
	 si.sin_family = AF_INET;
	 si.sin_port = htons(T19_PORT);
	 //si.sin_addr.s_addr = htonl(INADDR_ANY);
	 si.sin_addr = *((struct in_addr *)he->h_addr);
	 if (connect(s, (struct sockaddr *)&si, sizeof(si)) < 0)
	    {
	     ULOGW ("[tek] error connect to %s",tekon[d].number);
    	     Events (dbase, 4, tekon[d].device, 4, 0, 3, 0);
	     close (s);
	     sleep (10);
	     continue;
	    }
	 else ULOGW ("[tek] connect success to %s",tekon[d].number);
	 if (fcntl(s, F_SETFL, O_NONBLOCK, 1) == -1)
	    {
	     ULOGW ("[tek] failed to set to non-blocking");
	     continue;
	    }
	} 
     for (UINT r=0;r<chan_num[d];r++)
	{
         if (debug>2) ULOGW ("[tek] ReadDataCurrent (%d/%d)",r,tekon[d].device);
         //UpdateThreads (dbase, TYPE_INPUTTEKON-1, 1, 1, tekon[d].channel[r], tekon[d].device, 1, 0, (CHAR*)"");
         tekon[d].ReadDataCurrent (r);
	 //sleep (5);
         if (currenttime->tm_min>20)
    	    {
             if (debug>2) ULOGW ("[tek] ReadDataArchive (%d)",r);
             tekon[d].ReadAllArchive (r,5);
	    }	
	  if (!dk.pth[TYPE_INPUTTEKON])
    	    {
	     if (debug>0) ULOGW ("[tek] tekon thread stopped");
	     //dbase.sqldisconn();
	     tek_thread=false;	// we are finished
	     pthread_exit (NULL);
	     return 0;
	    }
	}
     if (tekon[d].interface==2)
	     close (fd);
     if (tekon[d].interface==4)
	     close (s);
    }
 UpdateThreads (dbase, TYPE_INPUTTEKON, 1, 0, 0, 0, 1, 0, (CHAR*)"");

 dbase.sqldisconn();
 if (debug>0) ULOGW ("[tek] tekon thread end");
 tek_thread=false;	// we are finished
 pthread_exit (NULL); 
}
//-----------------------------------------------------------------------------
// ReadDataCurrent - read single device. Readed data will be stored in DB
int DeviceTekon::ReadDataCurrent (UINT  sens_num)
{
 UINT   rs,chan;
 float  fl;
 BYTE   data[400];
 CHAR   date[20];
 this->qatt++;  // attempt

 if (!sens_num)
    {
     if (this->protocol==1) 
        {
         rs=send_tekon (CMD_READ_PARAM, 0, 0, 6);
	 if (rs)  rs = this->read_tekon(data);
         if (rs)  if (debug>2) ULOGW ("[tek] [%d] [%x %x %x]",rs,data[0],data[1],data[2]);

         rs=send_tekon (CMD_READ_PARAM, 0, 0, 7);
	 if (rs)  rs = this->read_tekon(data);
         if (rs)  if (debug>2) ULOGW ("[tek] [%d] [%x %x %x]",rs,data[0],data[1],data[2]);
         rs=send_tekon (CMD_READ_PARAM, 0, 0, 7);
	 if (rs)  rs = this->read_tekon(data);
         if (rs)  if (debug>2) ULOGW ("[tek] [%d] [%x %x %x]",rs,data[0],data[1],data[2]);	 
	}

//     if (this->adr==1) send_tekon (CMD_SET_ACCESS_LEVEL, 0xf017, 0, 5);
//     this->read_tekon(data);
     rs=send_tekon (CMD_SET_ACCESS_LEVEL, 0xf028, 0, 2);
     if (rs)  rs = this->read_tekon(data);
     sleep (1);
     rs=send_tekon (CMD_READ_PARAM, 0xf028, 0, 0);
     if (rs)  rs = this->read_tekon(data);
     if (rs)
	{
    	 if (debug>2) ULOGW ("[tek] [task: %x %x %x %x]",data[0],data[1],data[2],data[3]);
	}     
     sleep (2);
     rs=send_tekon (CMD_READ_PARAM, 0xf017, 0, 0);
     if (rs)  rs = this->read_tekon(data);
     if (rs)
	{ 
         sleep (1);
	 usleep (500000);
	 //sprintf (this->devtim,"%d%02d%02d",2000+(10*(data[3]>>4)+data[3]&0xf),(10*(data[2]>>4)+data[2]&0xf),(10*(data[1]>>4)+data[1]&0xf));
         sprintf (this->devtim,"20%02x%02x%02x",data[3],data[2],data[1]);

	 rs=send_tekon (CMD_READ_PARAM, 0xf018, 0, 0);
         if (rs)  rs = this->read_tekon(data);
	 if (rs)
    	    {
             //sprintf (this->devtim+8,"%02d%02d%02d",(10*(data[3]>>4)+data[3]&0xf),(10*(data[2]>>4)+data[2]&0xf),(10*(data[1]>>4)+data[1]&0xf));
	     sprintf (this->devtim+8,"%02x%02x%02x",data[3],data[2],data[1]);
	     sprintf (query,"UPDATE device SET lastdate=NULL,conn=1,devtim=%s WHERE id=%d",this->devtim,this->iddev);
    	     if (debug>2) ULOGW ("[tek] [%s]",query);
             res=dbase.sqlexec(query); if (res) mysql_free_result(res); 
	    }
	}
    }
 sleep (1);

 rs=send_tekon (CMD_READ_PARAM, this->cur[sens_num], 0, 0);
 if (rs)  rs = this->read_tekon(data); 

 if (rs<5) 
    {
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>15) this->akt=0; // !!! 5 > normal quant from DB
     if (debug>2) ULOGW ("[tek] Events[%d]",((3<<28)|(TYPE_INPUTTEKON<<24)|SEND_PROBLEM));
     if (this->qerrors==16) Events (dbase,(3<<28)|(TYPE_INPUTTEKON<<24)|SEND_PROBLEM,this->device);
    }
 else
    { 
     this->akt=1;
     this->qerrors=0;
     this->conn=1;
     sprintf (this->lastdate,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,1+currenttime->tm_mon,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);
     fl=*(float*)(data);
     if (debug>3) ULOGW ("[tek] [%d] [%f]",sens_num,fl);
     chan=GetChannelNum (dbase,this->prm[sens_num], this->pipe[sens_num],this->device);
     StoreData (dbase, this->device, this->prm[sens_num], this->pipe[sens_num], 0, fl, 0, chan);
     sprintf (date,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);
     StoreData (dbase, this->device, this->prm[sens_num], this->pipe[sens_num], 9, 0, fl, date, 0, chan);
    }
 return 0;
}
//-----------------------------------------------------------------------------
// ReadDataArchive - read single device. Readed data will be stored in DB
int DeviceTekon::ReadAllArchive (UINT  sens_num, UINT tp)
{
 bool   rs;
 BYTE   data[400];
 CHAR   date[20];
 UINT	month,year,index,chan;
 float  value;
 UINT   code,vsk=0;
 time_t tims;
 tims=time(&tims);
 struct 	tm *prevtime;		// current system time 

 chan=GetChannelNum (dbase,this->prm[sens_num], this->pipe[sens_num],this->device); 
 this->qatt++;  // attempt

 //if (this->adr==2) send_tekon (CMD_READ_PARAM, 0xf017, 0, 2);
 //this->read_tekon(data);
 // usleep (25000);
 //if (this->adr==2) send_tekon (CMD_READ_PARAM, 0xf017, 0, 3);
 //this->read_tekon(data);

 //usleep (15000);
 //if (this->adr==2) send_tekon (CMD_READ_PARAM, 0xf017, 0, 4); 
 //usleep (25000);
 //this->read_tekon(data);
 rs=send_tekon (CMD_READ_INDEX_PARAM, this->n_hour[sens_num], 0, 1);
 rs = this->read_tekon(data);

 if (this->n_hour[sens_num])
    {    
     prevtime=localtime(&tims); 	// get current system time
     if (prevtime->tm_year%4==0) vsk=0; else vsk=1;
     index=24*(((prevtime->tm_year-100)*365+(prevtime->tm_year-100)/4+prevtime->tm_yday+vsk)%64)+prevtime->tm_hour;
     tims-=3*60*60;
     //if (debug>2) ULOGW ("[tekon] [%d] [%f]",sens_num,value);
     if (index>tp*5) month=index-tp*5; else month=0;
     while (index>=month)
	 {
	  //usleep (8000);
     	  //rs = this->read_tekon(data);
     	  rs=send_tekon (CMD_READ_INDEX_PARAM, this->n_hour[sens_num], index, 1);
     	  rs = this->read_tekon(data);
	  sleep (2);
          prevtime=localtime(&tims); 	// get current system time
	  if (rs)  sprintf (date,"%04d%02d%02d%02d0000",prevtime->tm_year+1900,prevtime->tm_mon+1,prevtime->tm_mday,prevtime->tm_hour);
          //SetDeviceStatus(dbase,TYPE_INPUTTEKON, sens_num, this->adr, 1, date); 
    	  //UpdateThreads (dbase, TYPE_INPUTTEKON-1, 1, 1, this->channel[sens_num], this->device, 1, 1, date);
	  if (currenttime->tm_year<prevtime->tm_year || currenttime->tm_year<prevtime->tm_year) break;	
	  if (rs)  value=*(float*)(data); 
          if (rs)  if (debug>2) ULOGW ("[tek] [1] [%d] [%d] [%f]",index,sens_num,value);
          if (rs)  StoreData (dbase, this->device, this->prm[sens_num], this->pipe[sens_num], 1, 0, value, date, 0, chan);
	  if (index==0) break;
	  tims-=60*60;
	  index--;
	 }
    }

 if (this->n_day[sens_num])
    {     
     tims=time(&tims);
     tims-=24*60*60;
     prevtime=localtime(&tims); 	// get current system time
     //sprintf (date,"%04d%02d%02d000000",prevtime->tm_year+1900,prevtime->tm_mon+1,prevtime->tm_mday);
     //ULOGW ("[21] [%s]",date);
     //tims-=24*60*60;
     //prevtime=localtime(&tims); 	// get current system time
     //sprintf (date,"%04d%02d%02d000000",prevtime->tm_year+1900,prevtime->tm_mon+1,prevtime->tm_mday);
     //ULOGW ("[22] [%s]",date);

     index=prevtime->tm_yday-1;
     if (index>tp) month=index-tp; else month=0;

     while (index>month)
	 {
     	  rs=send_tekon (CMD_READ_INDEX_PARAM, this->n_day[sens_num], index, 1);
	  usleep (20000);
	  rs = this->read_tekon(data);
	  
	  prevtime=localtime(&tims); 	// get current system time
	  if (rs)  sprintf (date,"%04d%02d%02d000000",prevtime->tm_year+1900,prevtime->tm_mon+1,prevtime->tm_mday);
	  //SetDeviceStatus(dbase,TYPE_INPUTTEKON, sens_num, this->adr, 2, date);
    	  //UpdateThreads (dbase, TYPE_INPUTTEKON-1, 1, 1, this->channel[sens_num], this->device, 1, 2, date);

	  if (currenttime->tm_year<prevtime->tm_year || currenttime->tm_year<prevtime->tm_year || index>365) break;	
	  if (rs)  value=*(float*)(data); 
	  if (rs)  if (debug>2) ULOGW ("[2] [%d] [%d] [%f] [%s]",sens_num,index,value,date);
	  if (rs)  StoreData (dbase, this->device, this->prm[sens_num], this->pipe[sens_num], 2, 0, value, date, 0, chan);
	  tims-=24*60*60;
	  if (index==0) break;
	  index--;
	 }
    }

 if (this->n_month[sens_num])
    {
     tims=time(&tims);
     prevtime=localtime(&tims); 	// get current system time
     index=(prevtime->tm_year%4)*12+prevtime->tm_mon;
     if (index==0) { index=47; prevtime->tm_mon=11; prevtime->tm_year--; }
     else index--;
//01-11 11:03:47 [23] [0] [110]
//01-11 11:03:47 [23] [11] [109]
//01-11 11:03:47 [4] [8] [23] [63.710899] [20100001000000]
//01-11 11:03:47 [tek] INSERT INTO data(flat,prm,type,value,status,date,source) VALUES('0','11','4','63.710899','0','20100001000000','5')
     //if (debug>2) ULOGW ("[%d] [%d] [%d]",index,prevtime->tm_mon,prevtime->tm_year);
     //if (prevtime->tm_mon==0) { prevtime->tm_mon=11; prevtime->tm_year--; }
     sprintf (date,"%04d%02d01000000",prevtime->tm_year+1900,prevtime->tm_mon+1);
     //if (debug>2) ULOGW ("[%d] [%d] [%d] [%s]",index,prevtime->tm_mon,prevtime->tm_year+1900,date);     
     
     month=0; tp=5;
     while (month<1)
	 {
	  rs=send_tekon (CMD_READ_INDEX_PARAM, this->n_month[sens_num], index, 1);
	  usleep (20000);
	  if (rs)  rs = this->read_tekon(data);
	  if (rs)  sprintf (date,"%04d%02d01000000",prevtime->tm_year+1900,index-(prevtime->tm_year%4)*12+1);
	  //SetDeviceStatus(dbase,TYPE_INPUTTEKON, sens_num, this->adr, 4,date);
    	  //UpdateThreads (dbase, TYPE_INPUTTEKON-1, 1, 1, this->channel[sens_num], this->device, 1, 4, date);

	  if (rs)  value=*(float*)(data); 
	  if (rs)  if (debug>2) ULOGW ("[4] [%d] [%d] [%f] [%s]",sens_num,index,value,date);
	  if (rs)  StoreData (dbase, this->device, this->prm[sens_num], this->pipe[sens_num], 4, 0, value, date, 0, chan);
	  month++;
	  if (prevtime->tm_mon>0) prevtime->tm_mon--;
	  else { prevtime->tm_mon=11; prevtime->tm_year--; }
	  if (index>0) index--;	  
	 }
    }

 //if (this->adr==2) send_tekon (CMD_READ_PARAM, 0xf017, 0, 2);
 //this->read_tekon(data);
 //usleep (25000);
 //if (this->adr==2) send_tekon (CMD_READ_PARAM, 0xf017, 0, 3);
 //this->read_tekon(data);
     
 return 0;
}

//-----------------------------------------------------------------------------
// ReadDataArchive - read single device. Readed data will be stored in DB
int DeviceTekon::ReadDataArchive (UINT  sens_num)
{
 bool   rs;
 BYTE   data[400];
 CHAR   date[20];
 UINT	month,year;
 float  value;
 UINT   code;
 time_t tims;
 tims=time(&tims);
 struct 	tm *prevtime;		// current system time 
 
 this->qatt++;  // attempt
 if (this->n_hour[sens_num])
    {
     rs=send_tekon (CMD_READ_PARAM, this->n_hour[sens_num], 0, 0);
     if (rs)  rs = this->read_tekon(data);
     //tims-=60*60;
     prevtime=localtime(&tims); 	// get current system time
     if (rs)  sprintf (date,"%04d%02d%02d%02d0000",prevtime->tm_year+1900,prevtime->tm_mon+1,prevtime->tm_mday,prevtime->tm_hour);
     if (rs)  value=*(float*)(data); 
     if (rs)  if (debug>2) ULOGW ("[tek] [1] [%d] [%f]",sens_num,value);
     if (rs)  StoreData (dbase, this->device, this->prm[sens_num], this->pipe[sens_num], 1, 0, value, date, 1);
    }
 if (this->n_day[sens_num])
    {     
     rs=send_tekon (CMD_READ_PARAM, this->n_day[sens_num], 0, 0);
     if (rs)  rs = this->read_tekon(data);
     //tims-=24*60*60;
     prevtime=localtime(&tims); 	// get current system time
     if (rs)  sprintf (date,"%04d%02d%02d000000",prevtime->tm_year+1900,prevtime->tm_mon+1,prevtime->tm_mday);
     if (rs)  value=*(float*)(data); 
     if (rs)  if (debug>2) ULOGW ("[tek] [2] [%d] [%f]",sens_num,value);
     if (rs)  StoreData (dbase, this->device, this->prm[sens_num], this->pipe[sens_num], 2, 0, value, date, 1);
    }
 if (this->n_month[sens_num])
    {
     rs=send_tekon (CMD_READ_PARAM, this->n_month[sens_num], 0, 0);
     if (rs)  rs = this->read_tekon(data);
     if (currenttime->tm_mon>0) 
        {
         month=currenttime->tm_mon; year=currenttime->tm_year+1900;
        }
     else 
        {
         month=12; year=currenttime->tm_year+1900-1;
        } 
     if (rs)  sprintf (date,"%04d%02d01000000",year,month);
     if (rs)  value=*(float*)(data); 
     if (rs)  if (debug>2) ULOGW ("[tek] [4] [%d] [%f]",sens_num,value);
     if (rs)  StoreData (dbase, this->device, this->prm[sens_num], this->pipe[sens_num], 4, 0, value, date, 1);
    }
 /*
 rs=send_tekon (CMD_READ_INDEX_PARAM, tekon.n_hour[sens_num], 0, 1);
 if (rs)  rs = this->read_tekon(data);
 if (rs)  sprintf (date,"%d%02d%02d",2000+(10*(data[3]>>4)+data[3]&0xf),(10*(data[2]>>4)+data[2]&0xf),(10*(data[1]>>4)+data[1]&0xf));

 if (rs)  rs=send_tekon (CMD_READ_INDEX_PARAM, tekon.n_hour[sens_num], 1, 1);
 if (rs)  rs = this->read_tekon(data);
 if (rs)  sprintf (date+7,"%02d%02d%02d",(10*(data[3]>>4)+data[3]&0xf),(10*(data[2]>>4)+data[2]&0xf),(10*(data[1]>>4)+data[1]&0xf));
 if (rs)  rs=send_tekon (CMD_READ_INDEX_PARAM, tekon.n_hour[sens_num], 2, 1);
 if (rs)  rs = this->read_tekon(data);
 if (rs)  value=*(float*)(data); 
 if (rs)  rs=send_tekon (CMD_READ_INDEX_PARAM, tekon.n_hour[sens_num], 3, 1);
 if (rs)  rs = this->read_tekon(data);
 if (rs  code=*(uint*)(data);
 
 if (rs)  if (debug>0) ULOGW ("[tekon] [A %s] [%f] (%d)",date, value, code);
 if (rs)  StoreData (this->device, this->prm[sens_num], 1, code, value, date);
*/    
 if (!rs) 
    {
     if (debug>2) ULOGW ("[tek] Events[%d]",((3<<28)|(TYPE_INPUTTEKON<<24)|SEND_PROBLEM));
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>15) this->akt=0; // !!! 5 > normal quant from DB
     //Events ((3<<28)|(TYPE_INPUTTEKON<<24)|SEND_PROBLEM,this->device);
    }
 else
    { 
     this->akt=1;
     this->qerrors=0;
     this->conn=1;
     if (debug>2) ULOGW ("[tek] [%d] [%f]",sens_num,value);
     sprintf (this->lastdate,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,1+currenttime->tm_mon,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);     
    }

 return 0;
}
//--------------------------------------------------------------------------------------
// load all tekon configuration from DB
BOOL LoadTekonConfig()
{
 tekon_num=0;
 for (UINT d=0;d<device_num;d++)
 if (dev[d].type==TYPE_INPUTTEKON)
 if (dev[d].ust)
    {
     tekon[tekon_num].iddev=dev[d].id;
     tekon[tekon_num].device=dev[d].idd;
     tekon[tekon_num].SV=dev[d].SV;
     tekon[tekon_num].interface=dev[d].interface;
     tekon[tekon_num].protocol=dev[d].protocol;
     tekon[tekon_num].port=dev[d].port;
     tekon[tekon_num].speed=dev[d].speed;
     tekon[tekon_num].adr=dev[d].adr;
     tekon[tekon_num].type=dev[d].type;
     strcpy(tekon[tekon_num].number,dev[d].number);
     tekon[tekon_num].flat=dev[d].flat;
     tekon[tekon_num].akt=dev[d].akt;
     strcpy(tekon[tekon_num].lastdate,dev[d].lastdate);
     tekon[tekon_num].qatt=dev[d].qatt;
     tekon[tekon_num].qerrors=dev[d].qerrors;
     tekon[tekon_num].conn=dev[d].conn;
     strcpy(tekon[tekon_num].devtim,dev[d].devtim);
     tekon[tekon_num].chng=dev[d].chng;
     tekon[tekon_num].req=dev[d].req;
     tekon[tekon_num].source=dev[d].source;
     strcpy(tekon[tekon_num].name,dev[d].name);

     sprintf (query,"SELECT * FROM channels WHERE device=%d",dev[d].idd);
     res=dbase.sqlexec(query); 
     UINT nr=mysql_num_rows(res);
     if (nr>0)
     for (UINT r=0;r<nr;r++)
	{
         row=mysql_fetch_row(res);
	 tekon[tekon_num].idt=tekon_num;
	 tekon[tekon_num].pipe[r]=atoi(row[10]);
         tekon[tekon_num].cur[r]=atoi(row[5]);
         tekon[tekon_num].prm[r]=atoi(row[9]);
         tekon[tekon_num].nak[r]=atoi(row[4]);

         tekon[tekon_num].n_hour[r]=atoi(row[6]);
         tekon[tekon_num].n_day[r]=atoi(row[7]);
         tekon[tekon_num].n_month[r]=atoi(row[8]);
         tekon[tekon_num].channel[r]=atoi(row[0]);
	 chan_num[tekon_num]++;     
        } 
    if (debug>0) ULOGW ("[tek] device [0x%x],adr=%d total %d channels",tekon[tekon_num].device,tekon[tekon_num].adr, chan_num[tekon_num]);
    tekon_num++;
    }
 if (res) mysql_free_result(res);
}
//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 sprintf (devp,"/dev/ttyS%d",blok);
 if (debug>0) ULOGW("[tek] attempt open com-port %s on speed %d",devp,speed);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[tek] error open com-port %s [%s]",devp,strerror(errno)); 
     return false;
    }
 else if (debug>1) ULOGW("[tek] open com-port success"); 

 tcflush(fd, TCIOFLUSH); //Clear send & recive buffers
 tcgetattr(fd, &tio);
 cfsetospeed(&tio, baudrate(speed));
 tio.c_cflag = CS8|CREAD|baudrate(speed)|CLOCAL;
// tio.c_cflag &= ~(CSIZE|PARENB|PARODD);
// tio.c_cflag &= ~CSTOPB;
// tio.c_cflag &= ~CRTSCTS;
// tio.c_lflag = ICANON;
 tio.c_lflag = 0;
// tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
 tio.c_iflag = IGNPAR| ICRNL;
 tio.c_oflag = 0;
// tio.c_iflag &= ~(INLCR | IGNCR | ICRNL);
// tio.c_iflag &= ~(IXON | IXOFF | IXANY);
 
// tio.c_iflag = IGNCR | IGNPAR;
// tio.c_oflag &= ~(ONLCR);
 
 tio.c_cc[VMIN] = 0;
 tio.c_cc[VTIME] = 5; //Time out in 10e-1 sec
 cfsetispeed(&tio, baudrate(speed));
 fcntl(fd, F_SETFL, FNDELAY);
 //fcntl(fd, F_SETFL, 0);
 tcsetattr(fd, TCSANOW, &tio);
 tcsetattr (fd,TCSAFLUSH,&tio);

 return true;
}
//-----------------------------------------------------------------------------
BOOL DeviceTekon::send_tekon (UINT op, UINT prm, UINT index, UINT frame)
    {
     UINT       crc=0;          //(* CRC checksum *)
     UINT       nbytes = 0;     //(* number of bytes in send packet *)
     BYTE       data[100];      //(* send sequence *)
     BYTE	sn[2];

     int iFlags;
     if (this->protocol==1 && this->interface==2)
        {
         ioctl(fd, TIOCMGET, &iFlags);
         // SET RTS & DTR
	 iFlags |= TIOCM_RTS | TIOCM_DTR;
         ioctl(fd, TIOCMSET, &iFlags);
         // SET DTR
	 iFlags |= TIOCM_DTR;
         ioctl(fd, TIOCMSET, &iFlags);
         // CLEAR DTR
         iFlags &= ~TIOCM_DTR;
         ioctl(fd, TIOCMSET, &iFlags);
         tcsetattr (fd,TCSAFLUSH,&tio);
	}
     
     if (frame==0) // for 3 bytes
        {
	 if (this->protocol==2)
	    {
    	     // 10 4D 01 01 17 F0 00 56 16        
             data[0]=0x10; 
             data[1]=0x4D;
             if (this->adr==0) // 1!!!!
		{
    	         data[2]=this->adr&0xFF;
                 data[3]=(CHAR)op;
                 data[4]=prm&0xff;
                 data[5]=(prm&0xff00)>>8;
                 data[6]=(prm&0xff0000)>>16;
		}
	     else
		{
    	         data[2]=(CHAR)op;
	         data[3]=0x11;
                 data[4]=this->adr&0xFF;
                 data[5]=prm&0xff;
                 data[6]=(prm&0xff00)>>8;
		}
             data[7]=CRC (data+1, 6);
             data[8]=0x16;
	     if (debug>2) ULOGW("[tek] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
	     if (this->interface==4) write (s,&data,9);
	     if (this->interface==2) write (fd,&data,9);
	    }
	 if (this->protocol==1)
	    {
	     data[0]=this->adr;			// komu
	     data[1]=((n_pack&0x3)<<5)|0x80;	// zapros + nomer paketa
	     data[2]=0;				// CAN 0
	     data[3]=0;				// CAN 0
	     data[4]=4;				// DLC - num bytes

	     data[5]=0;				// ot kogo
	     data[6]=2;				// read one parametr command
	     data[7]=prm&0xff;			// param1
	     data[8]=(prm&0xff00)>>8;		// param2
	     data[9]=data[10]=data[11]=data[12]=0; 	// zero
	     if (debug>3) ULOGW("[tek] wr[%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",n_pack,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
	     n_pack++;
	     if (n_pack>3) n_pack=0;
	     if (this->interface==4) write (s,&data,13);
	     if (this->interface==2) write (fd,&data,13);
	    }                 
        }     
     if (frame==1) // for 4 bytes only
        {          
	 if (this->protocol==2)
	    {
	     // 68 07 07 68 46 01 15 01 09 FF 00 65 16
	     // 68 a a 68 40 1
             data[0]=0x68; 
             data[1]=0xa;
	     data[2]=0xa;
             data[3]=0x68;
	     data[4]=0x40;
	     data[5]=0x1;
	     data[6]=0x19;
             data[7]=this->adr&0xFF;
	     data[8]=prm&0xff;
    	     data[9]=(prm&0xff00)>>8;
	     data[10]=index&0xff;
	     data[11]=(index&0xff00)>>8;
	     data[12]=1;
	     data[13]=CRC (data+4, 9);
    	     data[14]=0x16;
    	     if (debug>3) ULOGW("[tek] out[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
    	     if (this->interface==4) write (s,&data,15);
	     if (this->interface==2) write (fd,&data,15);
	    }
	 if (this->protocol==1)
	    {
	     data[0]=this->adr;			// komu
	     data[1]=((n_pack&0x3)<<5)|0x80;	// zapros + nomer paketa
	     data[2]=0;				// CAN 0
	     data[3]=0;				// CAN 0
	     data[4]=6;				// DLC - num bytes

	     data[5]=0;				// ot kogo
	     data[6]=8;				// read one parametr command
	     data[7]=prm&0xff;			// param1
	     data[8]=(prm&0xff00)>>8;		// param2
	     data[9]=index&0xff;		// index1
	     data[10]=(index&0xff00)>>8;	// index2
	     data[11]=data[12]=0; 		// zero
	     if (debug>3) ULOGW("[tek] wr[%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",n_pack,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
	     n_pack++;
	     if (n_pack>3) n_pack=0;
    	     if (this->interface==4) write (s,&data,13);
	     if (this->interface==2) write (fd,&data,13);
	    }         		
	}
     if (frame==2)
        {
         data[0]=0x68; 
         data[1]=0x4;
         data[2]=0x4;
         data[3]=0x68;	 
         data[4]=0x40;
         data[5]=0x1;
         data[6]=0x14;
         data[7]=0x03;
         data[8]=0x02;
         data[9]=0x05;
         data[10]=0x02;
         data[11]=0x61;
         data[12]=0x16;
	 if (debug>3) ULOGW("[tek] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
         if (this->interface==4) write (s,&data,13);
	 if (this->interface==2) write (fd,&data,13);
        }     
     if (frame==3)
        {
         data[0]=0x10; 
         data[1]=0x40;
         data[2]=0x1;
         data[3]=0x11;
         data[4]=0x2;
         data[5]=0x1c;
         data[6]=0xf0;
         data[7]=0x60;
         data[8]=0x16;
	 if (debug>3) ULOGW("[tek] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
         if (this->interface==4) write (s,&data,9);
	 if (this->interface==2) write (fd,&data,9);
        }     
     if (frame==4)
        {
         data[0]=0x10; 
         data[1]=0x40;
         data[2]=0x1;
         data[3]=0x11;
         data[4]=0x2;
         data[5]=0x61;
         data[6]=0x80;
         data[7]=0x35;
         data[8]=0x16;
	 if (debug>3) ULOGW("[tek] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
         if (this->interface==4) write (s,&data,9);
	 if (this->interface==2) write (fd,&data,9);
        }     
     if (frame==5)
        {
	 if (this->protocol==2)
	    {	
	     data[0]=0x10;
    	     data[1]=0x40;
             data[2]=0x1;
             data[3]=0x17;
             data[4]=0x1;
             data[5]=0x0;
             data[6]=0x0;
             data[7]=0x59;
             data[8]=0x16;
	     if (debug>3) ULOGW("[tek] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
             if (this->interface==4) write (s,&data,9);
	     if (this->interface==2) write (fd,&data,9);
	    }
	 if (this->protocol==1)
	    {
	     data[0]=this->adr;			// komu
	     data[1]=((n_pack&0x3)<<5)|0x80;	// zapros + nomer paketa
	     data[2]=0;				// CAN 0
	     data[3]=0;				// CAN 0
	     data[4]=4;				// DLC - num bytes

	     data[5]=0;				// ot kogo
	     data[6]=2;				// read one parametr command
	     data[7]=prm&0xff;			// param1
	     data[8]=(prm&0xff00)>>8;		// param2
	     data[9]=data[10]=data[11]=data[12]=0; 	// zero
	     if (debug>3) ULOGW("[tek] wr[%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",n_pack,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
             if (this->interface==4) write (s,&data,13);
	     if (this->interface==2) write (fd,&data,13);
	    }	
        }     
     if (frame==6)
        {
	 for (int dd=0;dd<36;dd++) data[dd]=0;
	 if (debug>3) ULOGW("[tek] wr[%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",n_pack,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
	 write (fd,&data,23);
//	 usleep(100);
	 data[1]=0xff; data[5]=0xff; data[9]=0x2; data[10]=0xe0; data[11]=0x41; data[12]=0xdf; 
//	 for (int dd=0;dd<36;dd++) write (fd,&data+dd,36);	    
         //00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FF 00 00 00 FF 00 00 00 02 E0 41 DF 	
         if (debug>3) ULOGW("[tek] wr[%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",n_pack,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
         if (this->interface==4) write (s,&data,13);
	 if (this->interface==2) write (fd,&data,13);
        }     
     if (frame==7)
        {
         // 00 80 00 00 04 00 02 00 F0 00 00 00 00 	
	 data[0]=0; data[1]=0x80; data[2]=0; data[3]=0; data[4]=4; data[5]=0;
	 data[6]=2; data[7]=0; data[8]=0xf0; data[9]=0; data[10]=0; data[11]=0; data[12]=0;
         if (debug>3) ULOGW("[tek] wr[%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",n_pack,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
         write (fd,&data,13);	    
	 sleep(1);
	 // 00 A0 00 00 04 00 02 00 F0 00 00 00 00 	
	 data[0]=0; data[1]=0xa0; data[2]=0; data[3]=0; data[4]=4; data[5]=0;
	 data[6]=2; data[7]=0; data[8]=0xf0; data[9]=0; data[10]=0; data[11]=0; data[12]=0;
         if (debug>3) ULOGW("[tek] wr[%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",n_pack,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
         if (this->interface==4) write (s,&data,13);
	 if (this->interface==2) write (fd,&data,13);
	}
     return true;     
    }
//-----------------------------------------------------------------------------    
UINT  DeviceTekon::read_tekon (BYTE* dat)
    {
     UINT       crc=0;          //(* CRC checksum *)
     INT        nbytes = 0;     //(* number of bytes in recieve packet *)
     INT        bytes = 0;      //(* number of bytes in packet *)
     BYTE       data[300];      //(* recieve sequence *)
     UINT       i=0;            //(* current position *)
     UCHAR      ok=0xFF;        //(* flajochek *)
     CHAR       op=0;           //(* operation *)
     socklen_t fromlen;
     fromlen = sizeof(si);
     usleep (220000);
     if (this->interface==4)
	{
	 //nbytes=read (s, &data, 255);
	 nbytes = recvfrom(s, &data, sizeof(data), 0, (struct sockaddr *)&si, &fromlen);
         if (debug>3) ULOGW("[tek] read tekon %d",nbytes);
	 if (nbytes==0)
	    {
	     usleep (220000);
	     //nbytes = recvfrom(s, &data, sizeof(data), 0, (struct sockaddr *)&si, &fromlen);
	     nbytes=read (s, &data, 255);	
             if (debug>1) ULOGW("[tek] read tekon %d",nbytes);
	    }
	}
     else	
	{
    	 ioctl (fd,FIONREAD,&nbytes); 
    	 if (debug>3) ULOGW("[tek] nbytes=%d",nbytes);
         nbytes=read (fd, &data, 20);
         if (debug>4) ULOGW("[tek] nbytes=%d %x",nbytes,data[0]);
         usleep (8000);
         ioctl (fd,FIONREAD,&bytes);  
         if (debug>4) ULOGW("[tek] bytes=%d",bytes);     
         if (bytes>0 && nbytes>0 && nbytes<50) 
    	    {
             bytes=read (fd, &data+nbytes, 30);
             //if (debug>1) ULOGW("[tek] bytes=%d",bytes);
             nbytes+=bytes;
    	    }
	}
     //  0  1  2  3  4  5  6  7  8  9 10 11 12 13
     // 10 00 01 02 00 00 00 03 16
     // 68 08 08 68 46 01 15 29 F0 B0 01 08 2E 16 
     if (nbytes>5)
        {
	 if (debug>2) 
             if (this->protocol==2) ULOGW("[tek] rd[%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",nbytes,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11]);  
	     else ULOGW("[tek] rd[%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",nbytes,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);  

	 if (this->protocol==2)
	    {
             if (data[0]==0x10)
	        {
    	         crc=CRC (data, nbytes-2);
        	 if (crc==data[nbytes-2]) ok=0;
                 memcpy (dat,data+0x3,4);
	         return nbytes;
    		}
             if (data[0]==0x68)
        	{
                 crc=CRC (data, 12);
                 if (crc==data[12]) ok=0;
                 if (data[1]==data[2]) ok=0;
                 memcpy (dat,data+0x6,6);
                 return nbytes;
		}
            }
	 if (this->protocol==1)
	    {
	     // 00 60 75 4E 02 01 00 02 00 43 00 A9 F0
	     memcpy (dat,data+0x3,data[1]);
	     return nbytes;
	    }
         return 0;
        }
     else
        {
	 if (debug>3) ULOGW("[tek] negative answer [%d][0x%x,0x%x,0x%x]",nbytes,data[0],data[1],data[2]);
	}
     return 0;
    }
//-----------------------------------------------------------------------------        
BYTE CRC(const BYTE* const Data, const BYTE DataSize)
    {
     BYTE _CRC = 0;
     
     BYTE* _Data = (BYTE*)Data; 
        for(unsigned int i = 0; i < DataSize; i++) 
             _CRC += *_Data++;
        return _CRC;
    }
