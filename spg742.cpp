//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "spg742.h"
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
static  CHAR 	n_pack=0;

extern  "C" UINT device_num;    // total device quant
extern  "C" UINT spg742_num;	// total device quant
extern  "C" BOOL WorkRegim;     // work regim flag
extern  "C" tm 	*currenttime;    // current system time

extern  "C" DeviceR     dev[MAX_DEVICE];
extern  "C" DeviceSPG742   spg742[MAX_DEVICE_742];
extern  "C" DeviceDK	dk;
extern  "C" UINT	debug;
extern	"C" BOOL	threads[30];

static  union fnm fnum[5];
static  UINT    chan_num[MAX_DEVICE_742]={0};

//        BYTE    CRC(const BYTE* const Data, const BYTE DataSize);
        VOID    ULOGW (const CHAR* string, ...);              // log function
	UINT    baudrate (UINT baud);                   // baudrate select
static  BOOL    OpenCom (SHORT blok, UINT speed);       // open com function
//        VOID    Events (DWORD evnt, DWORD device);      // events 
static  BOOL    Load742Config();                      // load tekon configuration
static  BOOL    StoreData (UINT dv, UINT prm, UINT pipe, UINT status, FLOAT value);             // store data to DB
static  BOOL    StoreData (UINT dv, UINT prm, UINT pipe, UINT type, UINT status, FLOAT value, CHAR* data);

static int	spg742_get_ident(int adr);
static int 	generate_sequence (int adr, uint8_t type, uint8_t func, uint16_t padr, uint8_t nchan, uint8_t npipe, int no, struct tm* from, struct tm* to, char* sequence, uint16_t* len);
static int 	analyse_sequence (char* dats, uint len, char* answer, uint8_t analyse, uint8_t id, AnsLog *alog, uint16_t padr);
static int  	checkdate (uint8_t	type, int ret_fr, int ret_to, struct tm* time_from, struct tm* time_to, struct tm* time_cur, time_t *tim, time_t *sttime, time_t *fntime);
static int 	spg742_h_archiv (int device, int adr, int no, const char *from, const char *to);
static int 	spg742_d_archiv (int device, int adr, int no, const char *from, const char *to);
static int	spg742_get_string_param(int adr, uint16_t addr, char *save, uint8_t type, uint16_t padr);
//-----------------------------------------------------------------------------
void * spg742DeviceThread (void * devr)
{
 int devdk=*((int*) devr);                // DK identificator
 dbase.sqlconn("","root","");             // connect to database
 ULOGW ("[742] LoadSPGConfig()");
 
 Load742Config();
 BOOL rs=OpenCom (spg742[0].port, spg742[0].speed);
 if (!rs) return (0);

 for (UINT d=0;d<spg742_num;d++)
    {
     ULOGW ("[742] tekon[%d].ReadAllArchive",d);
     spg742[d].ReadAllArchive (50);
     if (!dk.pth_spg742)
        {
	 if (debug>0) ULOGW ("[742] spg742 thread stopped");
	 threads[TYPE_SPG742]=FALSE;	// we are finished
	 pthread_exit (NULL);
	 return 0;
	}
    }

 while (WorkRegim)
 for (UINT d=0;d<spg742_num;d++)
    {
     if (debug>3) ULOGW ("[spg] ReadDataCurrent");
     spg742[d].ReadDataCurrent ();
     sleep (5);
     if (debug>2) ULOGW ("[spg] ReadDataArchive ()");
     spg742[d].ReadAllArchive (1);

     if (!dk.pth_tek)
        {
	 if (debug>0) ULOGW ("[742] epg742 thread stopped");
	 threads[TYPE_SPG742]=FALSE;	// we are finished
	 pthread_exit (NULL);
	 return 0;
	}
    }
 dbase.sqldisconn();
 threads[TYPE_SPG742]=FALSE;	// we are finished
 pthread_exit (NULL); 
}
//-----------------------------------------------------------------------------
// ReadDataCurrent - read single device. Readed data will be stored in DB
int DeviceSPG742::ReadDataCurrent ()
{
 UINT   rs,i;
 int	rv;
 float  fl;
 BYTE   data[400];
 CHAR   date[20],ret[50];
 this->qatt++;  // attempt
 
 spg742_get_ident(this->adr);

 for (i=0; i<sizeof(currents742)/sizeof(currents742[0]); i++)
	{
	    rv = spg742_get_string_param(this->adr, currents742[i].adr, ret, TYPE_CURRENTS, currents742[i].type);
    	    sprintf (this->lastdate,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,1+currenttime->tm_mon,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);
    	    fl=*(float*)(ret);
    	    if (debug>3) ULOGW ("[742] [%d] [%f]",i,fl);
    	    if (fl) StoreData (this->device, currents742[i].no, currents742[i].no, 0, fl);
	}
 
 for (i=0; i<sizeof(const742)/sizeof(const742[0]); i++)
	{
    	    rv = spg742_get_string_param(this->adr, const742[i].adr, ret, TYPE_CONST, const742[i].type);
	    if (rv<0)	{
	         this->qerrors++;
	         this->conn=0;
	         if (this->qerrors>15) this->akt=0; // !!! 5 > normal quant from DB
	         if (debug>2) ULOGW ("[742] Events[%d]",((3<<28)|(TYPE_SPG742<<24)|SEND_PROBLEM));
	         if (this->qerrors==16) Events (dbase,(3<<28)|(TYPE_SPG742<<24)|SEND_PROBLEM,this->device);
		}
	    else	{ 
    		this->akt=1;
    		this->qerrors=0;
    		this->conn=1;
    		sprintf (this->lastdate,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,1+currenttime->tm_mon,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);
    		fl=*(float*)(ret);
    		if (debug>3) ULOGW ("[742] [%d] [%f]",i,fl);
    		if (fl) StoreData (this->device, currents742[i].no, currents742[i].no, 0, fl);
		}
	}
 return 0;
}
//-----------------------------------------------------------------------------
// ReadDataArchive - read single device. Readed data will be stored in DB
int DeviceSPG742::ReadAllArchive (UINT tp)
{
 bool   rs;
 BYTE   data[400];
 CHAR   date[20];
 UINT	month,year,index;
 float  value;
 char 	from[20],to[20];
 UINT   code,vsk=0;
 time_t tims;
 
 this->qatt++;  // attempt

 struct tm *prevtime=(tm *)malloc (sizeof *prevtime);
 struct	tm *time_from=(tm *)malloc (sizeof *time_from);

 tims=time(&tims);
 localtime_r(&tims,prevtime);
 sprintf (to,"%d-%d-%d,%d:%d:%d",&prevtime->tm_year, &prevtime->tm_mon, &prevtime->tm_mday, &prevtime->tm_hour, &prevtime->tm_min, &prevtime->tm_sec);
 tims-=3600*tp;
 localtime_r(&tims,prevtime);
 sprintf (from,"%d-%d-%d,%d:%d:%d",&time_from->tm_year, &time_from->tm_mon, &time_from->tm_mday, &time_from->tm_hour, &time_from->tm_min, &time_from->tm_sec);
 spg742_h_archiv(this->idspg,this->adr, tp, from, to);
 //spg742_h_archiv(this->idspg,device->adr, int no, const char *from, const char *to);
 //spg742_h_archiv(this->idspg,device->adr, int no, const char *from, const char *to);     
 free (prevtime);
 free (time_from);
 return 0;
}
//--------------------------------------------------------------------------------------
// load all tekon configuration from DB
BOOL Load742Config()
{
 spg742_num=0;
 for (UINT d=0;d<device_num;d++)
 if (dev[d].type==TYPE_SPG742)
    {
     sprintf (query,"SELECT * FROM dev_spg WHERE idd=%d",dev[d].idd);
     res=dbase.sqlexec(query); 
     UINT nr=mysql_num_rows(res);
     for (UINT r=0;r<nr;r++)
	{
         row=mysql_fetch_row(res);
	 spg742[spg742_num].idspg=spg742_num;
         if (!r)
	    {
    	     spg742[spg742_num].iddev=dev[d].id;
             spg742[spg742_num].device=dev[d].idd;
             spg742[spg742_num].SV=dev[d].SV;
             spg742[spg742_num].interface=dev[d].interface;
             spg742[spg742_num].protocol=dev[d].protocol;
             spg742[spg742_num].port=dev[d].port;
             spg742[spg742_num].speed=dev[d].speed;
             spg742[spg742_num].adr=dev[d].adr;
             spg742[spg742_num].type=dev[d].type;
             strcpy(spg742[spg742_num].number,dev[d].number);
             spg742[spg742_num].flat=dev[d].flat;
             spg742[spg742_num].akt=dev[d].akt;
             strcpy(spg742[spg742_num].lastdate,dev[d].lastdate);
             spg742[spg742_num].qatt=dev[d].qatt;
             spg742[spg742_num].qerrors=dev[d].qerrors;
             spg742[spg742_num].conn=dev[d].conn;
             strcpy(spg742[spg742_num].devtim,dev[d].devtim);
             spg742[spg742_num].chng=dev[d].chng;
             spg742[spg742_num].req=dev[d].req;
             spg742[spg742_num].source=dev[d].source;
             strcpy(spg742[spg742_num].name,dev[d].name);
            }    
	 chan_num[spg742_num]++;     
        } 
    if (debug>0) ULOGW ("[742] device [0x%x],adr=%d total %d channels",spg742[spg742_num].device,spg742[spg742_num].adr, chan_num[spg742_num]);
    spg742_num++;
    }
 if (res) mysql_free_result(res);
}
//---------------------------------------------------------------------------------------------------
BOOL StoreData (UINT dv, UINT prm, UINT pipe, UINT status, FLOAT value)
{
 sprintf (query,"SELECT * FROM data WHERE type=0 AND prm=%d AND flat=0 AND source=%d",prm,pipe);
 res=dbase.sqlexec(query); 
 if (row=mysql_fetch_row(res)) sprintf (query,"UPDATE data SET value=%f,status=%d WHERE type=0 AND prm=%d AND flat=0 AND source=%d",value,status,prm,pipe);
 else 
    {
     sprintf (query,"INSERT INTO data(flat,prm,type,value,status,source) VALUES('0','%d','0','%f','%d','%d')",prm,value,status,pipe);
     if (debug>3) ULOGW("[tek] %s",query);
    }
 if (res) mysql_free_result(res);  
 res=dbase.sqlexec(query);
 if (res) mysql_free_result(res);  
 return true;
}
//---------------------------------------------------------------------------------------------------
BOOL StoreData (UINT dv, UINT prm, UINT pipe, UINT type, UINT status, FLOAT value, CHAR* data)
{
 sprintf (query,"SELECT * FROM data WHERE source=%d AND type=%d AND prm=%d AND flat=0 AND date=%s",pipe,type,prm,data);
 res=dbase.sqlexec(query); 
 if (row=mysql_fetch_row(res))
    {     
     sprintf (query,"UPDATE data SET value=%f,status=%d,date=date WHERE source='%d' AND type='%d' AND prm=%d AND flat='0' AND date='%s'",value,status,pipe,type,prm,data);
     if (atof(row[3])<1)
    	{
	 if (debug>3) ULOGW("[742] %s",query);
	 if (res) mysql_free_result(res);
	 res=dbase.sqlexec(query);
	}
     if (res) mysql_free_result(res);
     return true;     
    }
 else sprintf (query,"INSERT INTO data(flat,prm,type,value,status,date,source) VALUES('0','%d','%d','%f','%d','%s','%d')",prm,type,value,status,data,pipe); 
 if (debug>2) ULOGW("[742] %s",query);

 if (res) mysql_free_result(res);
 res=dbase.sqlexec(query); 
 if (res) mysql_free_result(res);
 return true;
}
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 sprintf (devp,"/dev/ttyS%d",blok);
 if (debug>0) ULOGW("[742] attempt open com-port %s on speed %d",devp,speed);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[742] error open com-port %s [%s]",devp,strerror(errno)); 
     return FALSE;
    }
 else if (debug>1) ULOGW("[742] open com-port success"); 

 tcflush(fd, TCIOFLUSH); //Clear send & recive buffers
 tcgetattr(fd, &tio);
 cfsetospeed(&tio, baudrate(speed));
 tio.c_cflag = CS8|CREAD|baudrate(speed)|CLOCAL;
 tio.c_lflag = 0;
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

 return TRUE;
}
//-----------------------------------------------------------------------------        
static uint16_t
calc_bcc (uint8_t *ptr, int n)
{
 uint8_t crc = 0;
 while (n-- > 0) 
	{
	 crc = crc + (uint8_t) *ptr++;
	}
 crc=crc^0xff;
 return crc;
}

static uint16_t
calc_bcc16(uint8_t *ptr, int n)
{
    uint16_t crc=0,j;

    while (n-- > 0)
	{
	 crc = crc ^ (int) *ptr++ << 8;
	 for (j=0;j<8;j++)
		{
		 if(crc&0x8000) crc = (crc << 1) ^ 0x1021;
		 else crc <<= 1;
		}
	}
    return crc;
}

static int 
generate_sequence (int adr, uint8_t type, uint8_t func, uint16_t padr, uint8_t nchan, uint8_t npipe, int no, struct tm* from, struct tm* to, char* sequence, uint16_t* len)
{
 char 		buffer[150];
 float		temp=0;
 uint16_t 	i=0,ks16=0; 
 uint8_t	ks=0, startm=0, ln;

 switch (func)
	{
	case	M4_GET742_ARCHIVE:
				sprintf (sequence,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",DLE,adr,0x90,0x0,0x0,20,0x0,
				func,OCTET_STRING,0x5,0xff,0xff,0,type,nchan,
				ARCHDATE_TAG,0x4,from->tm_year-100,from->tm_mon+1,from->tm_mday,from->tm_hour,
				ARCHDATE_TAG,0x4,to->tm_year-100,to->tm_mon+1,to->tm_mday,to->tm_hour);
				ln=27; startm=0;
				break;
	case	M4_SET742_PARAM:memcpy (buffer,sequence,strlen(sequence));
				ln=13+strlen(sequence); 
				sprintf (sequence,"%c%c%c%c%c%c%c%c%c%c%c%c%c%s",DLE,adr,0x90,0x0,0x0,0x6+strlen(sequence),0x0,
				func,PNUM,0x3,npipe,(uint8_t)((padr)%256),(uint8_t)((padr)/256),buffer);
				startm=0;
				break;	
	case	M4_GET742_PARAM:sprintf (sequence,"%c%c%c%c%c%c%c%c%c%c%c%c%c",DLE,adr,0x90,0x55,0x0,0x6,0x0,
				0x72,PNUM,0x3,npipe,(uint8_t)((padr)%256),(uint8_t)((padr)/256));
				ln=13; startm=0;
				break;
	case	START_EXCHANGE:	for (i=0;i<16; i++) buffer[i]=0xff; buffer[16]=0;
				sprintf (sequence,"%s%c%c%c%c%c%c%c",buffer,DLE,adr,func,ZERO,ZERO,ZERO,ZERO);
				ln=7+16; startm = 16;
				break;
	default:		return	-1;
	}
 ks = calc_bcc ((uint8_t *)sequence+1+startm, ln-1-startm);
 sequence[ln]=(uint8_t)(ks);
 sequence[ln+1]=UK;
 if (func==M4_GET742_PARAM || func==M4_GET742_ARCHIVE || func==M4_SET742_PARAM)
    {
     ks16 = calc_bcc16 ((uint8_t *)sequence+1+startm, ln-1-startm);
     sequence[ln]=(ks16/256);
     sequence[ln+1]=(ks16%256);
    }
 *len=ln+2;
 return 0;
}
//----------------------------------------------------------------------------------------
static int 
analyse_sequence (char* dats, uint len, char* answer, uint8_t analyse, uint8_t id, AnsLog *alog, uint16_t padr)
{
 unsigned char 	dat[1000]; 
 uint16_t 	i=0,start=0, startm=0, cntNS=0, no=0, j=0, data_len=0, m4=0;
 if (len>1000) 	return -1;
 memcpy (dat,dats,len);
 alog->checksym = 0;
 for (i=0; i<len; i++)   printf ("in[%d]:0x%x (%c)\n",i,(uint8_t)dats[i],(uint8_t)dats[i]);
 i=0;
 while (i<len)
	{
	 if (dat[i+2]!=0x90 && dat[i]==DLE && (uint8_t)dat[i+1]<20 && !start)
		{
		 alog->from = (uint8_t)dat[i+1];
		 alog->func=(uint8_t)dat[i+2];
		 startm=i+1; i=i+3; start=1;
		 //if (analyse==ANALYSE) printf ("from=%d func=%d (st=%d)\n",alog->from,alog->func,startm);
		}
	 if (dat[i+2]==0x90 && dat[i]==DLE && (uint8_t)dat[i+1]<20 && !start)
		{
		 alog->from = (uint8_t)dat[i+1];
		 alog->func=(uint8_t)dat[i+7];
		 startm=i+1; i=i+7; start=7;
		 m4=1;
		 //if (analyse==ANALYSE) printf ("from=%d func=%d (st=%d)\n",alog->from,alog->func,startm);
		}
	 if (dat[i]==DLE && dat[i+2]==0x21 && dat[i+3]==0x3)
		{
		 return -1;
		}
	 if (dat[i]==DLE && dat[i+7]==0x21)
		{
		 return -1;
		}
	 //------------------------------------------------------------------------------------------------
	 if (m4 && i==len-1)
		{
 		 uint16_t ks16=calc_bcc16 ((uint8_t *)dat+startm,i-startm-1);
		 data_len=dat[startm+4];
		 if (analyse==NO_ANALYSE)
		    {
			if (ks16/256==(uint8_t)dat[i-1] && ks16%256==(uint8_t)dat[i])
				return 	1;
			else	return	-1;
		    }

		 if (ks16/256==(uint8_t)dat[i-1] && ks16%256==(uint8_t)dat[i]) 
			{
			 alog->checksym = 1;
			 //printf ("ks = %x %x %x %x\n",dat[i-1],dat[i],ks16/256,ks16%256);
		 	 if (alog->func==START_EXCHANGE)
				{
				 memcpy (alog->data[id][no],dat+startm+2,3);
				 alog->data[id][no][3]=0;
				 strcpy (alog->time[id][no],"");
				 alog->quant_param = 1;
				}
			 if (alog->func==M4_GET742_PARAM)
				{
				 for (i=0; i<len; i++)   printf ("in[%d]:0x%x (%c)\n",i,(uint8_t)dats[i],(uint8_t)dats[i]);
				 if (dat[startm+8]<12)
    					memcpy (alog->data[id][no],(dat+startm+9),dat[startm+8]);
    				 //printf ("==%s\n",alog->data[id][no]);
				 return 1;
				}

			 if (alog->func==M4_SET742_PARAM)
				{
				 for (i=0; i<len; i++)   printf ("in[%d]:0x%x (%c)\n",i,(uint8_t)dats[i],(uint8_t)dats[i]);
				 return 1;
				}
				
			 if (alog->func==M4_GET742_ARCHIVE)
				{
				 //for (i=0; i<len; i++)   printf ("in[%d]:0x%x (%c)\n",i,(uint8_t)dats[i],(uint8_t)dats[i]);
				 startm=10;
				 if (dat[21]==0x48) startm=0;
				 if (dat[22]==0x48) startm=1;
				 if (dat[23]==0x48) startm=2;
    			         memcpy (alog->data[id][alog->quant_param],(dat+startm+11+padr),4);
				 sprintf (alog->time[id][alog->quant_param],"%d-%02d-%02d,%02d:%02d:%02d",2000+dat[startm+25],dat[startm+24],dat[startm+23],dat[startm+20],0,0);
				 //printf ("[%s] %x %x %x %x (%f)\n",alog->time[id][alog->quant_param],alog->data[id][alog->quant_param][0],alog->data[id][alog->quant_param][1],alog->data[id][alog->quant_param][2],alog->data[id][alog->quant_param][3],*(float *)alog->data[id][alog->quant_param]);
				 if (startm<2)
				    return 1;
				 else
				    return -1;
				}
			}
		}
	 if (!m4)
	 if (i==len-1 && (dat[i]==UK && dat[i+1]!=UK) && ((id<98) || ((id>=98) && (i-2-startm>62))))
		{
 		 uint8_t ks=calc_bcc ((uint8_t *)dat+startm,i-startm-1);
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
				 strcpy (alog->time[id][no],"");
				 alog->quant_param = 1;
				}
			 if (alog->func==GET742_FLASH)
				{
				 memcpy (alog->data[id][no],(dat+startm+4+2+(id%4)*16),8);
				 alog->data[id][no][8]=0;
				 strcpy (alog->time[id][no],"");
				 alog->quant_param = 1;
				}
			}
		 else 
			{
			 alog->checksym = 0;
			 for (j=0;j<len;j++) fprintf (stderr,"[%d] [%d] = 0x%x [%c]\n",padr,j,(uint8_t)dat[j],(uint8_t)dat[j]);
			 //fprintf (stderr,"wrong checksum alog.checksym=%d(rec=%x,must=%x) at pos=%d\n",alog->checksym,ks,(uint8_t)dat[i-1],i);
			}
		 if (id<0x189) return 1;
		}
	 i++;
	}
 if (id>=98) return 1;
 else  return 0;
}

static int
spg742_get_ident(int adr)
{
	static char ident_query[100];
	char spg_id[100],answer[100],data[100];
	int 	ret;
	uint16_t	len,bytes;
	AnsLog	alog;
	struct 	tm *time_from=(tm *)malloc (sizeof *time_from);
	struct 	tm *time_to=(tm *)malloc (sizeof *time_to);

	time_t tim;
	tim=time(&tim);			// default values
	localtime_r(&tim,time_from); 	// get current system time
	localtime_r(&tim,time_to); 	// get current system time

	ret = -1;

	generate_sequence (adr, 0, START_EXCHANGE, 0, 0, 0, 1, time_from, time_to, ident_query, &len);
	write (fd,&ident_query,len);
	sleep(1);
	ioctl (fd,FIONREAD,&bytes);  
	if (debug>2) ULOGW("[741] bytes=%d",bytes);
        bytes=read (fd, &data, 90);
	analyse_sequence (data, len, answer, ANALYSE, 0, &alog, 0);

	generate_sequence (adr, 0, GET742_FLASH, 29, 0, 0, 1, time_from, time_to, ident_query, &len);
	write (fd,&ident_query,len);
	sleep(1);
    	ioctl (fd,FIONREAD,&bytes);  
    	if (debug>2) ULOGW("[741] bytes=%d",bytes);     
        bytes=read (fd, &data, 90);
	analyse_sequence (data, len, spg_id, ANALYSE, 1, &alog, 8);

	sprintf (spg_id,"%s",alog.data[1][0]);
	printf ("[741] SPG ident=%s\n", spg_id);
	ret = 0;
	free (time_from);
	free (time_to);
	return ret;
}

static int
spg742_get_string_param(int adr, uint16_t addr, char *save, uint8_t type, uint16_t padr)
{
	static char ident_query[100],	answer[1024], date[30];
	void 	*fnsave;
	int 	ret;
	uint16_t	len,bytes;
	float	temp=0;
	struct 	tm *time_from=(tm *)malloc (sizeof *time_from);
	struct 	tm *time_to=(tm *)malloc (sizeof *time_to);
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
    		    generate_sequence (adr, 0, M4_GET742_PARAM, addr, 0, 0, 4, time_from, time_to, ident_query, &len);
		if (padr>0)
    		    generate_sequence (adr, 0, M4_GET742_PARAM, addr, 0, 1, 4, time_from, time_to, ident_query, &len);

		write (fd,&ident_query,len);
		sleep(1);
		ioctl (fd,FIONREAD,&bytes);  
		if (debug>2) ULOGW("[tek] bytes=%d",bytes);     
	        bytes=read (fd, &answer, 200);
		analyse_sequence (answer, bytes, answer, ANALYSE, 0, &alog, padr);
		//devlog(dev, "RECV: [%x %x %x %x]\n", (uint8_t)alog.data[0][0][0], (uint8_t)alog.data[0][0][1], (uint8_t)alog.data[0][0][2], (uint8_t)alog.data[0][0][3]);

		if (padr>0)
			{
			 temp=*(float*)(alog.data[0][0]);
			 //fprintf (stderr,"%f (%x %x %x %x)\n",temp,(uint8_t)alog.data[0][0][0],(uint8_t)alog.data[0][0][1],(uint8_t)alog.data[0][0][2],(uint8_t)alog.data[0][0][3]);
			 snprintf (value_string,100,"%f",temp);
			 sprintf (save,"%s",value_string);
			 ret=0;
			}
		else	{
			 //fprintf (stderr,"%d-%02d-%02d,%02d:%02d:%02d\n",alog.data[0][0][2]+2000,alog.data[0][0][1],alog.data[0][0][0],alog.data[0][0][3],alog.data[0][0][4],alog.data[0][0][5]);
			 snprintf (value_string,100,"%d-%02d-%02d,%02d:%02d:%02d",alog.data[0][0][0]+2000,alog.data[0][0][1],alog.data[0][0][2],alog.data[0][0][3],alog.data[0][0][4],alog.data[0][0][5]);
			 sprintf (save,"%s",value_string);
			 ret=0;
			}
		}
	if (type==TYPE_CONST) {
		if (addr==1024)
    		     generate_sequence (adr, 0, M4_GET742_PARAM, addr, 0, 0, 1, time_from, time_to, ident_query, &len);
    		else
    		     generate_sequence (adr, 0, M4_GET742_PARAM, addr, 0, 0, 1, time_from, time_to, ident_query, &len);

		write (fd,&ident_query,len);
		sleep(1);
		ioctl (fd,FIONREAD,&bytes);  
		if (debug>2) ULOGW("[tek] bytes=%d",bytes);     
	        bytes=read (fd, &answer, 200);
		analyse_sequence (answer, bytes, answer, ANALYSE, 0, &alog, padr);
		
		snprintf (value_string,100,"%s",alog.data[0][0]);

		if (addr==0x400) {
			snprintf (date,20,"%s",alog.data[(addr)%4][0]);
			generate_sequence (adr, 0, M4_GET742_PARAM, addr+1, padr, 0, 1, time_from, time_to, ident_query, &len);

			write (fd,&ident_query,len);
			sleep(1);
    			ioctl (fd,FIONREAD,&bytes);  
    			if (debug>2) ULOGW("[tek] bytes=%d",bytes);     
		        bytes=read (fd, &answer, 200);

			analyse_sequence (answer, len, answer, ANALYSE, (addr+1)%4, &alog, padr);
			snprintf (value_string,30,"%s,%s",date,alog.data[(addr+1)%4][0]);
			sscanf(value_string,"%d-%d-%d,%d-%d-%d",&time_to->tm_mday, &time_to->tm_mon, &time_to->tm_year, &time_to->tm_hour, &time_to->tm_min, &time_to->tm_sec);
			snprintf (value_string,25,"%02d-%02d-%02d,%02d:%02d:%02d",time_to->tm_year+1900, time_to->tm_mon+1, time_to->tm_mday, time_to->tm_hour, time_to->tm_min, time_to->tm_sec);
			//printf ("date=%s\n",value_string);
		    }
		sprintf (save,"%s",value_string);
		ret=0;
		}
	free (time_from);
	free (time_to);
	return ret;
}

static int 
spg742_h_archiv(int device, int adr, int no, const char *from, const char *to)
{
	AnsLog alog;
	int ret,ret_fr,ret_to,quant=0,i;
	uint16_t len,ntry,bytes,nbytes;
	static char ident_query[100], answer[1024],date[100];
	time_t tim,sttime,fntime,entime;
	float	value;

	struct 	tm *time_from=(tm *)malloc (sizeof *time_from);
	ret_fr = sscanf(from,"%d-%d-%d,%d:%d:%d",&time_from->tm_year, &time_from->tm_mon, &time_from->tm_mday, &time_from->tm_hour, &time_from->tm_min, &time_from->tm_sec);
	struct 	tm *time_to=(tm *)malloc (sizeof *time_to);
	ret_to = sscanf(to,"%d-%d-%d,%d:%d:%d",&time_to->tm_year, &time_to->tm_mon, &time_to->tm_mday, &time_to->tm_hour, &time_to->tm_min, &time_to->tm_sec);

	struct 	tm *time_cur=(tm *)malloc (sizeof *time_cur);
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
		    ntry=1;
	    	    while (ntry--)	{
	    		alog.quant_param=0;

			if (-1 == generate_sequence (adr, 0, M4_GET742_ARCHIVE, archive742[i].adr, 1, 1, no, time_from, time_from, ident_query, &len)) break;
			write (fd,&ident_query,len);
			sleep(1);
    			ioctl (fd,FIONREAD,&bytes);  
    			if (debug>2) ULOGW("[tek] bytes=%d",bytes);     
		        bytes=read (fd, &answer, 230);
    			
    			for (i=0; i<sizeof(archive742)/sizeof(archive742[0]); i++)	{
			     if (analyse_sequence (answer, bytes, answer, ANALYSE, archive742[i].id, &alog, archive742[i].adr))
			        {
		                 //prevtime=localtime(&tims); 	// get current system time
			         sprintf (date,"%s",alog.data[archive742[i].id][alog.quant_param]);
			         value=*(float*)(date); 
        		         //if (debug>2) ULOGW ("[tek] [1] [%d] [%f]",index,value);
    			         StoreData (device, archive742[i].no, archive742[i].id, 1, 0, value, date);
    			        }
			    }
		        alog.quant_param++;
			break;
			}
		 sttime+=3600;
		}
      	free (time_from);
	free (time_to);
	return ret;
}



static int 
spg742_d_archiv(int device, int adr, int no, const char *from, const char *to)
{
	struct 	spg742_ctx *ctx;
	AnsLog 	alog;
	float	value;
	int ret,ret_fr,ret_to,quant=0,i;
	uint16_t len,ntry,bytes,nbytes;
	static char ident_query[100], answer[1024],date[100];
	time_t tim,sttime,fntime,entime;

	struct 	tm *time_from=(tm*)malloc (sizeof *time_from);
	ret_fr = sscanf(from,"%d-%d-%d,%d:%d:%d",&time_from->tm_year, &time_from->tm_mon, &time_from->tm_mday, &time_from->tm_hour, &time_from->tm_min, &time_from->tm_sec);

	struct 	tm *time_to=(tm*)malloc (sizeof *time_to);
	ret_to = sscanf(to,"%d-%d-%d,%d:%d:%d",&time_to->tm_year, &time_to->tm_mon, &time_to->tm_mday, &time_to->tm_hour, &time_to->tm_min, &time_to->tm_sec);

	struct 	tm *time_cur=(tm*)malloc (sizeof *time_cur);
	tim=time(&tim);
	localtime_r(&tim,time_cur);

	ret=checkdate (TYPE_MONTH,ret_fr,ret_to,time_from,time_to,time_cur,&tim,&sttime,&fntime);
	alog.quant_param=0;
	sttime-=3600;

	if (sttime && fntime && ret==0)
	while (sttime<=fntime)
		{
	         entime=sttime+3600*24;
		 localtime_r(&sttime,time_from);
		 localtime_r(&entime,time_to);

		    ntry=1;
	    	    while (ntry--)	{
	    		alog.quant_param=0;

			if (-1 == generate_sequence (adr, 3, M4_GET742_ARCHIVE, archive742[i].adr, 1, 1, no, time_from, time_from, ident_query, &len)) break;
			write (fd,&ident_query,len);
			sleep(1);
    			ioctl (fd,FIONREAD,&bytes);  
    			if (debug>2) ULOGW("[tek] bytes=%d",bytes);     
		        bytes=read (fd, &answer, 200);
    			
    			for (i=0; i<sizeof(archive742)/sizeof(archive742[0]); i++)	{
			     if (analyse_sequence (answer, bytes, answer, ANALYSE, archive742[i].id, &alog, archive742[i].adr))
				{
		                 //prevtime=localtime(&tims); 	// get current system time
			         sprintf (date,"%s",alog.data[archive742[i].id][alog.quant_param]);
			         value=*(float*)(date); 
        		         //if (debug>2) ULOGW ("[tek] [1] [%d] [%f]",index,value);
    			         StoreData (device, archive742[i].no, archive742[i].id, 1, 0, value, date);
    			        }
			    }

		        alog.quant_param++;
	    		break;
			}
		 sttime+=3600*24;
		}
	free (time_from);
	free (time_to);
	return ret;
}

static int  checkdate (uint8_t	type, int ret_fr, int ret_to, struct tm* time_from, struct tm* time_to, struct tm* time_cur, time_t *tim, time_t *sttime, time_t *fntime)
{
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
		if (type==TYPE_EVENTS) *sttime-=3600*24*365*5;
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
		if (type==TYPE_MONTH) *sttime-=3600*24*365;
		if (type==TYPE_EVENTS) *sttime-=3600*24*365*5;
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
