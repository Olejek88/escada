//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "lk.h"
#include "db.h"
#include "func.h"
//-----------------------------------------------------------------------------
static	db	dbase;
static INT	fd;
static termios 	tio;

static	 MYSQL_RES *res;
static	 MYSQL_ROW row;
static	 CHAR   	query[500];

extern 	"C" UINT device_num;	// total device quant
extern 	"C" UINT lk_num;	// LK sensors quant
extern 	"C" UINT bit_num;	// BIT sensors quant
extern 	"C" UINT ip2_num;	// 2IP sensors quant
extern 	"C" BOOL WorkRegim;	// work regim flag
extern 	"C" tm *currenttime;	// current system time
extern 	"C" UINT mee_num;	// MEE sensors quant

float    bitData	[MAX_DEVICE_BIT][6];	//
float	ip2Data	[MAX_DEVICE_2IP][10];   //

extern 	"C" DeviceR 	dev[MAX_DEVICE];	// device class
extern 	"C" DeviceLK 	lk[MAX_DEVICE_LK];	// local concantrator class
extern 	"C" DeviceBIT 	bit[MAX_DEVICE_BIT];	// BIT class
extern 	"C" Device2IP 	ip2[MAX_DEVICE_2IP];	// 2IP class
extern 	"C" DeviceMEE 	mee[MAX_DEVICE_MEE];	// 2IP class
extern  "C" DeviceDK	dk;

extern  "C" BOOL	lk_thread;
extern  "C" UINT	debug;

static 	int sped=0,totalfound=0, tst=0;
//------------------------------------------------------------------------------
struct Factory
{
 BYTE ManIdCode;	// Код производителя
 BYTE DevTypeFunc;	// Код типа устройства по функциональному назначению
 DWORD ID;
};
Factory	FS_src,FS_dst;

static	union fnm fnum[5];
UINT 	p1=0,p2=0;
//------------------------------------------------------------------------------
VOID 	ULOGW (const CHAR* string, ...);		// log function
UINT 	baudrate (UINT baud);			// baudrate select
UCHAR 	Crc8 (BYTE* data, UINT lent);		// crc checksum function
BYTE 	crc8_compute_tabel ( BYTE* str, BYTE col);
UCHAR 	Crc7530 (BYTE* data, UINT lent);		// crc checksum function
bool 	send_lk_can (UINT idlk, BYTE* data, BYTE lent);
bool 	send_long_lk_can (UINT idlk, BYTE* data, BYTE lent,UINT frame);
bool 	set_can_speed (BYTE speed);
bool 	read_status_7530 (UINT com);

static	BOOL 	OpenCom (SHORT blok, UINT speed);	// open com function
//VOID 	Events (DWORD evnt, DWORD device);	// events 
static	BOOL	CheckStatusAll ();			// main service for all sensors function
//BOOL 	LoadLKConfig();				// load all lk configuration
static	BOOL 	LoadBITConfig();			// load all bit configuration
static	BOOL 	Load2IPConfig();			// load all 2ip configuration
static	BOOL 	LoadMEEConfig();			// load all MEE configuration

static	BOOL 	StoreDataC (UINT dv, UINT prm, UINT prm2, UINT type, UINT status, FLOAT value);
//static	BOOL 	StoreData (UINT dv, UINT prm, UINT status, FLOAT value, UINT pipe);
//static 	BOOL 	StoreData (UINT dv, UINT prm, UINT type, UINT status, FLOAT value, CHAR* date);	// store data to DB
//-----------------------------------------------------------------------------
void * lkDeviceThread (void * devr)
{
 int devdk=*((int*) devr);			// DK identificator
 dbase.sqlconn("dk","root","");			// connect to database
 lk_thread=true;				// we are started
 // load from db all lk devices
 lk[0].LoadLKConfig();
 // load from db all bit devices > store to class members
 if (debug>2) ULOGW ("[lk] LoadBITConfig()");
 LoadBITConfig();
 // load from db all 2ip devices > store to class members
 //if (debug>2) ULOGW ("[lk] Load2IPConfig()");
 //Load2IPConfig(); 
 // load from db all 2ip devices > store to class members
 //if (debug>2) ULOGW ("[lk] LoadMEEConfig()");
 //LoadMEEConfig();
 // open port for work
 if (lk_num>0)
    {
     BOOL rs=OpenCom (lk[0].port, lk[0].speed);
    }
 else return (0);

 while (WorkRegim)
    {
     set_can_speed (2);
     //read_status_7530 (0);
     //read_status_7530 (1);
     //read_status_7530 (2);
     for (UINT r=0;r<lk_num;r++)
        {
         if (debug>2) ULOGW ("[lk] lk[%d/%d].ReadDataCurrent ()",r,lk_num);
         lk[r].ReadDataCurrent ();
	}
     if (0)
     if (!dk.pth[TYPE_LK])
        {
	 if (debug>0) ULOGW ("[lk] lk thread stopped");
	 //Events ((3<<28)|(TYPE_LK<<24)|THREAD_STOPPED,devdk);
	 //dbase.sqldisconn();
	 lk_thread=false;	// we are finished
	 pthread_exit (NULL);
	 return 0;
	}
     sleep (2);
     //dbase.sqldisconn();
     //dbase.sqlconn("dk","root","");			// connect to database
    }
 dbase.sqldisconn();
 if (debug>0) ULOGW ("[lk] lk thread end");
 lk_thread=false;		// we are finished
 pthread_exit (NULL);
}
//---------------------------------------------------------------------------------------------------
// ReadDataCurrent - read single device connected to concentrator
// Readed data will be stored in DB
int DeviceBIT::ReadDataCurrent ()
{
 int 	rs,rep=2;
 BYTE	data[400];
 CHAR	dat[50];
 float	h,hs,ta,tl,vcc,pr,rssi,chip; 
 p1++;
 
 this->qatt++;	// attempt

 while (rep--)
    {
     tcflush(fd, TCIOFLUSH); //Clear send & recive buffers
     rs = this->send_lk(READ_FROM_RAM, BIT_DataFrom_Structure);
     if (this->protocol==2 || this->protocol==3) usleep(300000); // 500000
     if (rs)  rs = this->read_lk(data);
     if (rs) break;
    }

 if (!rep)
    {// send problem
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>5) this->akt=0; // !!! 5 > normal quant from DB
     //if (this->qerrors==6) Events ((3<<28)|(TYPE_LK<<24)|SEND_PROBLEM,this->device);
     sprintf (query,"UPDATE device SET qatt=qatt+1,qerrors=qerrors+1,conn=0,devtim=devtim,akt=%d WHERE idd=%d",this->device,this->akt);
     res=dbase.sqlexec(query);
     if (res) mysql_free_result(res);
     usleep (100000);
     return 1;
    }
 if (debug>2) ULOGW ("[bit (0x%x)|(%x)] [%d] rs=%x (adr=%d)",this->ids_lk,this->ids_module,this->device,rs,this->adr);

 if (rs>26)
    {
     this->akt=1;
     this->qerrors=0;
     this->conn=1;
     sprintf (this->lastdate,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,currenttime->tm_mon,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);

     struct tm ttimb;
     struct tm ttimc;
     time_t tim;

     if (this->protocol==1)
	{          
         tim=data[12]+data[13]*256+data[14]*256*256+data[15]*256*256*256;
         localtime_r(&tim,&ttimb);
         if (debug>3) ULOGW ("[lk] tim(0x%x)[0x%x][0x%x][0x%x][0x%x](0x%x) (0x%x)[0x%x][0x%x][0x%x][0x%x](0x%x)",data[11],data[12],data[13],data[14],data[15],data[16],data[0],data[1],data[2],data[3],data[4],data[5]);
	 tim=data[1]+data[2]*256+data[3]*256*256+data[4]*256*256*256;
         localtime_r(&tim,&ttimc);

         sprintf (this->devtim,"%04d%02d%02d%02d%02d%02d",ttimc.tm_year+1930,ttimc.tm_mon+1,ttimc.tm_mday,ttimc.tm_hour,ttimc.tm_min,ttimc.tm_sec);

         h=*(float*)(data+16);      		// entalpy
         hs=*(float*)(data+20);      		// summ entalpy
         ta=((DOUBLE)data[25]*256+(DOUBLE)data[24])/100;	// average temperature
         tl=((DOUBLE)data[27]*256+(DOUBLE)data[26])/100;	// last temperature

         if (debug>3) ULOGW ("[bit 0x%x] [h=%.1f, hs=%.1f, ta=%.2f, tl=%.2f]",this->device,h,hs,ta,tl);
    
	 if (h>0.0) h=h/(3600); else { Events (dbase,(3<<28)|(TYPE_LK<<24)|H1_ZERO,this->device); return 1; }
         if (hs>0.0) hs=hs/(3600); else { Events (dbase,(3<<28)|(TYPE_LK<<24)|H2_ZERO,this->device); return 1; }
     	 if (ta>120 || tl>120 || hs==0) { Events (dbase,(3<<28)|(TYPE_LK<<24)|T_BIG,this->device); return 1; }

         vcc=((DOUBLE)data[29]*256+(DOUBLE)data[28])/100;	// vcc     	 
	 if (vcc<2.5) Events (dbase,(3<<28)|(TYPE_LK<<24)|VCC_WARNING,this->device);	 
	 rssi=(CHAR)data[35]/2.0-74;     
	 if (rssi<-95) Events (dbase,(3<<28)|(TYPE_LK<<24)|RSSI_WARNING,this->device);
         p2++;
         pr =(p2*100)/p1;
         if (debug>3) ULOGW ("[bit 0x%x(0x%x)] rssi[0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]",this->ids_lk,this->ids_module,data[30],data[31],data[32],data[33],data[34],data[35],data[36],data[37]);         
         if (debug>3) ULOGW ("[bit 0x%x(0x%x)] h[0x%x 0x%x 0x%x 0x%x] | hs[0x%x 0x%x 0x%x 0x%x] | ta[0x%x 0x%x] | tl[0x%x 0x%x]",this->ids_lk,this->ids_module,data[16],data[17],data[18],data[19],data[20],data[21],data[22],data[23],data[24],data[25],data[26],data[27],data[28],data[29]);	 
         if (debug>1) ULOGW ("[bit 0x%x(0x%x)][%.2f] (%02d-%02d-%04d %02d:%02d) [h=%.1f, hs=%.1f, ta=%.2f, tl=%.2f, rssi=%.2f]",this->ids_lk,this->ids_module, pr, ttimc.tm_mday,ttimc.tm_mon+1,ttimc.tm_year+1930,ttimc.tm_hour,ttimc.tm_min,h,hs,ta,tl,rssi);	 
	}
     //				       9            13            17            21
     // [00 01 01 28 02 00 00 00 00   13 40 cc 88   4c e0 c1 a7   49 6a bc a7   4c f3   0c f3  0c 3b   01  30  00  ff  18  31  33   78 0b 00 76 00]	
     if (this->protocol==2 || this->protocol==3)
	{          
//RX 57:ff 36 01 80 01 12 00 00 00 00 01 08 35 03 00 00 07 26 00 01 01 57 0b 00 00 00 00 13 c4 07 89 50 ee 1b b8 45 64 ec 97 4e 26 09 26 09 21 01 30 00 ff 7b 30 5e 00 09 00 56 4c 
//10-25 13:57:14 [lk] [rd][can-a] ff 36 1 80 1 12 0 0  0 0 1 8 35 3  0 0 7 26 0 1 1 57 b  0 0 0 0 13 0 0
//10-25 13:57:14 [bit (0x335)|(b57)] [16842764] rs=78 (adr=2891)
//10-25 13:57:14 [bit 0x335(0xb57)] status=[ff] (54|1)[0x180] str[0x0][0x0]
//10-25 13:57:14 [bit 0x101000c] (20010915232505)[h=0.0, hs=141841294950400.0, ta=0.11, tl=0.00]
//10-25 13:57:14 [bit 0x335(0xb57)] vcc[48.64] rssi[-110.50] his[0] V[68%] chip=248.32C
//10-25 13:57:14 [bit 0x335(0xb57)][100.00] (15-09-2001 23:25) [h=0.0, hs=19700178944.0, ta=0.11, tl=0.00, rssi=-110.50]
    	 //for (int i=0; i<70; i++) { ULOGW ("[lk] data-res[%d]=(0x%x)",i,data[i]); }
         if (debug>3) ULOGW ("[bit 0x%x(0x%x)] status=[%x] (%d|%d)[0x%x%x] str[0x%x][0x%x]",this->ids_lk,this->ids_module,data[0],data[1],data[2],data[4],data[3],data[7],data[8]);
	 tim=data[10]+data[11]*256+data[12]*256*256+data[13]*256*256*256;
         localtime_r(&tim,&ttimb);
         if (this->protocol==2) sprintf (this->devtim,"%04d%02d%02d%02d%02d%02d",ttimb.tm_year+1930,ttimb.tm_mon+1,ttimb.tm_mday,ttimb.tm_hour,ttimb.tm_min,ttimb.tm_sec);
         if (this->protocol==3) sprintf (this->devtim,"%04d%02d%02d%02d%02d%02d",ttimb.tm_year+1900,ttimb.tm_mon+1,ttimb.tm_mday,ttimb.tm_hour,ttimb.tm_min,ttimb.tm_sec);

         h=*(float*)(data+14);      		// entalpy
         hs=*(float*)(data+18);      		// summ entalpy
         ta=((DOUBLE)data[23]*256+(DOUBLE)data[22])/100;	// average temperature
         tl=((DOUBLE)data[25]*256+(DOUBLE)data[24])/100;	// last temperature

         if(debug>3) ULOGW ("[bit 0x%x] (%s)[h=%.1f, hs=%.1f, ta=%.2f, tl=%.2f]",this->device,this->devtim,h,hs,ta,tl);
	 if (h>0.0) h=h/(24*3600); else { if (this->req&&0x01==0) { /*Events ((3<<28)|(TYPE_LK<<24)|H1_ZERO,this->device);*/ this->req=this->req&0x1; } return 1; }
         if (hs>0.0) hs=hs/(24*3600); else { if (this->req&&0x02==0) { /*Events ((3<<28)|(TYPE_LK<<24)|H2_ZERO,this->device);*/ this->req=this->req&0x2; } return 1; }
     	 if (ta>120 || tl>120 || hs==0) { if (this->req&&0x04==0) { /*Events ((3<<28)|(TYPE_LK<<24)|T_BIG,this->device);*/ this->req=this->req&0x4; } return 1; }

         chip=((DOUBLE)data[35]*256+(DOUBLE)data[34])/100;	// chip     
         vcc=((DOUBLE)data[27]*256+(DOUBLE)data[26])/100;	// vcc     
	 p2++;
         pr =(p2*100)/p1;
	 rssi=(CHAR)data[32]/2.0-124;     
	 if (vcc<2.5) Events (dbase,(3<<28)|(TYPE_LK<<24)|VCC_WARNING,this->device);
	 if (rssi<-95) Events (dbase,(3<<28)|(TYPE_LK<<24)|RSSI_WARNING,this->device);
         if (debug>2) ULOGW ("[bit 0x%x(0x%x)] vcc[%.2f] rssi[%.2f] his[%x] V[%d%%] chip=%.2fC",this->ids_lk,this->ids_module,vcc,rssi,data[30],data[37],chip);
	 if (debug>1) ULOGW ("[bit 0x%x(0x%x)][%.2f] (%02d-%02d-%04d %02d:%02d) [h=%.1f, hs=%.1f, ta=%.2f, tl=%.2f, rssi=%.2f]",this->ids_lk,this->ids_module, pr, ttimb.tm_mday,ttimb.tm_mon+1,ttimb.tm_year+1930,ttimb.tm_hour,ttimb.tm_min,h,hs,ta,tl,rssi);
	}	 
     // 7530A

     this->req=0;	
//     select current records, if not present - create, if present - update
//     if (currenttime->tm_hour>8 && currenttime->tm_hour<20)
     if (1)
        {
         StoreData (dbase, this->device, 1, 0, 0, h, 0);
	 StoreData (dbase, this->device, 32, 0, 0, rssi, 0);
         // average temperature
	 // StoreData (this->device, 3, 0, 0, ta);
         // last temperature
	 if (tl<10000) StoreData (dbase, this->device, 4, 0, 0, tl, 0);
         if (hs<100000000) StoreData (dbase, this->device, 31, 0, 0,  hs, 0);
	} 

     if (currenttime->tm_hour>10)
        {
	 time_t tim;
	 struct tm tt;
         tim=time(&tim);
	 tim-=3600*24;
         localtime_r(&tim,&tt);
         sprintf (dat,"%04d%02d%02d000000",tt.tm_year+1900,tt.tm_mon+1,tt.tm_mday);
	 if (h<100000000) StoreData (dbase, this->device, 1, 0, 2, 0, h, dat, 0);
	 if (hs<100000000) StoreData (dbase, this->device, 31, 0, 2, 0, hs, dat, 0);
	}
     sprintf (query,"UPDATE device SET qatt=qatt+1,conn=1,devtim=%s,lastdate=NULL,qerrors=0 WHERE idd=%d",this->devtim,this->device);
     res=dbase.sqlexec(query); 
     if (debug>3) ULOGW ("[lk] (%s)[%d]",query,res);  if (res) mysql_free_result(res); 
    }
 else	
    {
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>6) 
        {
         sprintf (query,"UPDATE device SET qatt=qatt+1,conn=0,qerrors=qerrors+1 WHERE idd=%d",this->device);
         res=dbase.sqlexec(query); 
         if (debug>3) ULOGW ("[lk] (%s)[%d]",query,res);  if (res) mysql_free_result(res); 
	}
     if (this->qerrors==6) Events (dbase,(3<<28)|(TYPE_LK<<24)|RECIEVE_PROBLEM,this->device);
     return 1;
    }
 return 0;
}
//---------------------------------------------------------------------------------------------------
// ReadDataCurrent - read single device connected to concentrator
// Readed data will be stored in DB
int Device2IP::ReadDataCurrent ()
{
 int 	rs;
 UINT	ids_lk,ids_module;
 UINT	cycle_counter1,cycle_counter2,counter1,counter2;
 BYTE	data[400];
 CHAR	dat[50];
 float	v1,v2,q1,q2,vcc,rssi,chip; 

 this->qatt++;	// attempt

 // its temporary!!!!!!!!!!!!!!!!!!!!!!!!!
 bit[MAX_DEVICE_BIT-1].ids_lk=this->ids_lk;
 bit[MAX_DEVICE_BIT-1].ids_module=this->ids_module;
 bit[MAX_DEVICE_BIT-1].device=this->device;
 bit[MAX_DEVICE_BIT-1].protocol=this->protocol;
 bit[MAX_DEVICE_BIT-1].reserved2=0x2;

 usleep(100000);
 tcflush(fd, TCIOFLUSH); //Clear send & recive buffers
 rs = bit[MAX_DEVICE_BIT-1].send_lk(READ_FROM_RAM,I2CH_DataFrom_Structure); 
 usleep(350000);
 if (rs)  rs = bit[MAX_DEVICE_BIT-1].read_lk(data);
 else	// send problem
    {
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>5) this->akt=0; // !!! 5 > normal quant from DB
     if (this->qerrors==6) Events (dbase,(3<<28)|(TYPE_LK<<24)|SEND_PROBLEM,this->device);
     sprintf (query,"UPDATE device SET qatt=qatt+1,qerrors=qerrors+1,conn=0,devtim=devtim WHERE idd=%d",this->device);
     res=dbase.sqlexec(query); 
     if (debug>3) ULOGW ("[lk] (%s)[%d]",query,res); 
     if (res) mysql_free_result(res);
     return 1;
    }
 if (debug>2) ULOGW ("[2ip (0x%x)|(%x)] [%d] rs=%x (adr=%d)",this->ids_lk,this->ids_module,this->device,rs,this->adr);

 if (rs>30)
    {
     this->akt=1;
     this->qerrors=0;
     this->conn=1;
     sprintf (this->lastdate,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);

     struct tm ttimb;
     struct tm ttimc;
     time_t tim;
     if (debug>4) ULOGW ("[lk] [0x%x][0x%x]",data[0],data[1]);
     if (this->protocol==1)
	{               
         tim=data[12]+data[13]*256+data[14]*256*256+data[15]*256*256*256;
         localtime_r(&tim, &ttimb);
         tim=data[1]+data[2]*256+data[3]*256*256+data[4]*256*256*256;
         localtime_r(&tim, &ttimc);
         sprintf (this->devtim,"%04d%02d%02d%02d%02d%02d",ttimb.tm_year+1930,ttimb.tm_mon+1,ttimb.tm_mday-1,ttimb.tm_hour,ttimb.tm_min,ttimb.tm_sec);
         if (debug>3) ULOGW ("[2ip] time [0x%x 0x%x 0x%x 0x%x] [0x%x 0x%x 0x%x 0x%x] %s %s",data[1],data[2],data[3],data[4],data[12],data[13],data[14],data[15],this->devtim,this->lastdate);     
         v1=*(float*)(data+24);      		// V1
         v2=*(float*)(data+28);      		// V2
         q1=*(float*)(data+32);      		// Q1
         q2=*(float*)(data+36);      		// Q2
         if (v1<0 || v1>10000000 || v2<0 || v2>10000000) return 0;
         if (q1<0 || q1>1000 || q2<0 || q2>1000) return 0;
         if (debug>3) ULOGW ("[2ip 0x%x(0x%x)] v1[0x%x 0x%x 0x%x 0x%x] v2[0x%x 0x%x 0x%x 0x%x] q1[0x%x 0x%x 0x%x 0x%x] q2[0x%x 0x%x 0x%x 0x%x]",this->ids_lk,this->ids_module,data[24],data[25],data[26],data[27],data[28],data[29],data[30],data[31],data[32],data[33],data[34],data[35],data[36],data[37],data[38],data[39]);
         vcc=((DOUBLE)data[41]*256+(DOUBLE)data[40])*5/1024;	// vcc
         rssi=(CHAR)data[47]/2.0-74;
	}
     if (this->protocol==2)
	{               
         tim=data[10]+data[11]*256+data[12]*256*256+data[13]*256*256*256;
         localtime_r(&tim, &ttimb);
         sprintf (this->devtim,"%04d%02d%02d%02d%02d%02d",ttimb.tm_year+1930,ttimb.tm_mon+1,ttimb.tm_mday-1,ttimb.tm_hour,ttimb.tm_min,ttimb.tm_sec);
         if (debug>3) ULOGW ("[2ip] time [0x%x 0x%x 0x%x 0x%x] [0x%x 0x%x 0x%x 0x%x] %s %s",data[1],data[2],data[3],data[4],data[12],data[13],data[14],data[15],this->devtim,this->lastdate);     
	 cycle_counter1=*(DWORD*)(data+14);
	 cycle_counter2=*(DWORD*)(data+18);
	 counter1=*(DWORD*)(data+22);
	 counter2=*(DWORD*)(data+26);
         v1=*(float*)(data+30);      		// V1
         v2=*(float*)(data+34);      		// V2
         q1=*(float*)(data+38);      		// Q1
         q2=*(float*)(data+42);      		// Q2
         if (v1<0 || v1>10000000 || v2<0 || v2>10000000) { if (this->req&&0x08==0) { Events (dbase,(3<<28)|(TYPE_LK<<24)|V_ERROR,this->device); this->req=this->req&0x8; } return 0; }
         if (q1<0 || q1>1000 || q2<0 || q2>1000) { if (this->req&&0x10==0) { Events (dbase,(3<<28)|(TYPE_LK<<24)|Q_ERROR,this->device); this->req=this->req&0x10; } return 0; }
         if (debug>2) ULOGW ("[2ip 0x%x(0x%x)] cc1[%d] cc2[%d] c1[%d] c2[%d]",this->ids_lk,this->ids_module,cycle_counter1,cycle_counter2,counter1,counter2);
         vcc=((DOUBLE)data[47]*256+(DOUBLE)data[46])/100;	// vcc
         rssi=(CHAR)data[53]/2.0-74;
	 chip=((DOUBLE)data[55]*256+(DOUBLE)data[54])/100;	// rssi
	 if (vcc<2.5) Events (dbase,(3<<28)|(TYPE_LK<<24)|VCC_WARNING,this->device);
	 if (rssi<-95) Events (dbase,(3<<28)|(TYPE_LK<<24)|RSSI_WARNING,this->device);

	 if (debug>2) ULOGW ("[2ip 0x%x(0x%x)] cc1[%d] cc2[%d] c1[%d] c2[%d] status=[%x][%x] vcc=%.2f chip=%.2f bat=%d%%",this->ids_lk,this->ids_module,data[42],data[43],vcc,chip,data[51]);
	}
     this->req=0;
     if (debug>0) ULOGW ("[2ip 0x%x(0x%x)] (%02d-%02d-%04d %02d:%02d)[v1=%.2f m3, v2=%.2f m3, ql=%.2f m3/s, q2=%.2f m3/s, vcc=%.2f V, rs=%.2f]",this->ids_lk,this->ids_module,ttimb.tm_mday,ttimb.tm_mon+1,ttimb.tm_year+1930,ttimb.tm_hour,ttimb.tm_min,v1,v2,q1,q2,vcc,rssi);

     // select current records, if not present - create, if present - update volume 1
     if (currenttime->tm_hour%2==0 && currenttime->tm_min<40)
     if (currenttime->tm_hour>6 && currenttime->tm_hour<22)
        { 
         StoreData (dbase, this->device, 5, 0, 0, q1, 0); // GVS
	 StoreData (dbase, this->device, 7, 0, 0, q2, 1); // HVS
	 StoreData (dbase, this->device, 6, 0, 0, v1, 0); // GVP
	 StoreData (dbase, this->device, 8, 0, 0, v2, 1); // HVP
         StoreData (dbase, this->device, 32, 0, 0, rssi, 0);
	}     
     if (currenttime->tm_min<59)
    	{
	 //StoreDataC (this->device, 6, 5, 1, 0, v1);
	 //StoreDataC (this->device, 8, 7, 1, 0, v2);
	 //StoreData (this->device, 6, 1, 0, v1, this->lastdate);
	 //StoreData (this->device, 8, 1, 0, v2, this->lastdate);
	}
     if (currenttime->tm_hour>12)
        {
	 //StoreDataC (dbase, this->device, 6, 5, 2, 0, v1);
	 //StoreDataC (dbase, this->device, 8, 7, 2, 0, v2);
         sprintf (dat,"%04d%02d%02d000000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday);
	 StoreData (dbase, this->device, 6, 2, 0, 0, v1, dat, 0);
	 StoreData (dbase, this->device, 8, 2, 0, 0, v2, dat, 0);
	}
     sprintf (query,"UPDATE device SET qerrors=0,qatt=qatt+1,conn=1,devtim=%s,lastdate=NULL WHERE idd=%d",this->devtim,this->device);
     res=dbase.sqlexec(query); 
     if (debug>3) ULOGW ("[lk] (%s)[%d]",query,res);  if (res) mysql_free_result(res); 
    }
 else	
    {
     if (debug>2) if (rs>0) ULOGW ("[lk] error in recieve bytes[%d][%x][%x][%x][%x][%x][%x][%x][%x]",rs,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>6) 
        {
         sprintf (query,"UPDATE device SET qatt=qatt+1,conn=0,qerrors=qerrors+1 WHERE idd=%d",this->device);
	 res=dbase.sqlexec(query); 
         if (debug>3) ULOGW ("[lk] (%s)[%d]",query,res);  if (res) mysql_free_result(res); 
	}
     if (this->qerrors==6) Events (dbase,(3<<28)|(TYPE_LK<<24)|RECIEVE_PROBLEM,this->device);
     return 1;
    }
 return 0;
}
//---------------------------------------------------------------------------------------------------
int DeviceMEE::ReadDataCurrent ()
{
 int 	rs;
 UINT	ids_lk,ids_module,flat;
 BYTE	data[1000];
 CHAR	dat[50];
 float	w1,w2,pw2,q,hw,hw_abs,cw,cw_abs; 
 this->qatt++;	// attempt
 // its temporary!!!!!!!!!!!!!!!!!!!!!!!!!
 bit[MAX_DEVICE_BIT-1].ids_lk=this->ids_lk;
 bit[MAX_DEVICE_BIT-1].ids_module=this->ids_module;
 bit[MAX_DEVICE_BIT-1].protocol=this->protocol;
 bit[MAX_DEVICE_BIT-1].device=this->device;
 bit[MAX_DEVICE_BIT-1].reserved2=0x3;
 
 if (this->protocol==1) rs = bit[MAX_DEVICE_BIT-1].send_lk(GET_FLAT_INFO_COUNT,Flat_Raw_Data_Structure);
 if (this->protocol==2) rs = bit[MAX_DEVICE_BIT-1].send_lk(READ_STRUCTURE,Flat_Raw_Data_Structure);
 usleep(350000);
 if (rs)  rs = bit[MAX_DEVICE_BIT-1].read_lk(data);
 else // send problem	
    {
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>5) this->akt=0; // !!! 5 > normal quant from DB
     if (this->qerrors==6) Events (dbase,(3<<28)|(TYPE_MEE<<24)|SEND_PROBLEM,this->device);     
     sprintf (query,"UPDATE device SET qatt=qatt+1,qerrors=qerrors+1,conn=0,devtim=devtim WHERE idd=%d",this->device);
     res=dbase.sqlexec(query); 
     if (debug>3) ULOGW ("[lk] (%s)[%d]",query,res);  if (res) mysql_free_result(res);      
     return 1;     
    }
 if (debug>2) ULOGW ("[mee (0x%x)|(%x)] [%d] rs=%x (adr=%d)",this->ids_lk,this->ids_module,this->device,rs,this->adr);
// for (UINT tt=0;tt<rs;tt++) ULOGW ("[lk][%d] [%02x]",tt,data[tt]);

 if (rs>15)
    {
     this->akt=1;
     this->conn=1;
     sprintf (this->lastdate,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);
     if (this->protocol==1)
	{               
         flat=data[1];
         if (flat>8 || flat<1) return 0;
         // parse input
         w1=*(float*)(data+2);   		// W1    
         w2=*(float*)(data+6);   		// W2
         q=*(float*)(data+10);   		// Q
         if (q<0 || w1<0 || w2<0 || w1>1000000 || w2>1000000)  { if (this->req&&0x20==0) { Events (dbase,(3<<28)|(TYPE_LK<<24)|W_ERROR,this->device); this->req=this->req&0x20; } return 0; }
         if (debug>3) ULOGW ("[mee] [0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15]);
         if (debug>3) ULOGW ("[mee 0x%x|0x%x] w1 [0x%x 0x%x 0x%x 0x%x] | w2 [0x%x 0x%x 0x%x 0x%x] | q [0x%x 0x%x 0x%x 0x%x]",this->ids_lk,this->ids_module,data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13]);
         if (debug>0) ULOGW ("[mee 0x%x|0x%x] [0x%x] [%d] [w1=%f kWt, w2=%f kWt, q=%f]",this->ids_lk,this->ids_module,this->device,flat,w1,w2,q);   
	}
     if (this->protocol==2)
	{               
         flat=data[2]*256+data[1];
         if (flat>8 || flat<1) return 0;
         // parse input
         w1=*(float*)(data+69);   		// W1    
         w2=*(float*)(data+73);   		// W2
         q=*(float*)(data+77);   		// Q

         hw=*(float*)(data+81);   		//
         hw_abs=*(float*)(data+85);   		//
         cw=*(float*)(data+89);   		//
         cw_abs=*(float*)(data+93);   		//
	 
         if (q<0 || w1<0 || w2<0 || w1>1000000 || w2>1000000) { if (this->req&&0x20==0) { Events (dbase,(3<<28)|(TYPE_LK<<24)|W_ERROR,this->device); this->req=this->req&0x20; } return 0; }
         if (debug>3) ULOGW ("[mee] [0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]",data[69],data[70],data[71],data[72],data[73],data[74],data[75],data[76],data[77],data[78],data[79],data[80],data[81],data[82],data[83],data[84],data[85]);
         if (debug>2) ULOGW ("[mee 0x%x|0x%x] w1 [0x%x 0x%x 0x%x 0x%x] | w2 [0x%x 0x%x 0x%x 0x%x] | q [0x%x 0x%x 0x%x 0x%x]",this->ids_lk,this->ids_module,data[69],data[70],data[71],data[72],data[73],data[74],data[75],data[76],data[77],data[78],data[79],data[80],data[81],data[82],data[83],data[84],data[85]);
         if (debug>0) ULOGW ("[mee 0x%x|0x%x] [0x%x] [%d] [w1=%f kWt, w2=%f kWt] [q=%.3f, hw=%.3f, hw_abs=%.3f, cw=%.3f, cw_abs=%.3f]",this->ids_lk,this->ids_module,this->device,flat,w1,w2,q,hw,hw_abs,cw,cw_abs);   
	}	
     this->req=0;
     // select current records, if not present - create, if present - update  
     if (currenttime->tm_min>30)
        { 
         StoreData (dbase, this->device, 14, 0, 0, w1, 0);
         StoreData (dbase, this->device, 2, 0, 0, w2, 1);
	}
     //StoreDataC (dbase, this->device, 2, 14, 1, 0, w2);
     sprintf (dat,"%04d%02d%02d000000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday);
     StoreData (dbase, this->device, 2, 1, 0, 0, w2, dat, 0);
     
     if (currenttime->tm_hour==14)
        {
	 //StoreDataC (dbase, this->device, 2, 14, 2, 0, w2);
         sprintf (dat,"%04d%02d%02d000000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday);
	 StoreData (dbase, this->device, 2, 2, 0, 0, w2, dat, 0);
	}
     sprintf (query,"UPDATE device SET qerrors=0,qatt=qatt+1,conn=1,devtim=NULL,lastdate=NULL WHERE idd=%d",this->device);
     res=dbase.sqlexec(query); 
     if (debug>3) ULOGW ("[lk] (%s)[%d]",query,res);  if (res) mysql_free_result(res); 
    }
 else	
    {
     if (debug>2) if (rs) ULOGW ("[lk] bytes[%d][%x][%x][%x][%x][%x][%x][%x][%x]",rs,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>6) 
        {
         sprintf (query,"UPDATE device SET qatt=qatt+1,conn=0 WHERE idd=%d",this->device);
         res=dbase.sqlexec(query); 
	 if (debug>3) ULOGW ("[lk] (%s)[%d]",query,res);  
         if (res) mysql_free_result(res); 
         if (debug>3) ULOGW ("[lk] Events=0x%x",(3<<28)&TYPE_MEE<<24&RECIEVE_PROBLEM); 
	}
     if (this->qerrors==6) Events (dbase,(3<<28)|(TYPE_MEE<<24)|RECIEVE_PROBLEM,this->device);
     return 1;     
    }
 return 0;
}

//---------------------------------------------------------------------------------------------------
// ReadDataCurrent - read all devices connected to current concentrator
// run followed function ReadDataCurrent
int DeviceLK::ReadDataCurrent ()
{
 UINT qerrors=0,rs=0;
 BYTE	data[1000];
 // update RTC
 //this->protocol=4;

 if (this->protocol==1) rs = this->send_lk(UPDATE_RTC); 
 if (this->protocol==2 || this->protocol==3)
    {
     struct tm ttimb;
     time_t tim;
     //if (currenttime->tm_hour==0) rs = this->send_lk(SYNC_RTC);

     rs = this->send_lk(OPERATION_LK_TEST);
     if (this->protocol==2 || this->protocol==3) usleep (300000);
     if (rs) rs = this->read_lk(data);
     tcsetattr (fd,TCSAFLUSH,&tio);
     if (rs)
        {
         tim=data[21]+data[22]*256+data[23]*256*256+data[24]*256*256*256;
         localtime_r(&tim, &ttimb);
         sprintf (this->devtim,"%04d%02d%02d%02d%02d%02d",ttimb.tm_year+1900,ttimb.tm_mon+1,ttimb.tm_mday-1,ttimb.tm_hour,ttimb.tm_min,ttimb.tm_sec);
         if (debug>2) ULOGW ("[lk] time [%02d:%02d:%02d %02d-%02d-%d] version=%d",ttimb.tm_hour,ttimb.tm_min,ttimb.tm_sec,ttimb.tm_mday-1,ttimb.tm_mon+1,ttimb.tm_year+1900,data[20]);
	}
    // if (debug>2) ULOGW ("[lk] time was here");
    }

 if (this->protocol==1)
 if (!rs)
    {
     sprintf (query,"UPDATE device SET qatt=qatt+1,conn=0 WHERE idd=%d",this->device);
     res=dbase.sqlexec(query); 
     if (res) mysql_free_result(res); 

     tcgetattr(fd, &tio);
     if (debug>2) ULOGW ("[lk 0x%x] setbaudrate 57600",this->adr);
     cfsetospeed(&tio, baudrate(57600));
     tcsetattr(fd, TCSANOW, &tio);
     tcflush (fd,TCIFLUSH);
     
     rs = this->send_lk(SET_UART_BAUD_RATE);
     if (rs) rs = this->read_lk(data);
     sleep (1);     
     rs = this->send_lk(SET_UART_BAUD_RATE);
     if (rs) rs = this->read_lk(data);
     
     tcgetattr(fd, &tio);
     if (debug>2) ULOGW ("[lk 0x%x] setbaudrate 19200",this->adr);
     cfsetospeed(&tio, baudrate(19200));
     tcflush (fd,TCIFLUSH);

     tcsetattr(fd, TCSANOW, &tio);
     tcsetattr (fd,TCSAFLUSH,&tio);
    }
 else
    {
     sprintf (query,"UPDATE device SET devtim=NULL,lastdate=NULL,conn=%d WHERE idd=%d",this->device,rs);
     res=dbase.sqlexec(query); 
     if (debug>3) ULOGW ("[lk] (%s)[%d]",query,res);  if (res) mysql_free_result(res); 
    }

 // select all sensors with adress lk equivalent
 for (int i=0; i<mee_num;i++)
    if (mee[i].ids_lk==this->adr)
	{
	 //if (debug>2) ULOGW ("[lk] mee[%d/%d].ReadDataCurrent ()",i,mee_num);
	 //if (mee[i].akt==1 || currenttime->tm_sec%5==0)
	 qerrors+=mee[i].ReadDataCurrent ();
	 if (!WorkRegim) break;
	 usleep(70000);
	 if (!dk.pth[TYPE_LK]) return 0;	 
	}	


 // select all sensors with adress lk equivalent
 for (int i=0; i<bit_num;i++)
    {
//     if (bit[i].ids_lk==0x304 || bit[i].ids_lk==0x243 || bit[i].ids_lk==0x23c || bit[i].ids_lk==0x23a)
     if (bit[i].ids_lk==this->adr)
	{
	 if (debug>2) ULOGW ("[lk] bit[%d/%d].ReadDataCurrent (%d) [%d]",i,bit_num,this->device,this->adr);
	 //if (bit[i].akt==1 || currenttime->tm_sec%5==0)
	 qerrors+=bit[i].ReadDataCurrent ();
	 //if (qerrors) bit[i].ReadDataCurrent ();
	 if (!WorkRegim) break;	 	
	 usleep(70000);
	 if (!dk.pth[TYPE_LK]) return 0;	 
	}
    }
 // select all sensors with adress lk equivalent
 for (int i=0; i<ip2_num;i++)
    if (ip2[i].ids_lk==this->adr)	// ip2
	{
	 //if (debug>2) ULOGW ("[lk] 2ip[%d/%d].ReadDataCurrent ()",i,ip2_num);
	 //if (ip2[i].akt==1 || currenttime->tm_sec%5==0)	    
	 qerrors+=ip2[i].ReadDataCurrent ();
	 if (!WorkRegim) break;
	 usleep(70000);
	 if (!dk.pth[TYPE_LK]) return 0;
	}

 if (qerrors!=bit_num+ip2_num+mee_num) rs=1; else rs=0;
 sprintf (query,"UPDATE device SET qatt=qatt+%d,qerrors=qerrors+%d,conn=%d,devtim=devtim WHERE idd=%d",bit_num+ip2_num+mee_num,qerrors,rs,this->device);
 res=dbase.sqlexec(query); if (res) mysql_free_result(res); 
	
 
}
//---------------------------------------------------------------------------------------------------
int DeviceBIT::read_lk (BYTE* dat)
{
 UINT	crc=0;		//(* CRC checksum *)
 INT	nbytes = 0; 	//(* number of bytes in recieve packet *)
 INT	bytes = 0; 	//(* number of bytes in packet *)
 INT	data_len = 0; 	//(* number of bytes in data *)
 BYTE	data[1000]={0};	//(* recieve sequence *)
 BYTE	bf;
 UINT	i=0;		//(* current position *)
 UCHAR	ok=0xFF;	//(* flajochek *)
 CHAR	op=0;

 ioctl (fd,FIONREAD,&nbytes);
// if (debug>2) ULOGW ("[lk][%d] (total=%d)",nbytes,totalfound);
 bytes=read (fd, &data, nbytes);
// if (debug>2) ULOGW ("[lk][%d][%d]",bytes,nbytes);
// if (bytes>10) ULOGW ("[lk][!!!!!!!!!!]",bytes);

// for (i=0; i<nbytes; i++) { bytes=read (fd, &bf, 1); usleep (1000); data[i]=bf; /*ULOGW ("[lk] data[%d]=(0x%x)[%d]",i,data[i],bytes);*/ }
// for (i=0; i<nbytes; i++) { ULOGW ("[lk] data[%d]=(0x%x)[%c]",i,data[i],data[i]); }

 if (debug>3) if (bytes>1) ULOGW ("[lk][%d] [%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]",bytes,
 data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],
 data[16],data[17],data[18],data[19],data[20],data[21],data[22],data[23],data[24],data[25],data[26],data[27],data[28],data[29],data[30],
 data[31],data[32],data[33],data[34],data[35],data[36],data[37],data[38]);
 i=0; 
// for (i=0; i<nbytes; i++) { ULOGW ("[lk] data[%d]=(0x%x)[%d]",i,data[i],bytes);  }

// usleep (10000);
// ioctl (fd,FIONREAD,&bytes);  
// if (bytes>0) bytes=read (fd, &data+nbytes, 70);
// nbytes+=bytes; 

 if (dk.emul) // simulate ids>1
    { 
     //srandom (time(NULL));
     if (this->reserved2==0x1) { nbytes=44; data[9]=0x21; }
     if (this->reserved2==0x2) { nbytes=60; data[9]=0x31; }
     if (this->reserved2==0x3) { nbytes=20; data[9]=0xA; }
     data[0]=0xFF; data[1]=0x00; data[2]=0x00; data[3]=this->ids_lk/256; data[4]=this->ids_lk&0xFF;
     data[5]=0x8c;
     if (this->reserved2==0x3) data[5]=0x80+GET_FLAT_INFO_COUNT; 
     data[6]=0x00; data[7]=0x00; 
     data[8]=Crc8(data,8);
     //data[9]=0x21;	// lenght
     data[10]=0x00;	// error
     // wrong but near to true
     DWORD dw=(currenttime->tm_year-100)*365*24*3600+(currenttime->tm_mon)*31*24*3600+(currenttime->tm_mday)*24*3600+(currenttime->tm_hour)*3600+(currenttime->tm_min)*60+(currenttime->tm_sec);
     data[11]=dw&0xff; data[12]=(dw&0xff00)>>8; data[13]=(dw&0xff0000)>>16; data[14]=(dw&0xff000000)>>24;
     data[15]=0xFF; data[16]=this->ids_lk&0xFF; data[17]=this->ids_lk/256; 
     data[16]=this->ids_lk&0xFF; data[17]=this->ids_lk/256; 
     data[18]=this->ids_module&0xFF; data[19]=this->ids_module/256; 
     data[20]=0x48; data[21]=0x14;
     dw=(currenttime->tm_year-100)*365*24*3600+(currenttime->tm_mon)*732*3600+(currenttime->tm_mday)*24*3600+(currenttime->tm_hour-5)*3600+(currenttime->tm_min)*60;
     data[22]=dw&0xff; data[23]=(dw&0xff00)>>8; data[24]=(dw&0xff0000)>>16; data[25]=(dw&0xff000000)>>24;
     if (this->reserved2==0x1)
        {
	 fnum[0].f=12398+98*((DOUBLE)(random())/RAND_MAX)-10*((DOUBLE)(random())/RAND_MAX)-88*((this->flat_number%72)/9);
	 data[26]=fnum[0].c[0]; data[27]=fnum[0].c[1]; data[28]=fnum[0].c[2]; data[29]=fnum[0].c[3]; 
         fnum[1].f=70+10*((DOUBLE)(random())/RAND_MAX); 
         data[30]=(WORD)(fnum[1].f*100) & 0xff; data[31]=(WORD)(fnum[1].f*100)/256;
	 fnum[2].f=70+10*((DOUBLE)(random())/RAND_MAX);
	 data[32]=(WORD)(fnum[2].f*100) & 0xff; data[33]=(WORD)(fnum[2].f*100)/256;
         fnum[3].f=2+0.5*((DOUBLE)(random())/RAND_MAX);
         data[34]=(WORD)(fnum[3].f*100) & 0xff; data[35]=(WORD)(fnum[3].f*100)/256;
	 data[36]=0x30; data[37]=0x00; data[38]=0x01; data[39]=0x7F; data[40]=0x43; data[41]=0x00; data[42]=0x34;
         data[43]=Crc8(data+9,42-8);
        }
     if (this->reserved2==0x2)
        {
         data[29]=20+abs(30*((random())/RAND_MAX)); data[28]=0; data[27]=0; data[26]=0;
         data[33]=20+abs(30*((random())/RAND_MAX)); data[32]=0; data[31]=0; data[30]=0;
         fnum[0].f=1000*((DOUBLE)(random())/RAND_MAX)+(float)time(NULL)/300000;
         data[34]=fnum[0].c[0]; data[35]=fnum[0].c[1]; data[36]=fnum[0].c[2]; data[37]=fnum[0].c[3];
         fnum[0].f=3000*((DOUBLE)(random())/RAND_MAX)+(float)time(NULL)/100000; 
         data[38]=fnum[0].c[0]; data[39]=fnum[0].c[1]; data[40]=fnum[0].c[2]; data[41]=fnum[0].c[3]; 
         fnum[0].f=0.5+1*((DOUBLE)(random())/RAND_MAX);
         data[42]=fnum[0].c[0]; data[43]=fnum[0].c[1]; data[44]=fnum[0].c[2]; data[45]=fnum[0].c[3]; 	 
         fnum[0].f=1.5+1*((DOUBLE)(random())/RAND_MAX); 
         data[46]=fnum[0].c[0]; data[47]=fnum[0].c[1]; data[48]=fnum[0].c[2]; data[49]=fnum[0].c[3]; 
         fnum[0].f=2+0.5*((DOUBLE)(random())/RAND_MAX);	
         data[50]=(WORD)(fnum[0].f*5/1023) & 0xff; data[51]=(WORD)(fnum[0].f*5/1023)>>8;
         data[52]=0x30; data[53]=0x00; data[54]=0x01; data[55]=0x7F; data[56]=0x43; data[57]=0x00; data[58]=0x34;
	 data[59]=Crc8(data+9,59-8);
        }
     if (this->reserved2==0x3)
        {
         data[11]=88;
         fnum[0].f=100+12*((DOUBLE)(random())/RAND_MAX);
         data[12]=fnum[0].c[0]; data[13]=fnum[0].c[1]; data[14]=fnum[0].c[2]; data[15]=fnum[0].c[3];	
         fnum[0].f=100+(float)time(NULL)/10000+100*((DOUBLE)(random())/RAND_MAX); 
         data[16]=fnum[0].c[0]; data[17]=fnum[0].c[1]; data[18]=fnum[0].c[2]; data[19]=fnum[0].c[3]; 
	 data[20]=Crc8(data+9,20-8);
        }
    }

 if (this->protocol==3)
    {
     if (nbytes>11)
        {
	 BYTE 	res[120],pos=0,cur=0;
	 CHAR	temp[5];
	 totalfound++;
         //if (debug>2) ULOGW ("[lk] [rd][can] %c %c %c %c %c %c ",data[0],data[1],data[2],data[3],data[4],data[5]);
         for (i=0; i<nbytes; i++)
	    {
	     if (data[i]==0x65 && data[i+6]==0x31)
		{
		 data_len=data[i+9]-0x30;
		 //ULOGW ("[lk] %d",data_len);
		 for (int j=0; j<data_len-1; j++)
		    {
		     if (data[i+12+j*2]==0xd || data[i+12+j*2+1]==0xd) break;
		     if (data[i+12+j*2]==0xe) break;
		     sprintf (temp,"%c%c",data[i+12+j*2],data[i+12+j*2+1]);
		     //ULOGW ("[lk] [%d] %s",j,temp);
    		     sscanf (temp,"%x",&res[pos]);
		     pos++;
		    }
		} 
	    }
         //if (debug>2) ULOGW ("[lk] [rd][can-a] %x %x %x %x %x %x %x %x  %x %x %x %x %x %x %x %x  %x %x %x %x %x %x %x %x  %x %x %x %x %x %x %x %x",res[0],res[1],res[2],res[3],res[4],res[5],res[6],res[7],res[8],res[9],res[10],res[11],res[12],res[13],res[14],res[15],res[16],res[17],res[18],res[19],res[20],res[21],res[22],res[23],res[24],res[25],res[26],res[27],res[28],res[29],res[30],res[31]);	    
	 memcpy (dat,res+18,pos);
	}
     else
	{
	 //set_can_speed (sped);
	 //usleep (50000);
	 read_status_7530 (0);
	 //read_status_7530 (1);
	 //sped++;
	 //if (sped>8) sped=0;
	}
    }

// if (debug>2) ULOGW ("[lk] read (%d)",nbytes); 
// FF,FF,01,80,01,13,00,00,00,00,01,08,10,01,00,00,
// 07,EF, 00, 01,01,94,02,00,00,00,00,23,27,00,00,B4,62,7D,96,D4,E7,8D,38,1D,FB,E8,40,2F,23,71,CA,A8,7F,92,DE,F7,BA,E1,DF,BF,0B,6D,DA,F7,EB,B3,F8,ED,63,A8,09,5F,69,07,3F,D4,F7,D8,1F,39,16,11,C4,D0,55,70,F8,7C,94,24,70,72,B6,85,80,5F,F7,E8,B9,65,52,5F,62,61,8F,00,57,C4,CD,80,17,DE,8D,DA,CF,32,6B,B3,51,77,18,3C,02,BD,B1,8C,13,43,2B,F4,4B,3D,4B,3F,D2,F4,E4,55,F2,5B,F3,2F,A0,91,9B,0A,42,B0,4E,18,D2,D1,43,44,D6,10,A8,89,A2,F8,DC,59,02,88,DA,5E,4B,4C,F0,3E,43,9A,2A,1E,82,21,EB,70,B2,38,10,0E,9B,51,92,7E,D5,13,64,14,53,51,95,7F,37,D0,BB,79,EE,FC,E9,7C,3F,E2,CD,5D,76,E7,82,0B,88,DA,01,84,32,A3,A6,10,74,71,E1,2B,9F,F2,E0,75,87,7E,E7,B9,61,FF,F4,E4,AC,85,D9,47,EE,E0,EE,39,C2,E3,92,93,DC,82,B8,05,CE,FA,32,14,17,42,22,4C,4B,70,89,6B Длина - 258 байт.
 if (this->protocol==2)
    {
     if (data[0]==0xff) 
        {
	 bytes=data[1];
         FS_dst.ManIdCode=data[4];
	 FS_dst.DevTypeFunc=data[5];
         FS_dst.ID=*(DWORD*)(data+6);
         FS_src.ManIdCode=data[10];
         FS_src.DevTypeFunc=data[11];
         FS_src.ID=*(DWORD*)(data+12);
         op=data[16];
         data_len=data[17];
	 if (debug>2) ULOGW ("[lk] 0xff len[%d] to [%d|%d (%d)] from data[%d|%d (%d)] op[%d] len_data[%d]",bytes,FS_dst.ManIdCode, FS_dst.DevTypeFunc,FS_dst.ID,FS_src.ManIdCode,FS_src.DevTypeFunc, FS_src.ID, op, data_len);	 
         crc=Crc8(data,bytes+2);
	 //if (debug>2) ULOGW ("[lk] crc=[%x] [%x][%x][%x][%x] data[%d]=%x[%x %x %x][%x %x %x]",crc,Crc8(data,bytes-2),Crc8(data,bytes-1),Crc8(data,bytes+1),Crc8(data,bytes+2),bytes,data[bytes],data[bytes+2],data[bytes+1],data[bytes-1],data[bytes-2],data[bytes+3]);
	 if (debug>2) ULOGW ("[lk] crc=[%x] data[%d]=%x",crc,bytes,data[bytes+2]);
	 if (crc!=data[bytes+2]) Events (dbase,(3<<28)|(TYPE_LK<<24)|CRC_ERROR,this->device);
	 //if (data[18]!=0) Events ((3<<28)|(TYPE_LK<<24)|data[18],this->device);
         if (bytes>0) if (debug>3) ULOGW ("[lk] memcpy (%x,%x,%d)",dat,data+18,data_len);
	 if (bytes>0) memcpy (dat,data+18,data_len);  	 
	} 
    }
    
 if (this->protocol==1)
 if (nbytes>0)
    {
     while (i<nbytes)
        {
	 ok=0xFF;
	 if (debug>3) ULOGW ("[lk] data[%d]=(0x%x)",i,data[i]); 
	 switch (i)
	    {
		case 0:	if (data[i]==0xFF) ok=1; // (* its preambule *)
			break;
		case 1:	if (data[i]==0) ok=0; else ok=0; 	// (* !!compare adress here *)
			break;
		case 2: if (data[i]==0) ok=0; else ok=0; 	// (* !!compare adress here *)
			break;
		case 3:	if (data[i]==(this->ids_module/0x100)) ok=0; else ok=3; 	// (* high address not compare *)
			break;
		case 4: if (data[i]==(this->ids_module&0xFF)) ok=0; else ok=3; 	// (* low address not compare*)
			break;
		case 5:	if (data[i]>0x80) { ok=0; op=data[i]-0x80; data[i]=op;} else ok=4; // (* strange operation *)
			break;
		case 6: 
		case 7: ok=0; break;		// (* reserved *)
		case 8: crc=Crc8(data,8); if (crc==data[i]) ok=0; else ok=5; // (* compare cs *)
			break;
		case 9: bytes=data[i]; break;
	    }	
	if (i>0xA)
	    {
		if (bytes>0x40) break;
	        ok=0;
		switch (op)
		    {
		     case 0x1:	if (i==0xA+0xA) // (* before crc sum all bytes *)
		    		    {
				     crc=Crc8(data, 10);
    				     if (crc==data[i]) ok=0; else ok=6;
				    }
				break;
		     case 0x02: // (* LOAD_TO_RAM *)
		     case 0x03: // (* START_INITIALIZE_CYCLE *)
		     case 0x11: // (* START_DEINITIALIZE_CYCLE *)
		     case 0x18: // (* CLEAR_INIT_DEINIT_FRAME *)
		     case 0x19:	// (* UPDATE_RTC *)
				if (data[i] > 0) ok=7;
				break;
    		     case 0x04:	if (i==0xA+1 && data[i]>0) ok=7;	//(* READ_INITIALIZE_CYCLE_STATUS *)
				if (i==0x16+3 && data[i]>0) ok=8;
				break;
		     case 0xc:	//(* READ_FROM_RAM *)(* READ_IDENTIFICATION *)(* GET_NUM_VISIBLE *)(* READ_VISIBILITY_STAT *)
				if (i==0xA+1 && data[i]>0) ok=7;
				if (bytes>1)
				if (i==0xA+bytes)
				    {
				     crc=Crc8(data+8,bytes+2);
				     if (debug>3) ULOGW ("[lk] crc=[%x] data[%d]=%x(%x|%x)",crc,i,data[i]);
				     if (crc==data[i]) 
				        {
					 ok=0;
					 memcpy (dat,data+0xa,bytes);
					}
				     else ok=6;				     
				    }
				break;
		     case 0x12:	if (i==0xA+1 && data[i]>0) ok=7; //(* READ_DEINITIALIZE_CYCLE_STATUS *)
				if (i==0xA+3 && data[i]>0) ok=9;
				break;
    		     case 0x1c:	if (i==0xA+1 && data[i]>0) ok=7;	//(* READ_INITIALIZE_CYCLE_STATUS *)
				if (i==0x16+3 && data[i]>0) ok=8;
				break;
		     case 0x21:	if (i==0xA+1 && data[i]>0) ok=7; //(* GET_FLAT_INFO_COUNT *)
				if (i==0xA+bytes)
				    {
				     crc=Crc8(data+8,bytes+2);
				     if (debug>3) ULOGW ("[lk] crc=[%x] data[%d]=%x(%x|%x)",crc,i,data[i]);				     				     				     
				     if (crc==data[i]) 
				        {
					 ok=0;
					 //if (debug>2) ULOGW ("[lk] memcpy (%x,%x,%d)",dat,data+0xa,bytes); 
					 memcpy (dat,data+0xa,bytes);
					}
				     else ok=6;				     
				    }
				break;								
		    }
	    }
	i++;
	if (ok>1)
	    {
	     switch (ok)
	        {
		 case 0x0:	// (* no problems *)				
		 case 0x1:	// (* no preambule *)				
		 case 0x2:	// (* wrong reciever address *)
		 case 0x3:	// (* wrong sensor address *)
		 case 0x4: 	// (* wrong operation *)
		 case 0x5: 	// (* crc header error *)
		 case 0x6: 	// (* crc data error *)
		 case 0x7: 	// (* operation error *)
		 case 0x8:	// (* initialisation sensor error *)
	    	 case 0x9:	// (* deinitialisation error *)
		 break;
		}
	    }
	}
      i++;	// next byte
     if (bytes>0) if (debug>3) ULOGW ("[lk] memcpy (%x,%x,%d)",dat,data+0xa,bytes);
     if (bytes>0) memcpy (dat,data+0xa,bytes);  
    }
 return nbytes;
}
//---------------------------------------------------------------------------------------------------
bool DeviceBIT::send_lk (UINT op, UINT structure)
{
 UINT	crc=0,crc2=0;	//(* CRC checksum *)
 UINT	nbytes = 0; 	//(* number of bytes in send packet *)
 BYTE	data[100];	//(* send sequence *)
 BYTE	data2[100];	//(* send sequence *)
 UINT	service=0;	// service byte
 UINT 	len=0;
 UINT 	idslk=0,idsmodule=0;

 //this->ids_module=0;
 //this->ids_lk=0x359;
// this->protocol=2;

 if (this->protocol==3)
    {
     //     FF,14,00,00,01,08,11,01,00,00,01,12,00,00,00,00,15,04,02,B2,84,4C,30
     if (op==2) op=WRITE_STRUCTURE_BY_FACTORY;
     if (op==0xc) op=READ_STRUCTURE_BY_FACTORY;
     service=0;
//     idslk=0x345;
//     idsmodule=0xb5c;
     //this->ids_lk=0x345;
//     this->ids_lk=0x346;
     //this->ids_lk=0x333;
     //this->ids_module=0xb57;
//     this->ids_module=0xb5d;
//     this->ids_module=0xb5c;

     data[0]=0xFF;		// preamble
     data[1]=0;			// length
     data[2]=service/256;	// service byte
     data[3]=service&0xff;	// service byte
     data[4]=0x1;		// ZITC
     data[5]=0x8;		// LK
     data[6]=this->ids_lk&0xFF;	// LK id
     data[7]=this->ids_lk>>8;
//     data[6]=idslk&0xFF;	// LK id
//     data[7]=idslk>>8; 

     data[8]=0;
     data[9]=0;
     data[10]=0x1;		// ZITC
     data[11]=0x12;		// server
     data[12]=0;		// server id = 0
     data[13]=0; 
     data[14]=0;
     data[15]=0;     
     data[16]=(CHAR)op;

     switch (op)
	{
	 //(* LK_TEST *)
         case 0x1:data[1]=0x11;
		  data[17]=0x1;
		  data[18]=0x73;
		  data[19] = Crc8 (data, 19);
		  send_lk_can (this->ids_lk, data, 8);
		  send_lk_can (this->ids_lk, data+8, 8);
		  send_lk_can (this->ids_lk, data+16, 4);
		  //if (debug>2) ULOGW ("[lk] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19],data[20]);
		  break;

	 //(* OPERATION_READ_STRUCTURE *)
	 case 0x5:data[1]=0x13;		// global_lenght
		  data[17]=3;		// data_lenght
	          data[18]=this->ids_module&0xFF;
		  data[19]=this->ids_module/256;
		  data[20]=structure;
		  data[21]=Crc8 (data, 21);
		  send_long_lk_can (this->ids_lk, data, 7,1);
		  send_long_lk_can (this->ids_lk, data+7, 7,2);
		  send_long_lk_can (this->ids_lk, data+14, 7,3);
		  send_long_lk_can (this->ids_lk, data+21, 1,4);

		  //if (debug>2) ULOGW ("[lk] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19],data[20],data[21]);
		  //write (fd,&data,22);
		  break;

	 //(* WRITE_STRUCTURE_BY_FACTORY *)
         case 0x6:data[1]=38;
		  data[17]=0x26;
		  if (this->reserved2==0x1) memcpy (data+6, this->ReadConfig (), 38);
		  ip2[MAX_DEVICE_2IP-1].device=this->device;
		  if (this->reserved2==0x2) memcpy (data+6, ip2[MAX_DEVICE_2IP-1].ReadConfig (), 38);
		  write (fd,&data,18+38);
		  //if (debug>2) ULOGW ("[lk] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19],data[20]);
		  data[18+38] = Crc8 (data, 18+38);
		  break;
	 //(* READ_STRUCTURE_BY_FACTORY *)
	 case 0x7:data[1]=0x19;		// global_lenght
		  data[17]=9;		// data_lenght
		  if (this->reserved2==1 || this->reserved2==2) data[18]=0x1;	// ZITC
		  if (this->reserved2==3) data[18]=0x1;				// SPEC
		  if (this->reserved2==1) data[19]=0x1;
		  if (this->reserved2==2) data[19]=0x2;
		  if (this->reserved2==3) data[19]=0x8;
	          data[20]=this->ids_module&0xFF;
		  data[21]=this->ids_module/256;
	          //data[20]=idsmodule&0xFF;
		  //data[21]=idsmodule/256;

	          data[22]=0x0;
	          data[23]=0x0;
		  data[24]=0x0;
		  data[25]=0x0;
		  data[26]=structure;
		  //if (this->ids_lk==0x304 || this->ids_lk==0x305) data[26]=35;
		  data[27]=Crc8 (data, 27);

		  INT bytes=0, addr=0;
		  //addr=0x345;
		  //sped++;
		  send_long_lk_can (this->ids_lk, data, 7,1); usleep (15000);
		  //read_status_7530 (0);
		  //usleep (50000);
		  //ioctl (fd,FIONREAD,&bytes);
		  //nbytes=read (fd, &data, bytes);
	          //ULOGW ("[lk] [rd][can] (%d bytes) %c %c %c %c %c %c ",bytes,data[0],data[1],data[2],data[3],data[4],data[5]);
		  send_long_lk_can (this->ids_lk, data+7, 7,2); usleep (25000);
		  //read_status_7530 (0);
		  send_long_lk_can (this->ids_lk, data+14, 7,3); usleep (25000);
		  //read_status_7530 (0);
		  send_long_lk_can (this->ids_lk, data+21, 7,4); usleep (25000);
//		  tst++;
		  //read_status_7530 (0);
		  //0xff,0x19,0x0,0x0,0x1,0x8,0x10,0x1,0x0,0x0,0x1,0x12,0x0,0x0,0x0,0x0,0x7,0x9,0x1,0x1, 0x0,0x0,0x1,0x43,0x0,0x0,0x2
		  //if (debug>2) ULOGW ("[lk] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19],data[20],data[21],data[22],data[23],data[24],data[25],data[26],data[27]);
		  //for (int i=0; i<28; i++) { write (fd, &data+i, 1); usleep(1000); }
		  //write (fd,&data,28);
		  break;
	}
    }
 if (this->protocol==2)
    {     
     //     FF,14,00,00,01,08,11,01,00,00,01,12,00,00,00,00,15,04,02,B2,84,4C,30
     if (op==2) op=WRITE_STRUCTURE_BY_FACTORY;
     if (op==0xc) op=READ_STRUCTURE_BY_FACTORY;

     service=0;
     data[0]=0xFF;		// preamble
     data[1]=0;			// length
     data[2]=service/256;	// service byte
     data[3]=service&0xff;	// service byte     
     data[4]=0x1;		// ZITC
     data[5]=0x8;		// LK     
     data[6]=this->ids_lk&0xFF;	// LK id
     data[7]=this->ids_lk>>8; 
     data[8]=0;
     data[9]=0;
     data[10]=0x1;		// ZITC
     data[11]=0x13;		// server
     data[12]=0;		// server id = 0
     data[13]=0; 
     data[14]=0;
     data[15]=0;     
     data[16]=(CHAR)op;

     switch (op)
	{
	 //(* LK_TEST *)
         case 0x1:data[1]=0x11;
		  data[17]=0x1;
		  data[18]=0x73;
		  data[19] = Crc8 (data, 19);
		  write (fd,&data,20);
		  if (debug>2) ULOGW ("[lk] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19],data[20]);
		  break;

	 //(* OPERATION_READ_STRUCTURE *)
	 case 0x5:data[1]=0x13;		// global_lenght
		  data[17]=3;		// data_lenght
	          data[18]=this->ids_module&0xFF;
		  data[19]=this->ids_module/256;
		  data[20]=structure;
		  data[21]=Crc8 (data, 21);
		  if (debug>2) ULOGW ("[lk] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19],data[20],data[21]);
		  write (fd,&data,22);
		  break;		  

	 //(* WRITE_STRUCTURE_BY_FACTORY *)
         case 0x6:data[1]=38;
		  data[17]=0x26;
		  if (this->reserved2==0x1) memcpy (data+6, this->ReadConfig (), 38);
		  ip2[MAX_DEVICE_2IP-1].device=this->device;
		  if (this->reserved2==0x2) memcpy (data+6, ip2[MAX_DEVICE_2IP-1].ReadConfig (), 38);
		  write (fd,&data,18+38);
		  if (debug>2) ULOGW ("[lk] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19],data[20]);
		  data[18+38] = Crc8 (data, 18+38);
		  break;
	 //(* READ_STRUCTURE_BY_FACTORY *)
	 case 0x7:data[1]=0x19;		// global_lenght
		  data[17]=9;		// data_lenght
		  if (this->reserved2==1 || this->reserved2==2) data[18]=0x1;	// ZITC
		  if (this->reserved2==3) data[18]=0x1;				// SPEC
		  
		  if (this->reserved2==1) data[19]=0x1;
		  if (this->reserved2==2) data[19]=0x2;
		  if (this->reserved2==3) data[19]=0x8;
	          data[20]=this->ids_module&0xFF;
		  data[21]=this->ids_module/256;
	          data[22]=0x0; 
	          data[23]=0x0;
		  data[24]=0x0;
		  data[25]=0x0;
		  data[26]=structure;
		  //data[26]=35;
		  data[27]=Crc8 (data, 27);
		  //0xff,0x19,0x0,0x0,0x1,0x8,0x10,0x1,0x0,0x0,0x1,0x12,0x0,0x0,0x0,0x0,0x7,0x9,0x1,0x1, 0x0,0x0,0x1,0x43,0x0,0x0,0x2
		  if (debug>2) ULOGW ("[lk] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19],data[20],data[21],data[22],data[23],data[24],data[25],data[26],data[27]);
		  //for (int i=0; i<28; i++) { write (fd, &data+i, 1); usleep(1000); }
		  write (fd,&data,28);
		  break;		  
	}
     
    }
  if (this->protocol==1)
    {
     data[0]=0xFF;
     data[1]=this->ids_lk/256;
     data[2]=this->ids_lk&0xFF;
     data[3]=0;
     data[4]=0;
     data[5]=(CHAR)op;
     data[6]=0;
     data[7]=0;
 
     switch (op)
         {
    	  case 0x2:	nbytes=7+38; break;	//(* LOAD_TO_RAM *)
    	  case 0x3:	nbytes=4+38; break; 	//(* START_INITIALIZE_CYCLE *)
	  case 0x4:	nbytes=0; break; 	//(* READ_INITIALIZE_CYCLE_STATUS *)
	  case 0xc: 	nbytes=2; break; 	//(* READ_FROM_RAM *)
	  case 0x11:	nbytes=2; break; 	//(* START_DEINITIALIZE_CYCLE *)
	  case 0x12:	nbytes=0; break; 	//(* READ_DEINITIALIZE_CYCLE_STATUS *)
	  case 0x21:	nbytes=1; break; 	//(* GET_FLAT_INFO_COUNT *)	
	  case 0x1C:	nbytes=0; break; 	//(* SET_FLAT_CONFIG *)		
	}
     data[8] = Crc8 (data, 8);
     // if (debug>2) ULOGW ("[lk] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
     //if (debug>2) ULOGW ("[lk] write (0x%x,0x%x)(crc,nbytes)",data[8],nbytes);
     switch (op)
	{
         case 0x2: data[0]=0xFF;		//(* LOAD_TO_RAM *)		  
		  data[1]=this->ids_lk/256;	// bit[bit_num].ids_lk
		  data[2]=this->ids_lk&0xFF;	// bit[bit_num].ids_lk
		  data[3]=this->ids_module/256;	// bit[bit_num].ids_module
		  data[4]=this->ids_module&0xFF; // bit[bit_num].ids_module
		  data[5]=0x6;
		  data[6]=0x26;
		  memcpy (data+6, this->ReadConfig (), 38);
		  write (fd,&data,45);
		  if (debug>2) ULOGW ("[lk] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
		  if (debug>2) ULOGW ("[lk] write (0x%s)",data+6);
		  crc = Crc8 (data, 45);
		  break;
	 case 0x3: data[0]=0; 		//(* START_INITIALIZE_CYCLE *)
		  data[1]=1;
		  data[2]=1;
		  data[3]=0;
		  write (fd,&data,4);
		  if (this->reserved2==0x1) memcpy (data+6, this->ReadConfig (), 38);
		  ip2[MAX_DEVICE_2IP-1].device=this->device;
		  if (this->reserved2==0x2) memcpy (data+6, ip2[MAX_DEVICE_2IP-1].ReadConfig (), 38);
		  //if (this.type==2) memcpy (data+6, this.ReadLKConfig (this.device, this.type), 34);
		  write (fd,&data,42);
		  //if (this.type==2) write (fd,&data,38);
		  crc = Crc8 (data, 42);
		  //if (this.type==2) crc = crc8 (data, 38);
		  break;		  
	 case 0xc: data[9]=nbytes;
		  data[10]=this->ids_module/0x100; // (* READ_FROM_RAM *)
		  data[11]=this->ids_module&0xff;
		  data[12] = Crc8 (data+8, 4);
		  write (fd,&data,13); 
		  if (debug>2) ULOGW ("[lk] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)(0x%x)(crc)(0x%x,0x%x,0x%x,0x%x)(nb)(data)(crc)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
		  break;
	 case 0x11:data[0]=this->ids_module/0x100; // (* START_DEINITIALIZE_CYCLE *)
		  data[1]=this->ids_module&0xff;
		  write (fd,&data,2); 
		  crc = Crc8 (data, 2);
		  break;
	 case 0x1c:data[9]=nbytes;			// (* SET_FLAT_CONFIG *)
		  data[10]=this->ids_module&0xff;
		  data[11] = Crc8 (data+8, 3);
		  write (fd,&data,12);
		  if (debug>2) ULOGW ("[lk] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)(0x%x)(crc)(0x%x,0x%x,0x%x)(nb)(data)(crc)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11]);
		  break;	
	case 0x21:data[9]=nbytes;			// (* GET_FLAT_INFO_COUNT *)
		  data[10]=this->ids_module&0xff;
		  data[11] = Crc8 (data+8, 3);
		  write (fd,&data,12);
		  if (debug>2) ULOGW ("[lk] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)(0x%x)(crc)(0x%x,0x%x,0x%x)(nb)(data)(crc)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11]);
		  break;	
	}
    }
//write (fd,(VOID *)crc,1);
//if (debug>2) ULOGW ("[lk] write (0x%x) (crc)",crc);
return true;
}
//---------------------------------------------------------------------------------------------------
int DeviceLK::read_lk (BYTE* dat)
{
 BYTE	data[1000];	//(* recieve sequence *)
 UINT	i=0;		//(* current position *)
 INT	nbytes = 0; 	//(* number of bytes in recieve packet *)
 INT	bytes = 0; 	//(* number of bytes in packet *)

 ioctl (fd,FIONREAD,&nbytes);
 if (nbytes>100) nbytes=100;
 bytes=read (fd, &data, nbytes);
// for (i=0; i<bytes; i++) { ULOGW ("[lk] data[%d]=(0x%x)",i,data[i]);  }
 if (debug>2) ULOGW ("[lk][%d] [%02x %02x %02x %02x %02x]",bytes,data[0],data[1],data[2],data[3],data[4]); 
 if (bytes>0) 
    {
     memcpy (dat,data,bytes); 
     return true;
    }
 else return 0;
}

//---------------------------------------------------------------------------------------------------
bool DeviceLK::send_lk (UINT op)
{
 UINT	crc=0;		//(* CRC checksum *)
 INT	nbytes = 0; 	//(* number of bytes in send packet *)
 BYTE	data[100];	//(* send sequence *)
 struct tm ttime;
 struct tm ttimb;
 double dif;
 UINT df;
 time_t tim,tim2;

 if (this->protocol==1)
    {     
     data[0]=0xFF;
     data[1]=this->adr/256;
     data[2]=this->adr&0xFF;
     data[3]=0;
     data[4]=0;
     data[5]=(CHAR)op;
     data[6]=0;
     data[7]=0;
     switch (op)
	 {
            case 0x1: 	nbytes=0; break; //(* TEST *)
	    case 0xb:	nbytes=1; break; //(* GET_NUM_VISIBLE *)
	    case 0x13:	nbytes=0; break; //(* READ_IDENTIFICATION *)
	    case 0x14:	nbytes=0; break; //(* GET_NUM_VISIBLE *)
	    case 0x15:	nbytes=0; break; //(* READ_VISIBILITY_STAT *)
	    case 0x18:	nbytes=0; break; //(* CLEAR_INIT_DEINIT_FRAME *)
	    case 0x19:	nbytes=4; //(* UPDATE_RTC *)
	}
     data[8] = Crc8 (data, 8);
     data[9] = nbytes;
    }
 if (this->protocol==2)
    {
     UINT service=0;
     data[0]=0xFF;		// preamble
     data[1]=0;			// length
     data[2]=service/256;	// service byte
     data[3]=service&0xff;	// service byte     
     data[4]=0x1;		// ZITC
     data[5]=0x8;		// LK     
     data[6]=this->adr&0xFF;	// LK id
     data[7]=this->adr>>8; 
     data[8]=0;
     data[9]=0;
     data[10]=0x1;		// ZITC
     data[11]=0x13;		// server
     data[12]=0;		// server id = 0
     data[13]=0; 
     data[14]=0;
     data[15]=0;     
     data[16]=(CHAR)op;     
    } 
 switch (op)
    {
      case OPERATION_LK_TEST:		      		  
		  data[1]=0x11;		// global_lenght
		  data[17]=0x1;		// data_lenght
	          data[18]=0x0;
		  data[19]=Crc8 (data, 19);
		  //if (debug>2) ULOGW ("[lk] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19]);
		  //write (fd,&data,20);
    		  break;
		  
      case READ_STRUCTURE_BY_FACTORY:
		  data[1]=0x13;		// global_lenght
		  data[17]=3;		// data_lenght
	          data[20]=this->adr&0xFF;
		  data[21]=this->adr/256;
	          data[22]=0x0;
	          data[23]=0x0;
		  data[24]=0x0;
		  data[25]=0x0;
		  data[26]=LK_Data_Structure;
		  data[27]=Crc8 (data, 27);
		  if (debug>2) ULOGW ("[lk] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19],data[20],data[21],data[22],data[23],data[24],data[25],data[26],data[27]);
		  //for (int i=0; i<28; i++) { write (fd, &data+i, 1); usleep(1000); }
		  write (fd,&data,28);
    		  break;

      case OPERATION_LK_SYNC_RTC:	  
    		  data[1]=0x14;		// global_lenght
		  data[18]=4;		// data_lenght

		  tim=time(&tim);
		  localtime_r(&tim,&ttime);		  
		  ttimb.tm_year=100; ttimb.tm_mon=0; ttimb.tm_mday=1; ttimb.tm_hour=0; ttimb.tm_min=0;
		  ttimb.tm_hour=0;   ttimb.tm_sec=0; ttimb.tm_yday=0; 
		  tim2=mktime(&ttimb);
		  dif=difftime (tim,tim2);		  
		  df=(UINT)dif;
		  data[19]=df&0xff;
		  data[20]=(df&0xff00)>>8;
		  data[21]=(df&0xff0000)>>16;
		  data[22]=(df&0xff000000)>>24;
		  
		  if (debug>3) ULOGW ("[lk][time][0x%x] %u(%u|%u) [0x%x 0x%x 0x%x 0x%x]",this->adr,(UINT)dif,tim,tim2,data[10],data[11],data[12],data[13]);
		  data[23]=Crc8 (data, 23);
		  write (fd,&data,24);
    		  break;
		  
      case 0xb: data[10]=s19200;
		data[11] = Crc8 (data+8, 3);     
		write (fd,&data,12);
	        if (debug>2) ULOGW ("[lk] wr (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)(0x%x)(crc)(0x%x,0x%x,0x%x)(nb)(data)(crc)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11]);
		break;
		
      case 0x19:  tim=time(&tim);
		  localtime_r(&tim,&ttime);
		  
		  ttimb.tm_year=100;
		  ttimb.tm_mon=0;
		  ttimb.tm_mday=1;
		  ttimb.tm_hour=0;
		  ttimb.tm_min=0;
		  ttimb.tm_hour=0;
		  ttimb.tm_sec=0;
		  ttimb.tm_yday=0;
		  
		  tim2=mktime(&ttimb);
		  dif=difftime (tim,tim2);
		  df=(UINT)dif;
		  data[10]=df&0xff;
		  data[11]=(df&0xff00)>>8;
		  data[12]=(df&0xff0000)>>16;
		  data[13]=(df&0xff000000)>>24;
		  
		  if (debug>3) ULOGW ("[lk][time][0x%x] %u(%u|%u) [0x%x 0x%x 0x%x 0x%x]",this->adr,(UINT)dif,tim,tim2,data[10],data[11],data[12],data[13]);
		  data[14] = Crc8 (data+8, 6);
		  write (fd,&data,15);
		  usleep (20000);
		  
		  ioctl (fd,FIONREAD,&nbytes);
		  nbytes=read (fd, &data, 70);
		  if (debug>2) ULOGW ("[lk][time][0x%x] %d [0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]",this->adr,nbytes,data[0],data[1],data[2],data[3],data[4],data[5]);
		  
		  if (nbytes>0) return true;
		  else return false;		  
		  if (debug>2) ULOGW ("[lk][time][0x%x] %d [0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]",this->adr,nbytes,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11]);		  
		  break;
    }

if (this->protocol==3)
    {
     //this->adr=0x13A;     
     //adr=0x345;
     
     UINT service=0;
     data[0]=0xFF;		// preamble
     data[1]=0;			// length
     data[2]=service/256;	// service byte
     data[3]=service&0xff;	// service byte     
     data[4]=0x1;		// ZITC
     data[5]=0x8;		// LK     
     data[6]=this->adr&0xFF;	// LK id
     data[7]=this->adr>>8; 
//     data[6]=adr&0xFF;	// LK id
//     data[7]=adr>>8; 
     data[8]=0;
     data[9]=0;
     data[10]=0x1;		// ZITC
     data[11]=0x13;		// server
     data[12]=0;		// server id = 0
     data[13]=0; 
     data[14]=0;
     data[15]=0;     
     data[16]=(CHAR)op;      
     switch (op)
	    {
    	     case OPERATION_LK_TEST:
		  data[1]=0x11;		// global_lenght
		  data[17]=0x1;		// data_lenght
	          data[18]=0x0;
		  data[19]=Crc8 (data, 19);
		  //if (debug>2) ULOGW ("[lk] write (0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x)",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19]);
		  send_long_lk_can (adr, data, 7,1); usleep (20000);
		  send_long_lk_can (adr, data+7, 7,2); usleep (20000);
		  send_long_lk_can (adr, data+14, 6,3); usleep (20000);
		  
		  //send_long_lk_can (this->adr, data, 7,1);
		  //send_long_lk_can (this->adr, data+7, 7,2);
		  //send_long_lk_can (this->adr, data+14, 6,3);
    		  break;
	    }
     if (debug>2) ULOGW ("[lk] write complete");
    }

//write (fd,(VOID *)crc,1);
return true;
}
//---------------------------------------------------------------------------------------------------
// function read BIT config from DB
BYTE* DeviceBIT::ReadConfig ()
{
 BYTE conf[100]; // configuration
 BYTE *pconf=conf;
 // read device configuration
 sprintf (query,"SELECT * FROM dev_bit WHERE device=%d",this->device); 
 res=dbase.sqlexec(query);
 row=mysql_fetch_row(res);

/* if (mysql_real_query(mysql, query, strlen(query)))
 if (!(res=mysql_store_result(mysql)))
    {
     if (debug>1)ULOGW ("error in function store_result");
     mysql_close(mysql);
     return (0);
    }
 row=mysql_fetch_row(res);*/
 // fill class members
 this->idbit=atoi(row[0]);
 this->device=atoi(row[1]);
 this->rf_interact_interval=atoi(row[2]);
 this->ids_lk=atoi(row[3]);
 this->ids_module=0x88;
 this->measure_interval=atoi(row[5]);
 this->integral_measure_count=atoi(row[6]);
 this->pi=atof(row[7]);
 this->flat_number=atoi(row[8]);
 this->strut_number=atoi(row[9]);
 this->reserved0=atoi(row[10]);
 this->low_error_temperature=atoi(row[11]);
 this->high_error_temperature=atoi(row[12]);
 this->low_warning_temperature=atoi(row[13]);
 this->high_warning_temperature=atoi(row[14]);
 this->imitate_temperature=atoi(row[15]);
 this->pa_table=atoi(row[16]);

 // form sequence
 memcpy (conf,(VOID *)this->rf_interact_interval,4);
 memcpy (conf+4,(VOID *)this->ids_lk,2);
 memcpy (conf+6,(VOID *)this->ids_module,2);
 memcpy (conf+8,(VOID *)this->measure_interval,2);
 memcpy (conf+10,(VOID *)this->integral_measure_count,2);
 memcpy (conf+12,(VOID *)row[7],4);
 memcpy (conf+16,(VOID *)this->flat_number,2);
 memcpy (conf+18,(VOID *)this->strut_number,1);
 memcpy (conf+19,(VOID *)this->reserved0,1);
 memcpy (conf+20,(VOID *)this->low_error_temperature,2);
 memcpy (conf+22,(VOID *)this->high_error_temperature,2);
 memcpy (conf+24,(VOID *)this->low_warning_temperature,2);
 memcpy (conf+26,(VOID *)this->high_warning_temperature,2);
 memcpy (conf+28,(VOID *)this->imitate_temperature,2);
 memcpy (conf+30,(VOID *)this->pa_table,1);
 
 if (res) mysql_free_result(res);  
 sprintf (query,"SELECT * FROM device WHERE idd=%d",this->device); 
 res=dbase.sqlexec(query);
 row=mysql_fetch_row(res);
 this->ids_module=atoi(row[7]);
 
 if (res) mysql_free_result(res);  
 return pconf;
}
//---------------------------------------------------------------------------------------------------
// function read 2IP config from DB
BYTE* Device2IP::ReadConfig ()
{
 BYTE conf[100]; // configuration
 BYTE *pconf=conf;
 // read device configuration
 sprintf (query,"SELECT * FROM dev_2ip WHERE device=%d",this->device); 
 res=dbase.sqlexec(query);
 row=mysql_fetch_row(res);
 // fill class members
 this->id2ip=atoi(row[0]);
 this->device=atoi(row[1]);
 this->rf_interact_interval=atoi(row[2]);
 this->ids_lk=atoi(row[3]);
 this->ids_module=atoi(row[4]);
 this->measure_interval=atoi(row[5]);
 this->flat_number=atoi(row[6]);
 this->imp_weight1=atoi(row[7]);
 this->imp_weight2=atoi(row[8]);
 this->warn_exp1=atof(row[9]);
 this->warn_exp2=atof(row[10]);
 this->imit_exp1=atof(row[11]);
 this->imit_exp2=atof(row[12]);
 this->pa_table=atoi(row[13]);

 // form sequence
 memcpy (conf,(VOID *)this->rf_interact_interval,4);
 memcpy (conf+4,(VOID *)this->ids_lk,2);
 memcpy (conf+6,(VOID *)this->ids_module,2);
 memcpy (conf+8,(VOID *)this->measure_interval,2);
 memcpy (conf+10,(VOID *)this->flat_number,2);
 memcpy (conf+12,(VOID *)this->imp_weight1,2);
 memcpy (conf+14,(VOID *)this->imp_weight2,2);
 memcpy (conf+16,(VOID *)row[9],4);
 memcpy (conf+20,(VOID *)row[10],4);
 memcpy (conf+24,(VOID *)row[11],4);
 memcpy (conf+28,(VOID *)row[12],4);
 memcpy (conf+32,(VOID *)this->pa_table,2);

/* fnum[0].f=atof(row[7]);
 sprintf (conf,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
			  atoi(row[2])&0xff,atoi(row[2])/256,atoi(row[2])/256*256,0,
			  atoi(row[3])&0xff,atoi(row[3])/256,
			  atoi(row[4])&0xff,atoi(row[4])/256,
			  atoi(row[5])&0xff,atoi(row[5])/256,
			  atoi(row[6])&0xff,atoi(row[6])/256,
			  fnum[0].c[0],fnum[0].c[1],fnum[0].c[2],fnum[0].c[3],
			  atoi(row[8])&0xff,atoi(row[8])/256,
			  atoi(row[9]),
			  0,
			  atoi(row[10])&0xff,atoi(row[10])/256,
			  atoi(row[11])&0xff,atoi(row[11])/256,
			  atoi(row[12])&0xff,atoi(row[12])/256,
			  atoi(row[13])&0xff,atoi(row[13])/256,
			  atoi(row[14])&0xff,atoi(row[14])/256,
			  atoi(row[15]));*/
 if (res) mysql_free_result(res); 
 return pconf;
}


//---------------------------------------------------------------------------------------------------
// function read LK config from DB
// store it to class members and form sequence to send in device
BYTE* DeviceLK::ReadConfig ()
{
 BYTE conf[1000]; // configuration
 BYTE *pconf=conf;
 // read device configuration with ident <device>
 sprintf (query,"SELECT * FROM dev_lk WHERE device=%d",this->device);
 res=dbase.sqlexec(query);
 row=mysql_fetch_row(res);
 // fill class members
 this->adr=atoi(row[1]);
 memcpy(this->sensors,row[2],400);
 this->level=atoi(row[3]);
 memcpy(this->flats,row[4],200);
 memcpy(this->struts,row[5],200);
 this->napr=atoi(row[6]);
 // form sequence
 memcpy (conf,(void *)this->adr,4);
 memcpy (conf+4,this->sensors,400);
 memcpy (conf+404,(void *)this->level,2);
 memcpy (conf+406,this->flats,200);
 memcpy (conf+606,this->struts,200);
 memcpy (conf+806,(void *)this->adr,4);
 
 if (res) mysql_free_result(res); 
 return pconf;
}

//--------------------------------------------------------------------------------------
// load all configuration from DB
BOOL DeviceLK::LoadLKConfig()
{
 // load from db all lk devices
 res=dbase.sqlexec("SELECT * FROM dev_lk");
 if (res)
    {
     UINT nr=mysql_num_rows(res);     
     for (UINT r=0;r<nr;r++)
        {
	 row=mysql_fetch_row(res);
	 lk[lk_num].iddev=atoi(row[0]);
	 lk[lk_num].device=atoi(row[1]);
	 for (UINT d=0;d<device_num;d++)
	    {
	     if (dev[d].idd==lk[lk_num].device)
	        {
		 lk[lk_num].SV=dev[d].SV;
		 lk[lk_num].interface=dev[d].interface;
		 lk[lk_num].protocol=dev[d].protocol;
		 lk[lk_num].port=dev[d].port;
		 lk[lk_num].speed=dev[d].speed;
		 lk[lk_num].adr=dev[d].adr;
		 lk[lk_num].type=dev[d].type;
		 strcpy(lk[lk_num].number,dev[d].number);
		 lk[lk_num].flat=dev[d].flat;
		 lk[lk_num].akt=dev[d].akt;
		 strcpy(lk[lk_num].lastdate,dev[d].lastdate);
		 lk[lk_num].qatt=dev[d].qatt;
		 lk[lk_num].qerrors=dev[d].qerrors;
		 lk[lk_num].conn=dev[d].conn;
		 strcpy(lk[lk_num].devtim,dev[d].devtim);
		 lk[lk_num].chng=dev[d].chng;
		 lk[lk_num].req=dev[d].req;
		 lk[lk_num].source=dev[d].source;
		 strcpy(lk[lk_num].name,dev[d].name);
		 break;
		}
	    }
	 lk[lk_num].adr=atoi(row[2]);
	 memcpy(lk[lk_num].sensors,row[3],400);
	 lk[lk_num].level=atoi(row[4]);
	 memcpy(lk[lk_num].sensors,row[5],200);
	 memcpy(lk[lk_num].sensors,row[6],200);
	 lk[lk_num].napr=atoi(row[7]);
	 lk_num++;
	} 
     if (debug>1) ULOGW ("[lk] total %d lk add to list",lk_num);
     mysql_free_result(res);
    }
}
//--------------------------------------------------------------------------------------
// load all BIT configuration from DB
BOOL LoadBITConfig()
{
 bit_num=0;
 res=dbase.sqlexec("SELECT * FROM dev_bit");
 if (res)
    {
     UINT nr=mysql_num_rows(res);
     for (UINT r=0;r<nr;r++)
        {
	 row=mysql_fetch_row(res);
	 // main properties
	 bit[bit_num].idbit=atoi(row[0]);
	 bit[bit_num].device=atoi(row[1]);
	 for (UINT d=0;d<device_num;d++)
	    {
	     if (dev[d].idd==bit[bit_num].device)
	        {         
	    	 bit[bit_num].iddev=dev[d].id;
	    	 bit[bit_num].SV=dev[d].SV;
    		 bit[bit_num].interface=dev[d].interface;
		 bit[bit_num].protocol=dev[d].protocol;
		 bit[bit_num].port=dev[d].port;
		 bit[bit_num].speed=dev[d].speed;
		 bit[bit_num].adr=dev[d].adr;
		 bit[bit_num].type=dev[d].type;
		 strcpy(bit[bit_num].number,dev[d].number);
		 bit[bit_num].flat=dev[d].flat;
		 bit[bit_num].akt=dev[d].akt;
		 strcpy(bit[bit_num].lastdate,dev[d].lastdate);
		 bit[bit_num].qatt=dev[d].qatt;
		 bit[bit_num].qerrors=dev[d].qerrors;
		 bit[bit_num].conn=dev[d].conn;
		 strcpy(bit[bit_num].devtim,dev[d].devtim);
		 bit[bit_num].chng=dev[d].chng;
		 bit[bit_num].req=dev[d].req;
		 bit[bit_num].source=dev[d].source;
		 strcpy(bit[bit_num].name,dev[d].name);
		}
	    }
	 bit[bit_num].rf_interact_interval=atoi(row[2]);
	 bit[bit_num].ids_lk=atoi(row[3]);
	 bit[bit_num].ids_module=atoi(row[4]);
	 bit[bit_num].measure_interval=atoi(row[5]);
	 bit[bit_num].integral_measure_count=atoi(row[6]);
	 bit[bit_num].pi=atof(row[7]);
	 bit[bit_num].flat_number=atoi(row[8]);
	 bit[bit_num].strut_number=atoi(row[9]);
	 bit[bit_num].low_error_temperature=atoi(row[10]);
	 bit[bit_num].high_error_temperature=atoi(row[11]);
	 bit[bit_num].low_warning_temperature=atoi(row[12]);
	 bit[bit_num].high_warning_temperature=atoi(row[13]);
	 bit[bit_num].imitate_temperature=atoi(row[14]);
	 bit[bit_num].pa_table=atoi(row[15]);
	 bit[bit_num].napr=atoi(row[16]);	 
	 bit[bit_num].reserved2=0x1;		// BIT
	 //if (debug>3) ULOGW ("[lk] [%d] [%d][%d-%d] dev=%d",bit_num,bit[bit_num].adr,bit[bit_num].ids_lk,bit[bit_num].ids_module,bit[bit_num].device);
	 bit_num++;
	} 
     if (debug>0) ULOGW ("[lk] total %d BIT add to list",bit_num);
     if (res) mysql_free_result(res);
    }
}
//--------------------------------------------------------------------------------------
// load all 2IP configuration from DB
BOOL Load2IPConfig()
{
 ip2_num=0;
 res=dbase.sqlexec("SELECT * FROM dev_2ip");
 if (res)
    { 
     UINT nr=mysql_num_rows(res);
     for (UINT r=0;r<nr;r++)
        {
	 row=mysql_fetch_row(res);
	 // main properties
	 ip2[ip2_num].id2ip=atoi(row[0]);
	 ip2[ip2_num].device=atoi(row[1]);
	 for (UINT d=0;d<device_num;d++)
	    {
	     if (dev[d].idd==ip2[ip2_num].device)
	        {         
	    	 ip2[ip2_num].iddev=dev[d].id;
	    	 ip2[ip2_num].SV=dev[d].SV;
    		 ip2[ip2_num].interface=dev[d].interface;
		 ip2[ip2_num].protocol=dev[d].protocol;
		 ip2[ip2_num].port=dev[d].port;
		 ip2[ip2_num].speed=dev[d].speed;
		 ip2[ip2_num].adr=dev[d].adr;
		 ip2[ip2_num].type=dev[d].type;
		 strcpy(ip2[ip2_num].number,dev[d].number);
		 ip2[ip2_num].flat=dev[d].flat;
		 ip2[ip2_num].akt=dev[d].akt;
		 strcpy(ip2[ip2_num].lastdate,dev[d].lastdate);
		 ip2[ip2_num].qatt=dev[d].qatt;
		 ip2[ip2_num].qerrors=dev[d].qerrors;
		 ip2[ip2_num].conn=dev[d].conn;
		 strcpy(ip2[ip2_num].devtim,dev[d].devtim);
		 ip2[ip2_num].chng=dev[d].chng;
		 ip2[ip2_num].req=dev[d].req;
		 ip2[ip2_num].source=dev[d].source;
		 strcpy(ip2[ip2_num].name,dev[d].name);
		}
	    }
	 ip2[ip2_num].rf_interact_interval=atoi(row[2]);
	 ip2[ip2_num].ids_lk=atoi(row[3]);
	 ip2[ip2_num].ids_module=atoi(row[4]);
	 ip2[ip2_num].measure_interval=atoi(row[5]);
	 ip2[ip2_num].flat_number=atoi(row[6]);
	 ip2[ip2_num].imp_weight1=atoi(row[7]);
	 ip2[ip2_num].imp_weight2=atoi(row[8]);
	 ip2[ip2_num].warn_exp1=atof(row[9]);
	 ip2[ip2_num].warn_exp2=atof(row[10]);
	 ip2[ip2_num].imit_exp1=atof(row[11]);
	 ip2[ip2_num].imit_exp2=atof(row[12]);
	 ip2[ip2_num].pa_table=atoi(row[13]);
	 ip2[ip2_num].reserved2=0x2;		// 2IP
//	 if (debug>3) ULOGW ("[lk] [%d] [%d][%d-%d]",ip2_num,ip2[ip2_num].adr,ip2[ip2_num].ids_lk,ip2[ip2_num].ids_module);
	 ip2_num++;
	} 
     if (debug>1) ULOGW ("[lk] total %d 2IP add to list",ip2_num);
     if (res) mysql_free_result(res);
    }
}
//--------------------------------------------------------------------------------------
// load all MEE configuration from DB
BOOL LoadMEEConfig()
{
 mee_num=0;
 res=dbase.sqlexec("SELECT * FROM dev_mee");
 if (res)
    { 
     UINT nr=mysql_num_rows(res);
     for (UINT r=0;r<nr;r++)
        {
	 row=mysql_fetch_row(res);
	 // main properties
	 mee[mee_num].idmee=atoi(row[0]);
	 mee[mee_num].device=atoi(row[1]);	 
	 for (UINT d=0;d<device_num;d++)
	    {
	     if (dev[d].idd==mee[mee_num].device)
	        {         
	    	 mee[mee_num].iddev=dev[d].id;
	    	 mee[mee_num].SV=dev[d].SV;
    		 mee[mee_num].interface=dev[d].interface;
		 mee[mee_num].protocol=dev[d].protocol;
		 mee[mee_num].port=dev[d].port;
		 mee[mee_num].speed=dev[d].speed;
		 mee[mee_num].adr=dev[d].adr;
		 mee[mee_num].type=dev[d].type;
		 strcpy(mee[mee_num].number,dev[d].number);
		 mee[mee_num].flat=dev[d].flat;
		 mee[mee_num].akt=dev[d].akt;
		 strcpy(mee[mee_num].lastdate,dev[d].lastdate);
		 mee[mee_num].qatt=dev[d].qatt;
		 mee[mee_num].qerrors=dev[d].qerrors;
		 mee[mee_num].conn=dev[d].conn;
		 strcpy(mee[mee_num].devtim,dev[d].devtim);
		 mee[mee_num].chng=dev[d].chng;
		 mee[mee_num].req=dev[d].req;
		 mee[mee_num].source=dev[d].source;
		 strcpy(mee[mee_num].name,dev[d].name);
		}
	    }
	 mee[mee_num].ids_lk=atoi(row[7]);
	 mee[mee_num].ids_module=atoi(row[2]);
	 if (debug>4) ULOGW ("[lk] [%d] [%d][%d-%d]",mee_num,mee[mee_num].adr,mee[mee_num].ids_lk,mee[mee_num].ids_module);
	 mee_num++;
	} 
     if (debug>1) ULOGW ("[lk] total %d MEE add to list",mee_num);
     if (res) mysql_free_result(res);
    }
}
//---------------------------------------------------------------------------------------------------
BOOL StoreData (UINT dv, UINT prm, UINT type, UINT status, FLOAT value, CHAR* date)
{
 CHAR dat[20],dat2[20]; 
 if (type==1) sprintf (dat,"%04d%02d%02d%02d0000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour);
 if (type==2) 
    {
     sprintf (dat,"%04d%02d%02d000000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday);
     if (prm==1)
        {
	 time_t tim;
	 struct tm tt;
         tim=time(&tim);
	 tim-=3600*24;
         localtime_r(&tim,&tt);
         sprintf (dat,"%04d%02d%02d000000",tt.tm_year+1900,tt.tm_mon+1,tt.tm_mday);
	}
    }
 if (prm>1 || type!=2 || prm==1)
    { 
     if (value<0 || value>100000000) return false;
     
     //sprintf (query,"SELECT * FROM prdata WHERE type=%d AND prm=%d AND device=%d AND date=%s",type,prm,dv,dat);
     if (type==1) sprintf (query,"SELECT * FROM hours WHERE type=%d AND prm=%d AND device=%d AND date=%s",type,prm,dv,dat);
     if (type==2) sprintf (query,"SELECT * FROM prdata WHERE type=%d AND prm=%d AND device=%d AND date=%s",type,prm,dv,dat);
     if (debug>3) ULOGW ("[lk] %s",query);
 
     res=dbase.sqlexec(query); 
     if (row=mysql_fetch_row(res))
        {
         //sprintf (query,"UPDATE prdata SET value=%f,status=%d,date=%s WHERE prm='%d' AND type=%d AND device='%d' AND date='%s'",value,status,dat,prm,type,dv,dat);
         if (res) mysql_free_result(res);  
	 return true;
	}
     else
        {
         if (type==1) sprintf (query,"INSERT INTO hours(device,prm,value,status,date,type) VALUES('%d','%d','%f','%d','%s','%d')",dv,prm,value,status,dat,type);
	 if (type==2) sprintf (query,"INSERT INTO prdata(device,prm,value,status,date,type) VALUES('%d','%d','%f','%d','%s','%d')",dv,prm,value,status,dat,type);
	}
     if (debug>2) ULOGW ("[lk] %s",query);

     if (res) mysql_free_result(res);  
     res=dbase.sqlexec(query); 
     if (res) mysql_free_result(res);  
    }
 else
    {
     float cnt=0,sum=0;
     sprintf (dat2,"%04d%02d%02d000000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday-1);
     sprintf (query,"SELECT SUM(value),COUNT(id) FROM prdata WHERE type=1 AND prm=1 AND device=%d AND date>%s AND date<%s",dv,dat2,dat);
     if (debug>2) ULOGW ("[lk] %s",query);     
     res=dbase.sqlexec(query); 
     if (row=mysql_fetch_row(res)) 
          {
	   sum=atof(row[0]);
	   cnt=atof(row[1]);
	   if (cnt>0) sum=24*(sum/cnt); else sum=0;
	   if (debug>2) ULOGW ("[lk] sum=%f cnt=%d res=%f",sum,cnt,24*(sum/cnt));
	  }
     if (res) mysql_free_result(res);  
     sprintf (query,"SELECT * FROM prdata WHERE type=%d AND prm=%d AND device=%d AND date=%s",type,prm,dv,dat);
     if (debug>3) ULOGW ("[lk] %s",query);
 
     res=dbase.sqlexec(query); 
     if (row=mysql_fetch_row(res))
        {
         //sprintf (query,"UPDATE prdata SET value=%f,status=%d,date=%s WHERE prm='%d' AND type=%d AND device='%d' AND date='%s'",sum,status,dat,prm,type,dv,dat);
	 if (debug>2) ULOGW ("[lk] %s",query);
         if (res) mysql_free_result(res);
	 return true;
	}
     else sprintf (query,"INSERT INTO prdata(device,prm,value,status,date,type) VALUES('%d','%d','%f','%d','%s','%d')",dv,prm,sum,status,dat,type);
     if (debug>2) ULOGW ("[lk] %s",query);

     if (res) mysql_free_result(res);  
     res=dbase.sqlexec(query); 
     if (res) mysql_free_result(res);  	 
    }
 return true;
}
//---------------------------------------------------------------------------------------------------
BOOL StoreDataC (UINT dv, UINT prm, UINT prm2, UINT type, UINT status, FLOAT value)
{ 
 CHAR dat[20];
 if (type==1) sprintf (dat,"%04d%02d%02d%02d0000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour);
 if (type==2) sprintf (dat,"%04d%02d%02d000000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday);  

 float pw2;
 time_t tim,tim2;
 double	dt;
 struct tm tt;
 
 if (type==2) sprintf (query,"SELECT * FROM prdata WHERE type=%d AND prm=%d AND device=%d ORDER BY date DESC",type,prm,dv);
 if (type==1) sprintf (query,"SELECT * FROM hours WHERE type=%d AND prm=%d AND device=%d ORDER BY date DESC",type,prm,dv); 
 
 if (debug>3) ULOGW ("[lk] %s",query);
 res=dbase.sqlexec(query); pw2=0;
 if (debug>3) ULOGW ("[lk] res=%d",res);
 
 if (row=mysql_fetch_row(res)) 
    {
     if (debug>3) ULOGW ("[lk] row4=%s,row5=%s",row[4],row[5]);         
     pw2=atof(row[5]);
     sprintf (query,"DELETE FROM prdata WHERE id=%d",atoi(row[0]));
     //if (debug>1) ULOGW ("[lk] %s",query);
     //res=dbase.sqlexec(query); 
     //if (res) mysql_free_result(res);  
     
     // convert current time to time_t
     tim=time(&tim);
     tim2=time(&tim2);
     // convert row time to time_t
     //if (debug>1) ULOGW ("[lk] row4=%s",row[4]);
     localtime_r(&tim2,&tt);
     
     tt.tm_year=(row[4][0]-0x30)*1000+(row[4][1]-0x30)*100+(row[4][2]-0x30)*10+(row[4][3]-0x30);
     tt.tm_mon=(row[4][5]-0x30)*10+(row[4][6]-0x30);
     tt.tm_mday=(row[4][8]-0x30)*10+(row[4][9]-0x30);
     tt.tm_hour=(row[4][11]-0x30)*10+(row[4][12]-0x30);
     tt.tm_year-=1900; tt.tm_mon-=1;
     tim2=mktime (&tt);
     //tim=mktime (&currenttime);
     
     // substract and divide on hour (3600)
     if (debug>2) ULOGW ("[lk] -> %ld %ld (%d)[%f][%f]",tim2,tim,(tim-tim2)/3600,value,pw2);
     if (value-pw2>0 && (tim-tim2)>=3500) dt=(value-pw2);
     else
        { 
	 if (res) mysql_free_result(res);
         return false;
	}
     //dt=dt/((tim-tim2)/3600);
     if (debug>2) ULOGW ("[lk] -> %ld %ld (%d)[%f][%f]",tim2,tim,(tim-tim2)/3600,dt,dt/((tim-tim2)/3600));
     if (type==1) dt=dt/((tim-tim2)/3600);
     if (type==2 && prm==31) dt=dt*2;
     if (type==2 && prm!=31) dt=dt;
     
     if (res) mysql_free_result(res);       
     // store to base in current - 1 discr
     if (type==1) sprintf (query,"SELECT * FROM hours WHERE type=%d AND prm=%d AND device=%d AND date=%s",type,prm2,dv,dat);
     if (type==2) sprintf (query,"SELECT * FROM prdata WHERE type=%d AND prm=%d AND device=%d AND date=%s",type,prm2,dv,dat);
     
     res=dbase.sqlexec(query); 
     if (debug>2) ULOGW ("[lk] %s",query);
     if (row=mysql_fetch_row(res)) 
        {
	 if (type==1) sprintf (query,"UPDATE hours SET value=%f,status=0 WHERE type=%d AND prm=%d AND device=%d AND date=%s",dt,type,prm2,dv,dat);
	 if (type==2) sprintf (query,"UPDATE prdata SET value=%f,status=0 WHERE type=%d AND prm=%d AND device=%d AND date=%s",dt,type,prm2,dv,dat);	 
         if (res) mysql_free_result(res);
	 return true;
	}
     else 
        {
	 if (type==1) sprintf (query,"INSERT INTO hours(device,prm,value,status,date,type) VALUES('%d','%d','%f','%d','%s','%d')",dv,prm2,dt,status,dat,type);
	 if (type==2) sprintf (query,"INSERT INTO prdata(device,prm,value,status,date,type) VALUES('%d','%d','%f','%d','%s','%d')",dv,prm2,dt,status,dat,type);
	}
     if (res) mysql_free_result(res);
     if (debug>2) ULOGW ("[lk] %s",query);
     res=dbase.sqlexec(query);     
    } 
 if (res) mysql_free_result(res);      
 return true;
}
//---------------------------------------------------------------------------------------------------
// function check status all lk and sensors
// connection errors
// aktivity flag after n failed attempts
BOOL	CheckStatusAll ()
{
 for (int i=0; i<bit_num;i++)
    {
     // strcpy(bit[bit_num].devtim,dev[d].devtim);
     // bit[bit_num].req=dev[d].req;
     sprintf (query,"UPDATE device SET akt=%d,lastdate=%s,qatt=%d,qerrors=%d,conn=%d,devtim=%s WHERE idd=%d",bit[i].akt,bit[i].lastdate,bit[i].qatt,bit[i].qerrors,bit[i].conn,bit[i].devtim,bit[i].device);
     res=dbase.sqlexec(query); 
     if (res) mysql_free_result(res);      
    }
}
//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
// sprintf (devp,"/dev/ttyS%d",blok);
// sprintf (devp,"/dev/ttyUSB0");
 sprintf (devp,"/dev/ttyS3");
 speed=19200;
//1152000!!!!
 if (debug>0) ULOGW("[lk] attempt open com-port %s on speed %d",devp,speed);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NONBLOCK | O_NDELAY);
// fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0)
    {
     if (debug>0) ULOGW("[lk] error open com-port %s [%s]",devp,strerror(errno));
     return false;
    }
 else if (debug>1) ULOGW("[lk] open com-port success");
 tcflush(fd, TCIOFLUSH); //Clear send & recive buffers
 tcgetattr(fd, &tio);
 cfsetospeed(&tio, baudrate(speed));
// tio.c_oflag=tio.c_lflag=tio.c_iflag=0; 
 tio.c_cflag = 0;
 tio.c_oflag = 0;
 tio.c_iflag = 0;

 tio.c_cflag &= ~(CSIZE|PARENB|PARODD|CSTOPB|CRTSCTS);
 tio.c_cflag |= CS8|CREAD|CLOCAL|baudrate(speed);
// tio.c_lflag |= ICANON;
 tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ISIG | ECHONL);
 tio.c_iflag &= ~(INLCR | IGNCR | ICRNL | INPCK | IGNPAR | PARMRK | ISTRIP | IGNBRK);
 tio.c_iflag |= IGNCR | ICRNL;
 //tio.c_iflag = IGNPAR|IGNBRK;
 tio.c_iflag &= ~(IXON | IXOFF | IXANY); 
 //tio.c_iflag |= PARMRK;
 //tio.c_oflag |= OFILL | ONLRET | ONOCR;
 tio.c_oflag &= ~(ONLCR | OCRNL);
 if (debug>1) ULOGW("[lk] c[0x%x] l[0x%x] i[0x%x] o[0x%x]",tio.c_cflag,tio.c_lflag,tio.c_iflag,tio.c_oflag); 

 //tio.c_cc[VMIN] = 0;
 //tio.c_cc[VTIME] = 10; //Time out in 10e-1 sec 
 cfsetispeed(&tio, baudrate(speed));
 //fcntl(fd, F_SETFL, FNDELAY);
 fcntl(fd, F_SETFL, 0);
 tcflush (fd,TCIFLUSH);
 tcsetattr(fd, TCSANOW, &tio);
 tcsetattr (fd,TCSAFLUSH,&tio);
 return true;
}
//---------------------------------------------------------------------------------------------------
bool set_can_speed (BYTE speed)
{
 BYTE	data2[100];	//(* send sequence *)
 INT	nbytes = 0; 	//(* number of bytes in recieve packet *)
 INT	bytes = 0; 	//(* number of bytes in packet *)

 data2[0]='P'; data2[1]='1'; data2[2]=0x30+speed; data2[3]=0xd;
 if (debug>2) ULOGW ("[lk] [can speed %d] %c%c%c 0x%x",speed,data2[0],data2[1],data2[2],data2[3]);
 write (fd,&data2,4);
 usleep (40000);
 ioctl (fd,FIONREAD,&nbytes);
 bytes=read (fd, &data2, nbytes);
 if (debug>2) ULOGW ("[lk] [can ret %d] 0x%x 0x%x 0x%x",nbytes,data2[0],data2[1],data2[2]);
}
//---------------------------------------------------------------------------------------------------
bool send_lk_can (UINT idlk, BYTE* data, BYTE lent)
{
 BYTE	data2[100];	//(* send sequence *)
 UCHAR	crc=0;

 data2[0]='t';
 sprintf ((char *)data2+1,"%01x",idlk>>8);
 sprintf ((char *)data2+2,"%02x",idlk&0xFF);
 data2[4]='8';
 for (int ff=0; ff<lent; ff++)
    {
     sprintf ((char *)data2+5+ff*2,"%x",(data[ff]&0xf0)>>4);
     sprintf ((char *)data2+5+ff*2+1,"%x",(data[ff]&0xf));
    }
 crc=Crc7530 (data,5+lent*2);
 //sprintf ((char *)data2+6+lent*2,"%x",(crc&0xf0)>>4);
 //sprintf ((char *)data2+6+lent*2+1,"%x",(crc&0xf));
 data2[5+lent*2]=0xd;
 for (int ff=0;ff<8+lent*2;ff++) ULOGW ("[lk] [wr][can] %x(%c)",data2[ff],data2[ff]);
 if (debug>2) ULOGW ("[lk] [wr][can] %c%c%c%c%c%c %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c %c%c 0x%x",data2[0],data2[1],data2[2],data2[3],data2[4],data2[5],data2[6],data2[7],data2[8],data2[9],data2[10],data2[11],data2[12],data2[13],data2[14],data2[15],data2[16],data2[17],data2[18],data2[19],data2[20],data2[21],data2[22],data2[23],data2[24],data2[25]);
 write (fd,&data2,8+lent*2);
// data2[0]='S';
// data2[1]=0xd;
// write (fd,&data2,2);
}
//---------------------------------------------------------------------------------------------------
bool send_long_lk_can (UINT idlk, BYTE* data, BYTE lent,UINT frame)
{
 BYTE	data2[100];	//(* send sequence *)
 UCHAR	crc=0;
 UINT	adr=0;

 data2[0]='e';
 adr=0x101+idlk%254;
// if (idlk==0x304 || idlk==0x305) adr=idlk;
// adr=0x101+tst;
 sprintf ((char *)data2+1,"%08x",adr);
 if (data2[8]>='a' || data2[8]>='f') data2[8]-=0x20;
 data2[9]=0x30+lent+1;
 //data2[10]=0x80+frame;
 if (frame==1 || frame==4) sprintf ((char *)data2+10,"8");
 else sprintf ((char *)data2+10,"0");

 sprintf ((char *)data2+11,"%x",(frame&0xf));

 for (int ff=0; ff<lent; ff++)
    {
     sprintf ((char *)data2+12+ff*2,"%x",(data[ff]&0xf0)>>4);
     sprintf ((char *)data2+12+ff*2+1,"%x",(data[ff]&0xf));
     //data2[ff*2+12]='0';
     //data2[ff*2+13]='0';
    }
 //data2[5]='0'; data2[6]='0'; data2[7]='0'; data2[8]='0';
 //data2[10]='0'; data2[11]='0';
 //crc=Crc7530 (data,10+lent*2);
 data2[12+lent*2]=0xd;
 //for (int ff=0;ff<13+lent*2;ff++) ULOGW ("[lk] [wr][can] %x",data2[ff]);
// if (debug>2) ULOGW ("[lk] [wr][can][%d] %c%c%c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",13+lent*2,data2[0],data2[1],data2[2],data2[3],data2[4],data2[5],data2[6],data2[7],data2[8],data2[9],data2[10],data2[11],data2[12],data2[13],data2[14],data2[15],data2[16],data2[17],data2[18],data2[19],data2[20],data2[21],data2[22],data2[23],data2[24],data2[25]);
// for (int ff=0;ff<13+lent*2;ff++) write (fd,data2+ff,1);
// ULOGW ("[lk] [wr][can] %x",data2[ff]);
 write (fd,&data2,13+lent*2);
}
//---------------------------------------------------------------------------------------------------
bool read_status_7530 (UINT com)
{
 BYTE	data2[100];	//(* send sequence *)
 if (com==0) data2[0]='S';
 if (com==1) data2[0]='C';
 if (com==2)
    {
     data2[0]='P'; data2[1]='0';
     data2[2]='0'; data2[3]='7';
     data2[4]='3'; data2[5]='0';
     data2[6]='0'; data2[7]='0';
     data2[8]='1'; data2[9]=0xd;
     if (debug>3) ULOGW ("[7530] [wr][cmd] (%c%c%c%c%c%c%c%c%c%c)",data2[0],data2[1],data2[2],data2[3],data2[4],data2[5],data2[6],data2[7],data2[8],data2[9]);
     write (fd,data2,10);
    }
 if (com==0 || com==1) 
    {
     data2[1]=0xd;
     if (debug>3) ULOGW ("[7530] [wr][cmd] (%c%c) 0x%x 0x%x",data2[0],data2[1],data2[0],data2[1]);
     write (fd,data2,2);
    }
 usleep (250000);
 UINT	nbytes=0;
 ioctl (fd,FIONREAD,&nbytes);
 nbytes=read (fd, &data2, 70);
// if (debug>2) ULOGW ("[7530] [sts] %d [0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x] (%c%c%c%c%c%c%c%c%c%c)",nbytes,data2[0],data2[1],data2[2],data2[3],data2[4],data2[5],data2[6],data2[7],data2[8],data2[9],data2[0],data2[1],data2[2],data2[3],data2[4],data2[5],data2[6],data2[7],data2[8],data2[9]);
// if (debug>2) ULOGW ("[7530] [sts] %d [0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]",nbytes,data2[0],data2[1],data2[2],data2[3],data2[4],data2[5],data2[6],data2[7]);
}
//----------------------------------------------------------------------------------------------------
unsigned char Crc7530 (BYTE* data, UINT lent)
{
unsigned char crc=0;

while (lent--) crc = *data++;
return crc;
}
