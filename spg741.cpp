//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "db.h"
#include "func.h"
#include <fcntl.h>

#include "spg741.h"

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

extern  "C" DeviceR     	dev[MAX_DEVICE];// device class
extern  "C" DeviceSPG741  	spg741[5];
extern  "C" DeviceDK		dk;
extern	"C" BOOL	threads[30];
extern  "C" UINT	debug;


BOOL send_spg (UINT op, UINT prm, UINT frame, UINT index);
UINT read_spg (BYTE* dat, BYTE type);
static  BYTE CRC(const BYTE* const Data, const BYTE DataSize, BYTE type);
        
        VOID    ULOGW (const CHAR* string, ...);              // log function
        UINT    baudrate (UINT baud);                   // baudrate select
static  BOOL    OpenCom (SHORT blok, UINT speed);       // open com function
        VOID    Events (DWORD evnt, DWORD device);      // events 
static  BOOL    LoadSPGConfig();                      // load tekon configuration

//-----------------------------------------------------------------------------
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include <wchar.h>
#include <locale.h>
#include <math.h>

static uint16_t calc_bcc (uint8_t *ptr, int n);
static int  spg741_init();
static int  spg741_get(uint8_t, int, char *);
static int  spg741_set(uint8_t, int, char *, char *);
static int spg741_get_ident(uint8_t);

static int  spg741_parse_msg(char *, int);
static int  spg741_send_msg();

static int  spg741_h_archiv(uint8_t, int, const char *, const char *);
static int  spg741_m_archiv(uint8_t, int, const char *, const char *);
static int  spg741_d_archiv(uint8_t, int, const char *, const char *);
static int  spg741_get_string_param(uint8_t, uint16_t, char **, uint8_t, uint16_t);
static int  spg741_set_string_param(uint8_t, uint16_t, char *, uint8_t, uint16_t);

static int  generate_sequence (uint8_t devaddr, uint8_t type, uint8_t func, uint16_t padr, uint8_t nchan, uint8_t npipe, int no, struct tm* from, struct tm* to, char* sequence, uint16_t* len);
static int  analyse_sequence (char* dats, uint len, char* answer, uint8_t analyse, uint8_t id, AnsLog *alog, uint16_t padr);
static int  checkdate (uint8_t	type, int ret_fr, int ret_to, struct tm* time_from, struct tm* time_to, struct tm* time_cur, time_t *tim, time_t *sttime, time_t *fntime);
static double cIEEE754toFloat (char *Data);
//-----------------------------------------------------------------------------
void * spg741DeviceThread (void * devr)
{
 int devdk=*((int*) devr);                      // DK identificator
 dbase.sqlconn("dk","root","");                 // connect to database

 // load from db km device
 LoadSPGConfig();
 // open port for work
 BOOL rs=OpenCom (spg741[0].port, spg741[0].speed);
 if (!rs) return (0);

 while (WorkRegim)
 for (UINT d=0;d<dev_num[TYPE_SPG741];d++)
    {
     spg741_get_ident(spg741[d].adr);
     if (debug>1) ULOGW ("[741] ReadDataCurrent (%d)",d);
     spg741[d].ReadDataCurrent (); 
     if (debug>1) ULOGW ("[741] ReadDataArchive (%d)",d);
     spg741[d].ReadAllArchive (10);
     if (!dk.pth[TYPE_SPG741])
        {
	 if (debug>0) ULOGW ("[741] spg741 thread stopped");
	 threads[TYPE_SPG741]=0;
	 pthread_exit (NULL);
	 return 0;	 
	}          
    }
 dbase.sqldisconn();
 if (debug>0) ULOGW ("[741] SPG741 thread end");
 threads[TYPE_SPG741]=0;
 pthread_exit (NULL); 
}
//-----------------------------------------------------------------------------
// ReadDataCurrent - read single device. Readed data will be stored in DB
int DeviceSPG741::ReadDataCurrent ()
{
 UINT   rs;
 float  fl;
 uint8_t i=0;
 CHAR   data[400];
 CHAR   date[20];
 this->qatt++;  // attempt

 for (i=0; i<sizeof(currents741)/sizeof(currents741[0]); i++)
	{
	 spg741_get(this->adr, currents741[i].no, data);
	 StoreData (dbase, this->device, currents741[i].no, currents741[i].pipe, 0, fl, 0);
	}
 for (i=0; i<sizeof(const741)/sizeof(const741[0]); i++)
	{
	 spg741_get(this->adr, const741[i].no, data);
	 ULOGW ("[%d] %s ret=%s",const741[i].no,const741[i].name,data);
	}
 //rs=send_ce (CURRENT_W, 0, 0, 0);
 //if (rs)  rs = this->read_ce(data, 0);
 //StoreData (dbase, this->device, 14, 0, 0, fl, 0);
 return 0;
}
//-----------------------------------------------------------------------------
// ReadDataArchive - read single device. Readed data will be stored in DB
int DeviceSPG741::ReadAllArchive (UINT tp)
{
 bool   rs;
 uint16_t i=0;
 BYTE   data[400];
 CHAR   date[20];
 time_t tim;
 struct 	tm *time_from=(struct tm *)malloc (sizeof (struct tm));
 tim=time(&tim);
 tim-=3600*24*3;
 localtime_r(&tim,time_from);

 for (i=0; i<sizeof(archive741)/sizeof(archive741[0]); i++)
	{
	 sprintf (date,"%d-%d-%d,%d:00:00",time_from->tm_year, time_from->tm_mon, time_from->tm_mday, time_from->tm_hour);
	 spg741_h_archiv (this->adr, archive741[i].no, date, "");
	 ULOGW ("[%d] %s ret=%s",const741[i].no,const741[i].name,data);
	}
 tim=time(&tim);
 tim-=3600*24*15;
 localtime_r(&tim,time_from);

 for (i=0; i<sizeof(archive741)/sizeof(archive741[0]); i++)
	{
	 sprintf (date,"%d-%d-%d,00:00:00",time_from->tm_year, time_from->tm_mon, time_from->tm_mday);
	 spg741_d_archiv (this->adr, archive741[i].no, date, "");
	 ULOGW ("[%d] %s ret=%s",const741[i].no,const741[i].name,data);
	}
 tim=time(&tim);
 tim-=3600*24*180;
 localtime_r(&tim,time_from);

 for (i=0; i<sizeof(archive741)/sizeof(archive741[0]); i++)
	{
	 sprintf (date,"%d-%d-01,00:00:00",time_from->tm_year, time_from->tm_mon);
	 spg741_m_archiv (this->adr, archive741[i].no, date, "");
	 ULOGW ("[%d] %s ret=%s",const741[i].no,const741[i].name,data);
	}

 free (time_from);
 return 0;
}
//--------------------------------------------------------------------------------------
// load all km configuration from DB
BOOL LoadSPGConfig()
{
 UINT spg_num=dev_num[TYPE_SPG741];
 for (UINT d=0;d<device_num;d++)
 if (dev[d].type==TYPE_SPG741)
    {
     spg741[spg_num].iddev=dev[d].id;
     spg741[spg_num].device=dev[d].idd;
     spg741[spg_num].SV=dev[d].SV;
     spg741[spg_num].interface=dev[d].interface;
     spg741[spg_num].protocol=dev[d].protocol;
     spg741[spg_num].port=dev[d].port;
     spg741[spg_num].speed=dev[d].speed;
     spg741[spg_num].adr=dev[d].adr;
     spg741[spg_num].type=dev[d].type;
     strcpy(spg741[spg_num].number,dev[d].number);
     spg741[spg_num].flat=dev[d].flat;
     spg741[spg_num].akt=dev[d].akt;
     strcpy(spg741[spg_num].lastdate,dev[d].lastdate);
     spg741[spg_num].qatt=dev[d].qatt;
     spg741[spg_num].qerrors=dev[d].qerrors;
     spg741[spg_num].conn=dev[d].conn;
     strcpy(spg741[spg_num].devtim,dev[d].devtim);
     spg741[spg_num].chng=dev[d].chng;
     spg741[spg_num].req=dev[d].req;
     spg741[spg_num].source=dev[d].source;
     strcpy(spg741[spg_num].name,dev[d].name);
     if (debug>0) ULOGW ("[741] device [0x%x],adr=%d",spg741[spg_num].device,spg741[spg_num].adr);
     spg_num++;
    }
 if (res) mysql_free_result(res);
}
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 sprintf (devp,"/dev/ttyS%d",blok);
// sprintf (devp,"/dev/ttyS2",blok);
 if (debug>0) ULOGW("[741] attempt open com-port %s on speed %d",devp,speed);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[741] error open com-port %s [%s]",devp,strerror(errno)); 
     return false;
    }
 else if (debug>1) ULOGW("[741] open com-port success"); 

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
BOOL DeviceSPG741::send_spg (UINT op, UINT prm, UINT index, UINT frame)
    {
     return true;     
    }
//-----------------------------------------------------------------------------    
UINT  DeviceSPG741::read_spg (BYTE* dat, BYTE type)
    {
     return 0;
    }
//-----------------------------------------------------------------------------    
static uint16_t calc_bcc (uint8_t *ptr, int n)
{
 uint8_t crc = 0;
 while (n-- > 0) 
	{
	 crc = crc + (uint8_t) *ptr++;
	}
 crc=crc^0xff;
 return crc;
}

static int spg741_get_ident(uint8_t adr)
{
	static char ident_query[100];
	char 	spg_id[100],answer[100];
	int 		ret;
	uint16_t	len;
	AnsLog	alog;
	struct 	tm *time_from=(struct tm *)malloc (sizeof (struct tm));
	struct 	tm *time_to=(struct tm *)malloc (sizeof (struct tm));

	time_t tim;
	tim=time(&tim);			// default values
	localtime_r(&tim,time_from); 	// get current system time
	localtime_r(&tim,time_to); 	// get current system time

	ret = -1;
	if (-1 == generate_sequence (adr, 0, START_EXCHANGE, 0, 0, 0, 1, time_from, time_to, ident_query, &len)) goto out;
        write (fd,&ident_query,len);
        ioctl (fd,FIONREAD,&len); 
	len=read (fd, &answer, 75);
	if (-1 == analyse_sequence (answer, len, answer, ANALYSE, 0, &alog, 0)) goto out;

	if (-1 == generate_sequence (adr, 0, GET741_FLASH, 3, 0, 0, 3, time_from, time_to, ident_query, &len)) goto out;
        write (fd,&ident_query,len);
        ioctl (fd,FIONREAD,&len); 
	len=read (fd, &answer, 75);
	if (-1 == analyse_sequence (answer, len, spg_id, ANALYSE, 3, &alog, 8)) goto out;

	sprintf (spg_id,"%s",alog.data[3][0]);
	ULOGW ("\tSPG ident=%s\n", spg_id);
	ret = 0;
out:
	free (time_from);
	free (time_to);
	return ret;
}

static int
spg741_get_string_param(uint8_t adr, uint16_t addr, char *save, uint8_t type, uint16_t padr)
{
	static char ident_query[100],	answer[1024];
	int 	ret;
	uint16_t	len;
	float	temp=0;
	struct 	tm *time_from=(struct tm *)malloc (sizeof (struct tm));
	struct 	tm *time_to=(struct tm *)malloc (sizeof (struct tm));
	char	*value_string;
	AnsLog	alog;

	value_string=(char *)malloc (100);
	time_t tim;
	tim=time(&tim);			// default values
	localtime_r(&tim,time_from); 	// get current system time
	localtime_r(&tim,time_to); 	// get current system time

	ret = -1;

	if (type==TYPE_CURRENTS) {
		if (padr==0)
    		    if (-1 == generate_sequence (adr, 0, GET741_RAM, addr, 0, 1, 8, time_from, time_to, ident_query, &len)) goto out;
		if (padr>0)
    		    if (-1 == generate_sequence (adr, 0, GET741_RAM, addr, 0, 1, 4, time_from, time_to, ident_query, &len)) goto out;

	        write (fd,&ident_query,len);
    		ioctl (fd,FIONREAD,&len); 
		len=read (fd, &answer, 75);

		if (-1 == analyse_sequence (answer, len, answer, ANALYSE, 0, &alog, padr)) goto out;
		ULOGW ("RECV: %s [%s]", alog.data[0][0],alog.time[0][0]);

		if (padr>0)
			{
			 temp=cIEEE754toFloat(alog.data[0][0]);
			 ULOGW ("%f (%x %x %x %x)",temp,alog.data[0][0][0],alog.data[0][0][1],alog.data[0][0][2],alog.data[0][0][3]);
			 //snprintf (save,100,"%f",(float *)alog.data[0][0]);
			}
		else	{
			 ULOGW ("%02d-%02d-%04d %02d:%02d:%02d\n",alog.data[0][0][0],alog.data[0][0][1],alog.data[0][0][2]+2000,alog.data[0][0][3],alog.data[0][0][4],alog.data[0][0][5]);
			 snprintf (value_string,100,"%02d-%02d-%d %02d:%02d:%02d",alog.data[0][0][0],alog.data[0][0][1],alog.data[0][0][2]+2000,alog.data[0][0][3],alog.data[0][0][4],alog.data[0][0][5]);
			 save=value_string;
			}
		}
	if (type==TYPE_CONST) {
		if (-1 == generate_sequence (adr, 0, GET741_FLASH, addr, padr, 0, 8, time_from, time_to, ident_query, &len)) goto out;
	        write (fd,&ident_query,len);
	        ioctl (fd,FIONREAD,&len); 
		len=read (fd, &answer, 75);
		if (-1 == analyse_sequence (answer, len, answer, ANALYSE, addr%4, &alog, padr)) goto out;
		ULOGW ("RECV: %s [%s]", alog.data[0][0],alog.time[0][0]);
		snprintf (value_string,100,"%s",alog.data[0][0]);
		save=value_string;
		}
out:
	free (time_from);
	free (time_to);
	return ret;
}

static int
spg741_set_string_param(uint8_t adr, uint16_t addr, char *save, uint8_t type, uint16_t padr)
{
	static char ident_query[100],	answer[1024];
	int 		ret, i;
	uint16_t	len;
	struct 	tm *time_from=(struct tm *)malloc (sizeof (struct tm));
	struct 	tm *time_to=(struct tm *)malloc (sizeof (struct tm));
	AnsLog	alog;

	time_t tim;
	tim=time(&tim);			// default values
	time_from=localtime(&tim); 	// get current system time
	time_to=localtime(&tim); 	// get current system time

	if (-1 == generate_sequence (adr, 0, SET741_PARAM, addr, padr, 0, 8, time_from, time_to, ident_query, &len)) goto out;
        write (fd,&ident_query,len);
        ioctl (fd,FIONREAD,&len); 
	len=read (fd, &answer, 75);
	if (-1 == analyse_sequence (answer, len, answer, ANALYSE, 0, &alog, padr)) goto out;

    	ULOGW ("RECV: %s [%s]", alog.data[0][0],alog.time[0][0]);

	if (type==TYPE_CONST)	{
		for (i=0; i<64; i++) 
		    {
		    if (save[i]>10) ident_query[i]=save[i];
		        else
	    		for (; i<64; i++) ident_query[i]=0x20;
		    }
		ident_query[64]=0;
		}

	if (-1 == generate_sequence (adr, 1, SET741_PARAM, addr, padr, 0, 8, time_from, time_to, ident_query, &len)) goto out;
        write (fd,&ident_query,len);
        ioctl (fd,FIONREAD,&len); 
	len=read (fd, &answer, 75);
	if (-1 == analyse_sequence (answer, len, answer, ANALYSE, 0, &alog, padr)) goto out;
    	ULOGW ("RECV: %s [%s]", alog.data[0][0],alog.time[0][0]);
	snprintf (save,100,"OK");

out:
	free (time_from);
	free (time_to);
	return ret;
}

static int spg741_get(uint8_t adr, int param, char *ret)
{
	int i=0, rv=0;

	for (i=0; i<sizeof(currents741)/sizeof(currents741[0]); i++)
	if (param == currents741[i].no) {
	    rv = spg741_get_string_param(adr,currents741[i].adr, ret, TYPE_CURRENTS, currents741[i].type);
    	    return rv;
	}
	for (i=0; i<sizeof(const741)/sizeof(const741[0]); i++)
	if (param == const741[i].no)  {
    	    rv = spg741_get_string_param(adr,const741[i].adr, ret, TYPE_CONST, const741[i].type);
	    return rv;
    	}
	return -1;
}

static int
spg741_set(uint8_t adr, int param, char* ret, char* save)
{
	int i=0, rv=0;
	char	addr[10];

	for (i=0; i<sizeof(const741)/sizeof(const741[0]); i++)
	if (param == const741[i].no)  {
		sprintf (addr,"%d",i);
		rv = spg741_set_string_param(adr, const741[i].adr, ret, TYPE_CONST, const741[i].type);
		return 0;
    	}
	for (i=0; i<sizeof(currents741)/sizeof(currents741[0]); i++)
	if (param == currents741[i].no)  {
		sprintf (addr,"%d",i);
		rv = spg741_set_string_param(adr,currents741[i].adr, ret, TYPE_CURRENTS, currents741[i].type);
		return 0;
    	}

	return -1;
}

static int 
spg741_h_archiv(uint8_t adr,  int no, const char *from, const char *to)
{
	struct spg741_ctx *ctx;
	float	val=0;
	AnsLog alog;
	int ret,ret_fr,ret_to,quant=0,i;
	uint16_t len,trys;
	static char ident_query[100], answer[1024];
	time_t tim,sttime,fntime,entime;

	struct 	tm *time_from=(struct tm *)malloc (sizeof (struct tm));
	ret_fr = sscanf(from,"%d-%d-%d,%d:%d:%d",&time_from->tm_year, &time_from->tm_mon, &time_from->tm_mday, &time_from->tm_hour, &time_from->tm_min, &time_from->tm_sec);

	struct 	tm *time_to=(struct tm *)malloc (sizeof (struct tm));
	ret_to = sscanf(to,"%d-%d-%d,%d:%d:%d",&time_to->tm_year, &time_to->tm_mon, &time_to->tm_mday, &time_to->tm_hour, &time_to->tm_min, &time_to->tm_sec);

	struct 	tm *time_cur=(struct tm *)malloc (sizeof (struct tm));
	tim=time(&tim);
	localtime_r(&tim,time_cur);

	ret=checkdate (TYPE_HOURS,ret_fr,ret_to,time_from,time_to,time_cur,&tim,&sttime,&fntime);
	alog.quant_param=0;

	if (sttime && fntime && ret==0)
	while (sttime<=fntime)
		{
	         entime=sttime+3600*1;
		 localtime_r(&sttime,time_from);
		 localtime_r(&entime,time_to);

		 for (i=0; i<sizeof(archive741)/sizeof(archive741[0]); i++)	{
		    trys=5;
	    	    while (trys--)	{
			snprintf(alog.time[archive741[i].id][quant], 20,"%d-%02d-%02d,%02d:00:00",time_to->tm_year+1900, time_to->tm_mon+1, time_to->tm_mday, time_to->tm_hour);
			if (-1 == generate_sequence (adr, TYPE_HOURS, GET741_HOURS, archive741[i].adr, 0, 1, no, time_from, time_to, ident_query, &len)) break;
		        write (fd,&ident_query,len);
		        ioctl (fd,FIONREAD,&len); 
			len=read (fd, &answer, 75);
	 		if (-1 == analyse_sequence (answer, len, answer, ANALYSE, archive741[i].id, &alog, archive741[i].adr)) continue;
			val = cIEEE754toFloat((alog.data[archive741[i].id][quant]));
			ULOGW ("(%d/%d) %s (%s) [%f]\n",archive741[i].id,quant,alog.time[archive741[i].id][quant],alog.data[archive741[i].id][quant],val);
			}
		    }
		 sttime+=3600; quant++;
		}
	free (time_from);
	free (time_cur);
	free (time_to);
	return 1;
}

static int spg741_d_archiv(uint8_t adr, int no, const char *from, const char *to)
{
	struct spg741_ctx *ctx;
	float val;
	AnsLog alog;
	int ret,ret_fr,ret_to,quant=0,i;
	uint16_t len,trys;
	static char ident_query[100], answer[1024];
	time_t tim,sttime,fntime,entime;

	struct 	tm *time_from=(struct tm *)malloc (sizeof (struct tm));
	ret_fr = sscanf(from,"%d-%d-%d,%d:%d:%d",&time_from->tm_year, &time_from->tm_mon, &time_from->tm_mday, &time_from->tm_hour, &time_from->tm_min, &time_from->tm_sec);
	//time_from->tm_mon-=1; time_from->tm_year-=1900;
	//sttime=mktime (time_from);

	struct 	tm *time_to=(struct tm *)malloc (sizeof (struct tm));
	ret_to = sscanf(to,"%d-%d-%d,%d:%d:%d",&time_to->tm_year, &time_to->tm_mon, &time_to->tm_mday, &time_to->tm_hour, &time_to->tm_min, &time_to->tm_sec);
	//time_to->tm_mon-=1; time_to->tm_year-=1900;
	//fntime=mktime (time_to);

	struct 	tm *time_cur=(struct tm *)malloc (sizeof (struct tm));
	tim=time(&tim);
	localtime_r(&tim,time_cur);

	ret=checkdate (TYPE_MONTH,ret_fr,ret_to,time_from,time_to,time_cur,&tim,&sttime,&fntime);
	alog.quant_param=0;
	sttime-=3600;

	if (sttime && fntime && ret==0)
	while (sttime<=fntime)
		{
	         entime=sttime+3600*24;
		 //if (entime>fntime) entime=fntime;
		 localtime_r(&sttime,time_from);
		 localtime_r(&entime,time_to);
		 //time_from->tm_year+=1900;
		 //time_to->tm_year+=1900;
		 //time_from->tm_mon++;
		 //time_to->tm_mon++;
		 for (i=0; i<sizeof(archive741)/sizeof(archive741[0]); i++)	{
		    trys=5;
	    	    while (trys--)	{
			snprintf(alog.time[archive741[i].id][quant], 20,"%d-%02d-%02d,00:00:00",time_to->tm_year+1900, time_to->tm_mon+1, time_to->tm_mday);
			if (-1 == generate_sequence (adr, TYPE_DAYS, GET741_DAYS, archive741[i].adr, 0, 1, no, time_from, time_to, ident_query, &len)) break;

		        write (fd,&ident_query,len);
		        ioctl (fd,FIONREAD,&len); 
			len=read (fd, &answer, 75);
			if (-1 == analyse_sequence (answer, len, answer, ANALYSE, archive741[i].id, &alog, archive741[i].adr)) continue;
			val = cIEEE754toFloat((alog.data[archive741[i].id][quant]));
			fprintf (stderr,"(%d/%d) %s (%s) [%f]\n",archive741[i].id,quant,alog.time[archive741[i].id][quant],alog.data[archive741[i].id][quant],val);

			sttime+=3600*24; quant++;
			}
		    }
		}
	alog.quant_param=quant;
	free (time_from);
	free (time_cur);
	free (time_to);
	return 1;
}

static int 
spg741_m_archiv(uint8_t adr, int no, const char *from, const char *to)
{
	struct spg741_ctx *ctx;
	float val=0;
	AnsLog alog;
	int ret,ret_fr,ret_to,quant=0,i;
	uint16_t len,trys;
	static char ident_query[100], answer[1024];
	time_t tim,sttime,fntime,entime;

	struct 	tm *time_from=(struct tm *)malloc (sizeof (struct tm));
	ret_fr = sscanf(from,"%d-%d-%d,%d:%d:%d",&time_from->tm_year, &time_from->tm_mon, &time_from->tm_mday, &time_from->tm_hour, &time_from->tm_min, &time_from->tm_sec);
	struct 	tm *time_to=(struct tm *)malloc (sizeof (struct tm));
	ret_to = sscanf(to,"%d-%d-%d,%d:%d:%d",&time_to->tm_year, &time_to->tm_mon, &time_to->tm_mday, &time_to->tm_hour, &time_to->tm_min, &time_to->tm_sec);
	struct 	tm *time_cur=(struct tm *)malloc (sizeof (struct tm));
	tim=time(&tim);
	localtime_r(&tim,time_cur);

	ret=checkdate (TYPE_MONTH,ret_fr,ret_to,time_from,time_to,time_cur,&tim,&sttime,&fntime);
	alog.quant_param=0;

	if (sttime && fntime && ret==0)
	while (sttime<=fntime)
		{
	         entime=sttime+3600*24*31;
		 //if (entime>fntime) entime=fntime;
		 localtime_r(&sttime,time_from);
		 localtime_r(&entime,time_to);

		 for (i=0; i<sizeof(archive741)/sizeof(archive741[0]); i++)	{
			trys=5;
	    		while (trys--)	{
			    snprintf(alog.time[archive741[i].id][quant], 20,"%d-%02d-01,00:00:00",time_to->tm_year+1900, time_to->tm_mon+1);
			    if (-1 == generate_sequence (adr, TYPE_MONTH, GET741_MONTHS, archive741[i].adr, 0, 1, no, time_from, time_to, ident_query, &len)) break;
		            write (fd,&ident_query,len);
		            ioctl (fd,FIONREAD,&len); 
			    len=read (fd, &answer, 75);
			    if (-1 == analyse_sequence (answer, len, answer, ANALYSE, archive741[i].id, &alog, archive741[i].adr))		continue;
    			    val = cIEEE754toFloat((alog.data[archive741[i].id][quant]));
			    fprintf (stderr,"(%d/%d) %s (%s) [%f]\n",archive741[i].id,quant,alog.time[archive741[i].id][quant],alog.data[archive741[i].id][quant],val);
			}
		    }
		 sttime+=3600*24*31; quant++;
		}
	free (time_from);
	free (time_cur);
	free (time_to);
	return 1;
}


//static int  generate_sequence (struct device *dev, uint8_t type, uint8_t func, const char* padr, uint8_t nchan, uint8_t npipe, int no, struct tm* from, struct tm* to, char* sequence);
// function generate send sequence for logika
static int 
generate_sequence (uint8_t devaddr, uint8_t type, uint8_t func, uint16_t padr, uint8_t nchan, uint8_t npipe, int no, struct tm* from, struct tm* to, char* sequence, uint16_t* len)
{
 char 		buffer[150];
 uint16_t 	i=0; 
 uint8_t	ks=0, startm=0, ln;

 switch (func)
	{
	// Запрос поиска записи в часовом архиве:  0x10 NT 0x48 гг мм дд чч КС 0x16
	case 	GET741_HOURS:	sprintf (sequence,"%c%c%c%c%c%c%c",DLE,devaddr,func,to->tm_year,to->tm_mon+1,to->tm_mday,to->tm_hour);
				ln=7; startm=0;
				break;
	case 	GET741_DAYS:	sprintf (sequence,"%c%c%c%c%c%c%c",DLE,devaddr,func,to->tm_year,to->tm_mon+1,to->tm_mday,ZERO);
				ln=7; startm=0;
				break;
	case 	GET741_MONTHS:	sprintf (sequence,"%c%c%c%c%c%c%c",DLE,devaddr,func,to->tm_year,to->tm_mon+1,ZERO,ZERO);
				ln=7; startm=0;
				break;
        // Запрос чтения ОЗУ: 0x10 NT 0x52 А1 А0 КБ 0x00 КС 0x16
	case 	GET741_RAM:	sprintf (sequence,"%c%c%c%c%c%c%c",DLE,devaddr,func,(uint8_t)((padr)%256),(uint8_t)((padr)/256),no,ZERO);
				ln=7; startm=0;
				break;
	case 	SET741_PARAM:	if (type==0)	{
				    sprintf (sequence,"%c%c%c%c%c%c%c",DLE,devaddr,func,(uint8_t)((padr)%256),(uint8_t)((padr)/256),ZERO,ZERO);
				    ln=7; startm=0;
				    }
				else	{
					sprintf (buffer,"%c%c%c%s",DLE,devaddr,func,sequence);
					ln=strlen(buffer); startm=0;
					sprintf (sequence,"%s",buffer);
				    }
				break;
	// Запрос чтения FLASH-памяти: 0x10 NT 0x45 N1 N0 K 0x00 КС 0x16
	case 	GET741_FLASH:	if (npipe==88) sprintf (sequence,"%c%c%c%c%c%c%c",DLE,devaddr,func,(uint8_t)((padr/64)%256),(uint8_t)((padr/64)/256),0x1,ZERO);
				else sprintf (sequence,"%c%c%c%c%c%c%c",DLE,devaddr,func,(uint8_t)(8+padr/4),(uint8_t)(0),1,ZERO);
				ln=7; startm=0;
				break;
	case	START_EXCHANGE:	for (i=0;i<16; i++) buffer[i]=0xff; buffer[16]=0;
				sprintf (sequence,"%s%c%c%c%c%c%c%c",buffer,DLE,devaddr,func,ZERO,ZERO,ZERO,ZERO);
				ln=7+16; startm = 16;
				break;
	case	GET741_PARAM:
	default:		return	-1;
	}
 ks = calc_bcc ((uint8_t *)sequence+1+startm, ln-1-startm);
 sequence[ln]=(uint8_t)(ks);
 sequence[ln+1]=UK;
 *len=ln+2;
 return 0;
}

//----------------------------------------------------------------------------------------
static int 
analyse_sequence (char* dats, uint len, char* answer, uint8_t analyse, uint8_t id, AnsLog *alog, uint16_t padr)
{
 char 	dat[1000]; 
 uint16_t 	i=0,start=0, startm=0, cntNS=0, no=0, j=0, data_len=0;
 if (len>1000) 	return -1;
 memcpy (dat,dats,len);
 alog->checksym = 0;
 //if (analyse==ANALYSE) for (i=0; i<len; i++)   printf ("in[%d]:0x%x (%c)\n",i,(uint8_t)dats[i],(uint8_t)dats[i]);
 i=0;
 while (i<len)
	{
	 //printf ("f[%d] %x %x %x\n",i,dat[i],dat[i+1],dat[i+2]);
	 if (dat[i]==DLE && (uint8_t)dat[i+1]<20 && !start)
		{
		 alog->from = (uint8_t)dat[i+1];
		 alog->func=(uint8_t)dat[i+2];
		 startm=i+1; i=i+3; start=1;
		 //if (analyse==ANALYSE) printf ("from=%d func=%d (st=%d)\n",alog->from,alog->func,startm);
		}
	 if (dat[i]==DLE && dat[i+2]==0x21 && dat[i+3]==0x3)
		{
		 printf ("no data\n");
		 return -1;
		}
	 //if (dat[i]==UK && dat[i+1]!=UK)
	 if ((dat[i]==UK && dat[i+1]!=UK) && ((id<98) || ((id>=98) && (i-2-startm>62))))
	 //if ((!(len>60 && i<58)) && (!(len>6 && len<10 && i<7)))
	 //if (((dat[i]==UK) && (padr<0x189)) || ((dat[i]==UK) && (padr>=0x189) && (i-2-startm>62)))
		{
 		 uint8_t ks=calc_bcc ((uint8_t *)dat+startm,i-startm-1);
		 //if (analyse==ANALYSE) printf ("KS [%d->%d] %x (%x) [%d]\n",startm,i-1-1,ks,(uint8_t)dat[i-1],len);
		 data_len=i-1-3;
		 if (analyse==NO_ANALYSE)
		    {
			if ((uint8_t)ks==(uint8_t)dat[i-1])
				return 	1;
			else	return	-1;
			}
		 if (((uint8_t)ks==(uint8_t)dat[i-1]) || (i==67 && len==68)) 
			{
			 alog->checksym = 1;
		 	 if (alog->func==START_EXCHANGE)
				{
				 memcpy (alog->data[id][no],dat+startm+2,3);
				 alog->data[id][no][3]=0;
				 //strcpy (alog->type[id][no],"");
				 strcpy (alog->time[id][no],"");
				 alog->quant_param = 1;
				 printf ("[start] 1 %s [%x %x %x]\n",alog->data[id][no],alog->data[id][no][0],alog->data[id][no][1],alog->data[id][no][2]);
				}
			 if (alog->func==SET741_PARAM)
				{
				 sprintf (alog->data[0][0],"ok");
				 printf ("write enable\n");
				}
			 if (alog->func==GET741_FLASH)
				{
				 if (id<98)		// Flash (param)
					{
					 memcpy (alog->data[id][no],(dat+startm+4+2+(id%4)*16),8);
					 alog->data[id][no][8]=0;
					 //strcpy (alog->type[id][no],"");
					 strcpy (alog->time[id][no],"");
					 alog->quant_param = 1;
					 printf ("[flash] [%s]\n",alog->data[id][no]);
					}
				 if (id>=98)		// Flash (NS)
					{
					 for (j=0; j<7; j++)	{
					    memcpy (alog->data[0][j],(dat+startm+6+j*8),8);
					    alog->quant_param = cntNS;
					    //printf ("cntNS = [%d] [%x %x %x %x]\n",cntNS,alog->data[0][j][0],alog->data[0][j][1],alog->data[0][j][2],alog->data[0][j][3]);
					    cntNS++;
					    }
					}
				}
			 if (alog->func==GET741_RAM)
				{
				 memcpy (alog->data[id][no],(dat+startm+2),data_len);
				 printf ("[RAM] [%s][%d]\n",alog->data[id][no],data_len);
				 return 1;
				}
			 if (alog->func==GET741_HOURS || alog->func==GET741_DAYS || alog->func==GET741_MONTHS)
			 if (len==69 || len==68)
				{
				/*
				 for (j=0; j<12; j++)
					{
					 memcpy (alog->data[id][j],(dat+startm+2+j*4),4);
					 if ((uint8_t)alog->data[id][j][3]<0x76 || (uint8_t)alog->data[id][j][3]>0x8a) 
						{
						 alog->data[id][j][0]=0; alog->data[id][j][1]=0; alog->data[id][j][2]=0; alog->data[id][j][3]=0; alog->data[id][j][4]=0;
						}
					 alog->data[id][j][5]=0;
					}
				 alog->quant_param = 11;*/
				}
			 if (alog->func==GET741_HOURS || alog->func==GET741_DAYS || alog->func==GET741_MONTHS)
			    {
				memcpy (alog->data[id][alog->quant_param],(dat+startm+2+padr),4);
				for (j=0; j<12; j++)
				    {
				     //memcpy (alog->data[id][j],(dat+startm+2+j*4),4);
				    }
				alog->quant_param += 1;
			    }
			}
		 else 
			{
			 alog->checksym = 0;
			 for (j=0;j<len;j++) fprintf (stderr,"[%d] [%d] = 0x%x [%c]\n",padr,j,(uint8_t)dat[j],(uint8_t)dat[j]);
			 fprintf (stderr,"wrong checksum alog.checksym=%d(rec=%x,must=%x) at pos=%d\n",alog->checksym,ks,(uint8_t)dat[i-1],i);
			}
		 if (id<0x189) return 1;
		}
	 i++;
	}
 if (id>=98) return 1;
 else  return 0;
}
//----------------------------------------------------------------------------------------
double cIEEE754toFloat (char *Data)
{
	uint8_t sign;
	int	exp,j;
	double res=0,zn=0.5, tmp;
	uint8_t mask;
	uint16_t i;
	if (*(Data+2)&0x80) sign=1; else sign=0;
	exp = ((*(Data+3)&0xff))-127;
	for (j=2;j>=0;j--)
    	    {
            mask = 0x80;
	    for (i=0;i<=7;i++)
	    	{
	    	 if (j==2&&i==0) {res = res+1; mask = mask/2;}
	    	 else {
	    		res = (*(Data+j)&mask)*zn/mask + res;
	    		mask = mask/2; zn=zn/2;
	    	    }
	    	}
	    }
	//printf ("%f %d (%x %x %x %x)\n",res,exp,*(Data+3),*(Data+2),*(Data+1),*(Data+0));
	res = res * pow (2,exp);
	tmp = 1*pow (10,-15);
	if (res<tmp) res=0;
	if (sign) res = -res;
	return res;
}
//----------------------------------------------------------------------------------------
static int  checkdate (uint8_t	type, int ret_fr, int ret_to, struct tm* time_from, struct tm* time_to, struct tm* time_cur, time_t *tim, time_t *sttime, time_t *fntime)
{
	//fprintf (stderr,"%d-%02d-%02d,%02d:%02d:%02d\n",time_cur->tm_year, time_cur->tm_mon, time_cur->tm_mday, time_cur->tm_hour, time_cur->tm_min, 0);
	if (ret_fr<6) ret_fr=-1;
	if (ret_to<6) ret_to=-1;

	if (ret_fr==-1 && ret_to==6)	{
		localtime_r(tim,time_from);
		time_from->tm_year=time_cur->tm_year;
		time_from->tm_mon=time_cur->tm_mon;
		time_from->tm_mday=time_cur->tm_mday;
		time_from->tm_hour=time_cur->tm_hour;
		time_to->tm_year-=1900;
		time_to->tm_mon-=1;
		*sttime=mktime (time_from);
		*fntime=mktime (time_to);
		if (type==TYPE_HOURS) *sttime-=3600*24*45;
		if (type==TYPE_DAYS) *sttime-=3600*24*365;
		if (type==TYPE_MONTH) *sttime-=3600*24*365*2;
		if (type==TYPE_EVENTS) *sttime-=3600*24*365;
    		//fprintf (stderr,"[1]%ld - %ld - %ld\n",*sttime,*fntime,*tim);
    		return 0;
		}
	if (ret_fr==6 && ret_to==-1)	{
		localtime_r(tim,time_to);
		time_to->tm_year=time_cur->tm_year;
		time_to->tm_mon=time_cur->tm_mon;
		time_to->tm_mday=time_cur->tm_mday;
		time_to->tm_hour=time_cur->tm_hour;
		time_from->tm_year-=1900;
		time_from->tm_mon-=1;
		*sttime=mktime (time_from);
		*fntime=mktime (time_to);
    		//fprintf (stderr,"[2]%ld - %ld - %ld\n",*sttime,*fntime,*tim);
    		return 0;
		}
	if (ret_fr==-1 && ret_to==-1)	{
		localtime_r(tim,time_from);
		localtime_r(tim,time_to);
		time_from->tm_year=time_cur->tm_year;
		time_from->tm_mon=time_cur->tm_mon;
		time_from->tm_mday=time_cur->tm_mday;
		time_from->tm_hour=time_cur->tm_hour;
		time_to->tm_year=time_cur->tm_year;
		time_to->tm_mon=time_cur->tm_mon;
		time_to->tm_mday=time_cur->tm_mday;
		time_to->tm_hour=time_cur->tm_hour;
		*sttime=mktime (time_from);
		*fntime=mktime (time_to);
		if (type==TYPE_HOURS) *sttime-=3600*24*45;
		if (type==TYPE_DAYS) *sttime-=3600*24*365;
		if (type==TYPE_MONTH) *sttime-=3600*24*365*2;
		if (type==TYPE_EVENTS) *sttime-=3600*24*365;
    		//fprintf (stderr,"[3]%ld - %ld - %ld\n",*sttime,*fntime,*tim);
		return 0;
		}
	if (ret_fr==6 && ret_to==6)	{
		time_to->tm_year-=1900;
		time_from->tm_year-=1900;
		time_to->tm_mon-=1;
		time_from->tm_mon-=1;
		*sttime=mktime (time_from);
		*fntime=mktime (time_to);
		//fprintf (stderr,"[4]%ld - %ld - %ld\n",*sttime,*fntime,*tim);
		return 0;
		}
	return -1;
}
