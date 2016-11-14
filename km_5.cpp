//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "km_5.h"
#include "db.h"
#include <fcntl.h>
#include "func.h"
//-----------------------------------------------------------------------------
static  db      dbase;
static  INT     fd;
static  termios tio;

static	 MYSQL_RES *res;
static	 MYSQL_ROW row;
static	 CHAR   	query[500];

extern  "C" UINT device_num;    // total device quant
extern  "C" UINT km_num;     	// total device quant
extern  "C" BOOL WorkRegim;     // work regim flag
extern  "C" tm *currenttime;    // current system time

extern  "C" DeviceR     dev[MAX_DEVICE];// device class
extern  "C" DeviceKM5  	km[MAX_DEVICE_KM];
extern  "C" DeviceDK	dk;

extern  "C" BOOL 	km5_thread;
extern  "C" UINT	debug;


static  union fnm fnum[5];
static  UINT    chan_num[MAX_DEVICE_KM]={0};

	UINT    read_km (BYTE* dat);
	BYTE    CRC(const BYTE* const Data, const BYTE DataSize, BYTE type);
        
        VOID    ULOGW (const CHAR* string, ...);              // log function
        UINT    baudrate (UINT baud);                   // baudrate select
static  BOOL    OpenCom (SHORT blok, UINT speed);       // open com function
//        VOID    Events (DWORD evnt, DWORD device);      // events 
static  BOOL    LoadKMConfig();                      // load tekon configuration
//static  BOOL    StoreData (UINT dv, UINT prm, UINT pipe, UINT status, FLOAT value);             // store data to DB
//static  BOOL    StoreData (UINT dv, UINT prm, UINT pipe, UINT type, UINT status, FLOAT value, CHAR* data);
//static  BOOL 	StoreDataC (UINT dv, UINT prm, UINT prm2, UINT type, UINT status, FLOAT value, CHAR* date);
//-----------------------------------------------------------------------------

void * kmDeviceThread (void * devr)
{
 int devdk=*((int*) devr);                      // DK identificator
 dbase.sqlconn("dk","root","");                 // connect to database
 km5_thread=true;

 // load from db km device
 LoadKMConfig();
 // open port for work
// BOOL rs=OpenCom (km[0].port, km[0].speed);
 BOOL rs=OpenCom (km[0].port, 9600);
 if (!rs) return (0);
    
// for (UINT d=0;d<=km_num;d++)
// for (UINT r=0;r<chan_num[d];r++)    
//	 km[d].ReadAllArchive (r,32);

 while (WorkRegim)
 for (UINT d=0;d<km_num;d++)
    {
     km[d].ReadInfo ();
     for (UINT r=0;r<chan_num[d];r++)
	{
        if (debug>3) ULOGW ("[km] ReadDataCurrent (%d)",r);
        km[d].ReadDataCurrent (r);
        //sleep (30);
        UpdateThreads (dbase, TYPE_INPUTKM-1, 1, 1, 1, km[d].device, 1, 0, (CHAR*)"");
//        if (currenttime->tm_min<20)
	if(1)
    	    {
             if (debug>3) ULOGW ("[km] ReadDataArchive (%d)",r);
             km[d].ReadAllArchive (r,25,0);
             if (debug>3) ULOGW ("[km] ReadDataArchive (%d)",r);
             km[d].ReadAllArchive (r,25,1);
	    }
        else sleep (1);
        if (!dk.pth[TYPE_INPUTKM])
    	    {
	    if (debug>0) ULOGW ("[km] km thread stopped");
	    //dbase.sqldisconn();
	    //pthread_exit ();	 
	    km5_thread=false;
	    pthread_exit (NULL);
	    return 0;	 
	    }
	}
    }
 dbase.sqldisconn();
 if (debug>0) ULOGW ("[km] km thread end");
 km5_thread=false;
}

//-----------------------------------------------------------------------------
// ReadInfo - read single device information: time, version, serial
int DeviceKM5::ReadInfo ()
{
 UINT rs=0;
 BYTE   data[400]={0}; 

 rs=send_km (CUR_REQUEST, 0, 0, 0, 0);
 if (rs)  rs = this->read_km(data, 0);
 rs=send_km (CUR_REQUEST, 0, 0, 0, 0);
 if (rs)  rs = this->read_km(data, 0);
 if (rs)
    {
     sprintf (this->devtim,"%04d%02d%02d%02d%02d%02d",2000+BCD(data[8]),BCD(data[7]),BCD(data[6]),BCD(data[10]),BCD(data[11]),BCD(data[12]));
     if (debug>3) ULOGW ("[km5] [date=%s]",this->devtim);
     sprintf (query,"UPDATE device SET lastdate=NULL,conn=1,devtim=%s WHERE idd=%d",this->devtim,this->iddev);
     if (debug>2) ULOGW ("[tek] [%s]",query);
     res=dbase.sqlexec(query); if (res) mysql_free_result(res); 
    }
 return 1;
}
//-----------------------------------------------------------------------------
// ReadDataCurrent - read single device. Readed data will be stored in DB
int DeviceKM5::ReadDataCurrent (UINT  sens_num)
{
 UINT   rs,chan;
 float  fl,prfl;
 BYTE   data[400]={0};
 CHAR   date[20];
 this->qatt++;  // attempt
 write (fd,&data,50);
 rs= this->read_km(data, 0);
 
 rs=send_km (CUR_REQUEST, this->addr[sens_num], this->addr[sens_num], 0, 0);
 if (rs)  rs = this->read_km(data, 0); 

 if (rs<5) 
    {
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>15) this->akt=0; // !!! 5 > normal quant from DB
     if (debug>3) ULOGW ("[km] Events[%d]",((3<<28)|(TYPE_INPUTKM<<24)|SEND_PROBLEM));
     if (this->qerrors==16) Events (dbase,(3<<28)|(TYPE_INPUTKM<<24)|SEND_PROBLEM,this->device);
    }
 else
    { 
     this->akt=1;
     this->qerrors=0;
     this->conn=1;
     sprintf (this->lastdate,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,1+currenttime->tm_mon,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);

     fl=*(float*)(data+this->cur[sens_num]+3);
     fl*=1.163;
     //for (UINT rr=0;rr<rs;rr++) ULOGW("[km] [%d][%x][%f]",rr,data[rr],*(float*)(data+rr));
     //ULOGW("[km] [%d][%f]",this->cur[sens_num],fl);
     if (fl<100000000 && fl>0) 
	{
	 //ULOGW ("[!!!] %d",this->device);
         chan=GetChannelNum (dbase,this->prm[sens_num],this->pipe[sens_num],this->device);
	 if (this->prm[sens_num]>20) StoreData (dbase,this->device, this->prm[sens_num]-10, this->pipe[sens_num], 0, fl, 0, chan);
	 else StoreData (dbase,this->device, this->prm[sens_num], this->pipe[sens_num], 0, fl, 0, chan);
         sprintf (date,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);
	 StoreData (dbase, this->device, this->prm[sens_num], this->pipe[sens_num], 9, 0, fl, date, 0, chan);
	}
     if (debug>2) ULOGW ("[km][%d][%d] [%d][%d] [0x%x 0x%x 0x%x 0x%x] [%f]",this->device,this->prm[sens_num],sens_num,this->cur[sens_num],data[this->cur[sens_num]+3],data[this->cur[sens_num]+4],data[this->cur[sens_num]+5],data[this->cur[sens_num]+6],fl);
    }
 return 0;
}
//-----------------------------------------------------------------------------
// ReadDataArchive - read single device. Readed data will be stored in DB
int DeviceKM5::ReadAllArchive (UINT  sens_num, UINT tp, UINT secd)
{
 bool   rs;
 BYTE   data[400];
 CHAR   date[20];
 UINT	month,year,index;
 float  value,prfl;
 UINT   code,vsk=0,chan;
 time_t tims;
 tims=time(&tims);
 struct tm *prevtime;		// current system time 
 
 this->qatt++;  // attempt


 if (this->n_hour[sens_num])
    {    
     prevtime=localtime(&tims); 	// get current system time
     rs=send_km (NSTR_REQUEST, this->n_hour[sens_num], 0, 0, 1);
     usleep (28000);
     rs = this->read_km(data, 1);
     //index=(data[6]&0xf0)>>4*10+(data[6]&0xf)+(data[7]&0xf0)>>4*1000+(data[6]&0xf)*100;
     index=data[7]*256+data[6];
     vsk=tp;
    
     while (index>=0 && vsk>0)
	 {	  
     	  rs=send_km (STR_REQUEST, this->n_hour[sens_num], index, 0, 2);
	  usleep (35000);
     	  rs = this->read_km(data, 2);
	  //if (debug>3) ULOGW ("[km] [rs=%d]",rs);
          if (data[5]!=0xee) break;
	  if (rs)  
	    {
	     int hrr=((data[10]&0xf0)>>4)*10+(data[10]&0xf);
	     if (hrr>0) hrr--;
	     else hrr=0;
	     sprintf (date,"%04d%02d%02d%02d0000",((data[8]&0xf0)>>4)*10+(data[8]&0xf)+2000,((data[7]&0xf0)>>4)*10+(data[7]&0xf),((data[6]&0xf0)>>4)*10+(data[6]&0xf),hrr);
	    }
	  if (rs)  { value=*(float*)(data+4+this->n_hour[sens_num]); value*=1.163; }
          if (rs)  if (debug>3) ULOGW ("[km] [1] [%d] [%d] [%d] [%x %x %x %x][%f]",index,sens_num,this->n_hour[sens_num],data[4+this->n_hour[sens_num]],data[5+this->n_hour[sens_num]],data[6+this->n_hour[sens_num]],data[7+this->n_hour[sens_num]],value);
          if (rs)  if (value<10000000 && value>0) 
	  if ((((data[8]&0xf0)>>4)*10+(data[8]&0xf))>8)
	    {	     
             chan=GetChannelNum (dbase,this->prm[sens_num],this->pipe[sens_num],this->device);
	     if (value>0) StoreData (dbase, this->device, this->prm[sens_num], this->pipe[sens_num], 1, 0, value, date, 0, chan);
	     //ULOGW ("%d %d = %d",this->prm[sens_num], this->pipe[sens_num],chan);

	     if (secd)
	     if (this->prm[sens_num]==11 || this->prm[sens_num]==13)
		{
		 sprintf (query,"SELECT value FROM hours WHERE type=1 AND device=%d AND prm=%d AND pipe=2 AND date<%s ORDER BY date DESC",this->device,this->prm[sens_num],date);
		 //ULOGW ("%s",query);
    		 res=dbase.sqlexec(query); 
		 if (row=mysql_fetch_row(res)) 
		    {
	             prfl=atof(row[0]);
		     prfl*=1.163;
		     value*=1.163;
		     //ULOGW ("[km] [1] %f %f",value,prfl);
		     if (prfl>0 && (value-prfl)>0 && (value-prfl)<100000)
			{
        		 chan=GetChannelNum (dbase,this->prm[sens_num],0,this->device);
    		         //ULOGW ("[km] [1] %d %d %d",this->prm[sens_num],0,this->device,chan);
	    	    	 StoreData (dbase, this->device, this->prm[sens_num], 0, 1, 0, (value-prfl), date,  0, chan);
			}
		    }
		 if (res) mysql_free_result(res);
	        }
	    }
	  if (index==0) break;
	  index--; vsk--;
	 }
    }

 if (this->n_day[sens_num])
    {    
     prevtime=localtime(&tims); 	// get current system time
     rs=send_km (NSTR_REQUEST, this->n_day[sens_num], 0, 1, 1);
     usleep (28000);
     rs = this->read_km(data, 1);
     //index=(data[6]&0xf0)>>4*10+(data[6]&0xf)+(data[7]&0xf0)>>4*1000+(data[6]&0xf)*100;
     index=data[7]*256+data[6];
     vsk=tp;
    
     while (index>=0 && vsk>0)
	 {	  
     	  rs=send_km (STR_REQUEST, this->n_day[sens_num], index, 1, 2);
	  usleep (35000);
     	  rs = this->read_km(data, 2);
	  //if (debug>3) ULOGW ("[km] [rs=%d]",rs);
          if (data[5]!=0xee) break;
	  if (rs)  
	    {
	     int hrr=((data[10]&0xf0)>>4)*10+(data[10]&0xf);
	     if (hrr>0) hrr--;
	     else hrr=0;
	     sprintf (date,"%04d%02d%02d000000",((data[8]&0xf0)>>4)*10+(data[8]&0xf)+2000,((data[7]&0xf0)>>4)*10+(data[7]&0xf),((data[6]&0xf0)>>4)*10+(data[6]&0xf));
	    }
	  if (rs)  { value=*(float*)(data+4+this->n_day[sens_num]); value*=1.163; }
          if (rs)  if (debug>2) ULOGW ("[km] [2] [%s] %d [%d] [%d] [%d] [%x %x %x %x][%f]",date,(((data[8]&0xf0)>>4)*10+(data[8]&0xf)),index,sens_num,this->n_hour[sens_num],data[4+this->n_hour[sens_num]],data[5+this->n_hour[sens_num]],data[6+this->n_hour[sens_num]],data[7+this->n_hour[sens_num]],value);
          if (rs)  if (value<100000000 && value>0) 
	  if ((((data[8]&0xf0)>>4)*10+(data[8]&0xf))>8)
	    {	     
             chan=GetChannelNum (dbase,this->prm[sens_num],this->pipe[sens_num],this->device);
	     if (value>0) StoreData (dbase, this->device, this->prm[sens_num], this->pipe[sens_num], 2, 0, value, date,  0, chan);

	     if (secd)
	     if (this->prm[sens_num]==11 || this->prm[sens_num]==13)
		{
		 sprintf (query,"SELECT value FROM prdata WHERE type=2 AND device=%d AND prm=%d AND pipe=2 AND date<%s ORDER BY date DESC",this->device,this->prm[sens_num],date);
		 //ULOGW ("!!!%s",query);
    		 res=dbase.sqlexec(query); 
		 if (row=mysql_fetch_row(res)) 
		    {
	             prfl=atof(row[0]);
		     prfl*=1.163;
		     value*=1.163;
		     //ULOGW ("[km] [2] %f %f",value,prfl);
		     if (prfl>0 && (value-prfl)>0 && (value-prfl)<100000)
			{
        		 chan=GetChannelNum (dbase,this->prm[sens_num],0,this->device);
    		         //ULOGW ("[km] [2] %d %d %d",this->prm[sens_num],0,this->device,chan);
	    	    	 StoreData (dbase, this->device, this->prm[sens_num], 0, 2, 0, (value-prfl), date,  0, chan);
			}
		    }
		 if (res) mysql_free_result(res);
	        }
	    }
	  if (index==0) break;
	  index--; vsk--;
	 }
    }

 if (this->n_day[sens_num])
    {    
     prevtime=localtime(&tims);
     rs=send_km (NSTR_REQUEST, this->n_day[sens_num], 0, 2, 1);
     usleep (28000);
     rs = this->read_km(data, 1);
     index=data[7]*256+data[6];
     vsk=tp;
    
     while (index>=0 && vsk>0)
	 {	  
     	  rs=send_km (STR_REQUEST, this->n_day[sens_num], index, 2, 2);
	  usleep (35000);
     	  rs = this->read_km(data, 2);
	  //if (debug>3) ULOGW ("[km] [rs=%d]",rs);
          if (data[5]!=0xee) break;
	  if (rs)  
	    {
	     //int hrr=((data[10]&0xf0)>>4)*10+(data[10]&0xf);
	     //if (hrr>0) hrr--;
	     //else hrr=0;
	     sprintf (date,"%04d%02d01000000",((data[8]&0xf0)>>4)*10+(data[8]&0xf)+2000,((data[7]&0xf0)>>4)*10+(data[7]&0xf),((data[6]&0xf0)>>4)*10+(data[6]&0xf));
	    }
	  if (rs)  { value=*(float*)(data+4+this->n_day[sens_num]); value*=1.163; }
          if (rs)  if (debug>2) ULOGW ("[km] [4] [%s] [%d] [%d] [%d] [%x %x %x %x][%f]",date,index,sens_num,this->n_day[sens_num],data[4+this->n_day[sens_num]],data[5+this->n_day[sens_num]],data[6+this->n_day[sens_num]],data[7+this->n_day[sens_num]],value);
          if (rs)  if (value<10000000 && value>0) 
	  if ((((data[8]&0xf0)>>4)*10+(data[8]&0xf))>8)
	    {	     
             chan=GetChannelNum (dbase,this->prm[sens_num],this->pipe[sens_num],this->device);
	     if (value>0) StoreData (dbase, this->device, this->prm[sens_num], this->pipe[sens_num], 4, 0, value, date,  0, chan);

	     if (secd)
	     if (this->prm[sens_num]==11 || this->prm[sens_num]==13)
		{
		 sprintf (query,"SELECT value FROM prdata WHERE type=4 AND device=%d AND prm=%d AND pipe=2 AND date<%s ORDER BY date DESC",this->device,this->prm[sens_num],date);
		 //ULOGW ("%s",query);
    		 res=dbase.sqlexec(query); 
		 if (row=mysql_fetch_row(res)) 
		    {
	             prfl=atof(row[0]);
		     prfl*=1.163;
		     value*=1.163;

		     //ULOGW ("[km] [4] %f %f",value,prfl);
		     if (prfl>0 && (value-prfl)>0 && (value-prfl)<5000000)
			{
        		 chan=GetChannelNum (dbase,this->prm[sens_num],0,this->device);
    		         //ULOGW ("[km] [1] %d %d %d",this->prm[sens_num],0,this->device,chan);
	    	    	 StoreData (dbase, this->device, this->prm[sens_num], 0, 4, 0, (value-prfl), date,  0, chan);
			}
		    }
		 if (res) mysql_free_result(res);
	        }
	    }
	  if (index==0) break;
	  index--; vsk--;
	 }
    }

 return 0;
}
//--------------------------------------------------------------------------------------
// load all km configuration from DB
BOOL LoadKMConfig()
{
 km_num=0;
 
 for (UINT d=0;d<device_num;d++)
 if (dev[d].type==TYPE_INPUTKM)
    {
     sprintf (query,"SELECT * FROM dev_km WHERE device=%d",dev[d].idd);
     res=dbase.sqlexec(query); 
     UINT nr=mysql_num_rows(res);
     for (UINT r=0;r<nr;r++)
	{
         row=mysql_fetch_row(res);
	 km[km_num].idt=km_num;
         km[km_num].iddev=dev[d].idd;
             km[km_num].device=dev[d].idd;
             km[km_num].SV=dev[d].SV;
             km[km_num].interface=dev[d].interface;
             km[km_num].protocol=dev[d].protocol;
             km[km_num].port=dev[d].port;
             km[km_num].speed=dev[d].speed;
             km[km_num].adr=dev[d].adr;
             km[km_num].type=dev[d].type;
             strcpy(km[km_num].number,dev[d].number);
             km[km_num].flat=dev[d].flat;
             km[km_num].akt=dev[d].akt;
             strcpy(km[km_num].lastdate,dev[d].lastdate);
             km[km_num].qatt=dev[d].qatt;
             km[km_num].qerrors=dev[d].qerrors;
             km[km_num].conn=dev[d].conn;
             strcpy(km[km_num].devtim,dev[d].devtim);
             km[km_num].chng=dev[d].chng;
             km[km_num].req=dev[d].req;
             km[km_num].source=dev[d].source;
             strcpy(km[km_num].name,dev[d].name);
        
         km[km_num].cur[r]=atoi(row[2]);
         km[km_num].prm[r]=atoi(row[3]);
         km[km_num].n_hour[r]=atoi(row[4]);
         km[km_num].n_day[r]=atoi(row[5]);
         km[km_num].addr[r]=atoi(row[6]);
	 km[km_num].pipe[r]=atoi(row[7]);
	 chan_num[km_num]++;     
        } 
    if (debug>0) ULOGW ("[km] device [0x%x],adr=%d total %d channels",km[km_num].device,km[km_num].adr, chan_num[km_num]);
    km_num++;
    }
 if (res) mysql_free_result(res);
}
//---------------------------------------------------------------------------------------------------
BOOL StoreDataC (UINT dv, UINT prm, UINT prm2, UINT type, UINT status, FLOAT value, CHAR* date)
{ 
 CHAR dat[20];
 if (type==1) sprintf (dat,"%04d%02d%02d%02d0000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour);
 if (type==2) sprintf (dat,"%04d%02d%02d000000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday);  
 float pw,pw2;
 time_t tim,tim2;
 double	dt;
 struct tm tt;
 struct tm ct;

 sprintf (query,"SELECT * FROM prdata WHERE type=%d AND prm=%d AND device=%d AND date=%s AND pipe=0 ORDER BY date DESC",type,prm,dv,date);
 if (debug>2) ULOGW ("[km] %s",query);
 res=dbase.sqlexec(query); pw2=0;
 if (row=mysql_fetch_row(res)) 
    {
     tim=time(&tim);
     localtime_r(&tim,&ct);     
     ct.tm_year=(row[4][0]-0x30)*1000+(row[4][1]-0x30)*100+(row[4][2]-0x30)*10+(row[4][3]-0x30);
     ct.tm_mon=(row[4][5]-0x30)*10+(row[4][6]-0x30);
     ct.tm_mday=(row[4][8]-0x30)*10+(row[4][9]-0x30);
     ct.tm_hour=(row[4][11]-0x30)*10+(row[4][12]-0x30);
     ct.tm_year-=1900; ct.tm_mon-=1;
     if (type==1) sprintf (dat,"%04d%02d%02d%02d0000",ct.tm_year+1900,ct.tm_mon+1,ct.tm_mday,ct.tm_hour);
     tim=mktime (&ct);
     pw2=atof(row[5]);
     if (debug>2) ULOGW ("[km] row=%s [%f]",row[4],pw2);
    }
    
 sprintf (query,"SELECT * FROM prdata WHERE type=%d AND prm=%d AND device=%d AND date<%s AND pipe=0 ORDER BY date DESC",type,prm,dv,date);
 if (debug>2) ULOGW ("[km] %s",query);
 res=dbase.sqlexec(query);
 if (row=mysql_fetch_row(res)) 
    {
     pw=atof(row[5]);
     if (debug>2) ULOGW ("[lk] row<%s [%f]",row[4],pw);
     //tim=time(&tim);
     tim2=time(&tim2);
     localtime_r(&tim2,&tt);     
     tt.tm_year=(row[4][0]-0x30)*1000+(row[4][1]-0x30)*100+(row[4][2]-0x30)*10+(row[4][3]-0x30);
     tt.tm_mon=(row[4][5]-0x30)*10+(row[4][6]-0x30);
     tt.tm_mday=(row[4][8]-0x30)*10+(row[4][9]-0x30);
     tt.tm_hour=(row[4][11]-0x30)*10+(row[4][12]-0x30);
     tt.tm_year-=1900; tt.tm_mon-=1;
     tim2=mktime (&tt);
     // substract and divide on hour (3600)     
     if (debug>2) ULOGW ("[km] -> %ld %ld (%d)[%f][%f][%f]",tim,tim2,(tim-tim2)/3600,value,pw2,pw);

     if (pw2-pw>=0 && pw>0 && (tim-tim2)>=3500) dt=(pw2-pw);
     else return false;

     if (debug>2) ULOGW ("[km][%d] -> %ld %ld (%d)[%f][%f] (%d)",dv,tim2,tim,(tim-tim2)/3600,dt,dt/((tim-tim2)/3600),prm2);
     if (type==1) dt=dt/((tim-tim2)/3600);
     
     if (res) mysql_free_result(res);       
     sprintf (query,"SELECT * FROM prdata WHERE type=%d AND prm=%d AND device=%d AND date=%s",type,prm2,dv,dat);
     res=dbase.sqlexec(query); 
     if (debug>1) ULOGW ("[km] %s",query);
     if (row=mysql_fetch_row(res)) 
        {
	 sprintf (query,"UPDATE prdata SET value=%f,status=0,date=date WHERE type=%d AND prm=%d AND device=%d AND date=%s",dt,type,prm2,dv,dat);
	 if (atof(row[5])==0 && dt>0) res=dbase.sqlexec(query);
         if (res) mysql_free_result(res);
	 return true;
	}
     else sprintf (query,"INSERT INTO prdata(device,prm,value,status,date,type) VALUES('%d','%d','%f','%d','%s','%d')",dv,prm2,dt,status,dat,type);
     if (res) mysql_free_result(res);
     if (debug>1) ULOGW ("[km] %s",query);
     res=dbase.sqlexec(query);
     if (res) mysql_free_result(res);  
    } 
 return true;
}

//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
// sprintf (devp,"/dev/ttyS%d",blok);
 sprintf (devp,"/dev/ttyr01",blok);

 if (debug>0) ULOGW("[km] attempt open com-port %s on speed %d",devp,speed);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[km] error open com-port %s [%s]",devp,strerror(errno)); 
     return false;
    }
 else if (debug>1) ULOGW("[km] open com-port success"); 

 tcflush(fd, TCIOFLUSH); //Clear send & recive buffers
 tcgetattr(fd, &tio);
 cfsetospeed(&tio, baudrate(speed));
 tio.c_cflag |= CREAD|CLOCAL|baudrate(speed);
 
 tio.c_cflag &= ~(CSIZE|PARENB|PARODD|CSTOPB|CRTSCTS);
 tio.c_cflag |=CS8;
// tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
 tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ISIG | ECHONL);
 tio.c_iflag &= ~(INLCR | IGNCR | ICRNL | INPCK | IGNPAR | PARMRK | ISTRIP | IGNBRK);
 tio.c_iflag = IGNPAR|IGNBRK;
 tio.c_iflag &= ~(IXON | IXOFF | IXANY); 
 tio.c_oflag &= ~(ONLCR | OCRNL); 

 tio.c_iflag = 0;

 tio.c_iflag &= ~(INLCR | IGNCR | ICRNL);
 tio.c_iflag &= ~(IXON | IXOFF | IXANY);
 
 //// tio.c_iflag = IGNCR;
 //tio.c_oflag &= ~(ONLCR);
 
 tio.c_cc[VMIN] = 0;
 tio.c_cc[VTIME] = 5; //Time out in 10e-1 sec
 cfsetispeed(&tio, baudrate(speed));
 //fcntl(fd, F_SETFL, FNDELAY);
 fcntl(fd, F_SETFL, 0);
 tcflush (fd,TCIFLUSH);
 tcsetattr(fd, TCSANOW, &tio);
 tcsetattr (fd,TCSAFLUSH,&tio);

 return true;
}
//-----------------------------------------------------------------------------
BOOL DeviceKM5::send_km (UINT op, UINT prm, UINT index, UINT type, UINT frame)
    {
     UINT       crc=0;          //(* CRC checksum *)
     UINT       nbytes = 0;     //(* number of bytes in send packet *)
     BYTE       data[100];      //(* send sequence *)
     BYTE	sn[2];
     //411709	 
     data[0]=(this->adr&0xff0000)>>16;
     data[1]=(this->adr&0xff00)>>8; 
     data[2]=this->adr&0xff;
     data[3]=0x0;
     
     if (frame==0)
        {
	 data[4]=op;
	 data[5]=prm;
	 data[6]=0x0;
	 data[7]=0x0; data[8]=0x0; data[9]=0x0; data[10]=0x0; data[11]=0x0; data[12]=0x0; data[13]=0x0;
         data[14]=CRC (data, 14, 1);
         data[15]=CRC (data, 14, 2);
         if (debug>2) ULOGW("[km] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15]);
         write (fd,&data,16);
        }     
     if (frame==1)
        {
	 data[4]=op;
	 data[5]=type;
	 if (debug>3) ULOGW("[km][%d] wr[%d,%d,%d,%d]",type,currenttime->tm_mday,1+currenttime->tm_mon,currenttime->tm_year-100,currenttime->tm_hour);
	 data[6]=((currenttime->tm_mday/10)<<4)+(currenttime->tm_mday%10); data[7]=((((currenttime->tm_mon+1)/10)<<4)+(currenttime->tm_mon+1)%10); data[8]=(((currenttime->tm_year-100)/10)<<4)+(currenttime->tm_year-100)%10; data[9]=((currenttime->tm_hour/10)<<4)+currenttime->tm_hour%10; 
	 data[10]=0x0; data[11]=0x0; data[12]=0x0; data[13]=0x0;
         data[14]=CRC (data, 14, 1);
         data[15]=CRC (data, 14, 2);
         if (debug>3) ULOGW("[km] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15]);
         write (fd,&data,16);
        }     
     if (frame==2)
        {
	 data[4]=op;
	 data[5]=type;
	 data[7]=index/256; data[6]=index%256;
	 data[8]=0x0; data[9]=0x0; data[10]=0x0; data[11]=0x0; data[12]=0x0; data[13]=0x0;
         data[14]=CRC (data, 14, 1);
         data[15]=CRC (data, 14, 2);
         if (debug>3) ULOGW("[km] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15]);
         write (fd,&data,16);
        }     
     if (frame==3)
        {
	 data[4]=4;
	 data[5]=0;
	 data[6]=0;
	 data[7]=0x0; data[8]=0x0; data[9]=0x0; data[10]=0x0; data[11]=0x0; data[12]=0x0; data[13]=0x0;
         data[14]=CRC (data, 14, 1);
         data[15]=CRC (data, 14, 2);
         if (debug>3) ULOGW("[km] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15]);
         write (fd,&data,16);
        }
     return true;     
    }
//-----------------------------------------------------------------------------    
UINT  DeviceKM5::read_km (BYTE* dat, BYTE type)
    {
     UINT       crc=0,crc2=0;   //(* CRC checksum *)
     INT        nbytes = 0;     //(* number of bytes in recieve packet *)
     INT        bytes = 0;      //(* number of bytes in packet *)
     BYTE       data[500];      //(* recieve sequence *)
     UINT       i=0;            //(* current position *)
     UCHAR      ok=0xFF;        //(* flajochek *)
     CHAR       op=0;           //(* operation *)

     usleep (250000);
     ioctl (fd,FIONREAD,&nbytes); 
     //if (debug>2) ULOGW("[km] nbytes=%d",nbytes);
     nbytes=read (fd, &data, 75);
     //if (debug>2) ULOGW("[km] nbytes=%d %x",nbytes,data[0]);
     usleep (200000);
     ioctl (fd,FIONREAD,&bytes);  
     
     if (bytes>0 && nbytes>0 && nbytes<50) 
        {
         if (debug>3) ULOGW("[km] bytes=%d fd=%d adr=%d",bytes,fd,&data+nbytes);
         bytes=read (fd, &data+nbytes, bytes);
         if (debug>3) ULOGW("[km] bytes=%d",bytes);
         nbytes+=bytes;
        }
     if (nbytes>5)
        {
         if (debug>3) ULOGW("[km] [%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",nbytes,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17]);  
	 crc=CRC (data, nbytes-2, 1);
	 crc2=CRC (data, nbytes-2, 2);
         if (debug>4) ULOGW("[km][crc][0x%x,0x%x | 0x%x,0x%x]",data[nbytes-2],data[nbytes-1],crc,crc2);
	 if (crc!=data[nbytes-2] || crc2!=data[nbytes-1]) nbytes=0;
         if (type==0x0)
            {
             memcpy (dat,data,32);
             return nbytes;
            }
         if (type==2)
            {
             memcpy (dat,data,nbytes);
             return nbytes;
            }
         if (type==1)
            {
             memcpy (dat,data,18);
             return nbytes;
            }
         return 0;
        }
     return 0;
    }
//-----------------------------------------------------------------------------        
BYTE CRC(const BYTE* const Data, const BYTE DataSize, BYTE type)
    {
     BYTE _CRC = 0;     
     BYTE* _Data = (BYTE*)Data; 

        for(unsigned int i = 0; i < DataSize; i++) 
		{
	         if (type==1) _CRC ^= *_Data++;
        	 if (type==2) _CRC += *_Data++;
		}
        return _CRC;
    }
