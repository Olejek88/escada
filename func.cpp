//---------------------------------------------------------------------------
// main function for all modules
//---------------------------------------------------------------------------
//#include <glibtop.h>
//#include <glibtop/cpu.h>

#include "types.h"
#include "main.h"
#include "db.h"
#include "func.h"

extern 	CHAR	kernellog[300];
extern	"C" UINT	debug;
//#include "kernel.h"
//---------------------------------------------------------------------------
void ULOGW (const char* string, ...)
{
char 	buf[500];
FILE *Log;
//int	fd;
struct tm 	*ttime;
//struct flock	lck;

time_t tim;
//lck.l_type=F_WRLCK;
//lck.l_whence=SEEK_SET;
//lck.l_start=0;
//lck.l_len=0;
//lck.l_pid=0;
Log = fopen(kernellog,"a");
//fd=open(KERNEL_LOG, O_APPEND);
//printf ("fd=%d\n",fd);
//if (fcntl (fd,F_SETLK,&lck)==-1) { close (fd); return; }

tim=time(&tim);
ttime=localtime(&tim);
sprintf (buf,"%02d-%02d %02d:%02d:%02d ",ttime->tm_mon+1,ttime->tm_mday,ttime->tm_hour,ttime->tm_min,ttime->tm_sec);
//write (fd,buf,strlen(buf));
fprintf (Log, buf); 

va_list arg; va_start(arg, string);
vsnprintf(buf,sizeof (buf), string, arg);
fprintf (Log, buf); 
//write (fd,buf,strlen(buf));
printf (buf); va_end(arg);
fprintf (Log,"\n"); 
//write (fd,"\n",1);
printf ("\n");
//lck.l_type=F_UNLCK;
//fcntl (fd, F_SETLK, &lck);
//close (fd);
fclose (Log);
}
//---------------------------------------------------------------------------
UINT baudrate (UINT baud)
{
 switch (baud)
    {
     case 300: return B300;
     case 600: return B600;
     case 1200: return B1200;
     case 2400: return B2400;
     case 4800: return B4800;
     case 9600: return B9600;
     case 19200: return B19200;
     case 38400: return B38400;
     case 57600: return B57600;
     case 115200: return B115200;
    }
}
//---------------------------------------------------------------------------------------------------
VOID Events (db dbase, DWORD evnt, DWORD device)
{
 //sprintf (query,"INSERT INTO register(code,device) VALUES('%d','%d')",evnt,device);
 //dbase.sqlexec(query); 
 //if (debug>1)  ULOGW ("[db::register] [0x%x] events: 0x%x",device,evnt);
}
//---------------------------------------------------------------------------------------------------
VOID Events (db dbase, DWORD evnt, DWORD device, FLOAT value)
{
 //sprintf (query,"INSERT INTO register(code,device,value) VALUES('%d','%d','%f')",evnt,device,value);
 //dbase.sqlexec(query); 
 //if (debug>1)  ULOGW ("[db::register] [0x%x] events: 0x%x [val=%f]",device,evnt,value);
}
//---------------------------------------------------------------------------------------------------
VOID Events (db dbase, DWORD evnt, DWORD device, DWORD code, FLOAT value, DWORD type, DWORD chan)
{
 char descr[100]={0};
 MYSQL_RES *res;
 MYSQL_ROW row;
 CHAR query[500];
 sprintf (query,"SELECT * FROM event WHERE code=%d",evnt);
 res=dbase.sqlexec(query);
 if (row=mysql_fetch_row(res)) 
    {
     sprintf (query,"INSERT INTO register(code,device,value,descr,type,chan) VALUES('%d','%d','%f','%s','%d','%d')",code,device,value,row[2],type,chan);
     dbase.sqlexec(query); 
    }
 if (debug>3)  ULOGW ("[db::register] [0x%x] events: 0x%x",device,evnt);
}
//---------------------------------------------------------------------------------------------------
unsigned char Crc8 (BYTE* data, UINT lent)
{
unsigned char crc=0xff;
unsigned int cnt=0;

while (lent--)
{
 crc ^= *data++; 
 for (UINT i=0; i<8; i++)
    crc = crc&0x80 ? (crc << 1) ^ 0x31 : crc << 1;
 cnt++;
}
return crc;
}
//---------------------------------------------------------------------------------------------------
const BYTE crc8_tabl[]={
0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53};

BYTE crc8_compute_tabel ( BYTE* str, BYTE col)
{
  BYTE temp=0, crc8=0;
  while (temp != col)
  {
    crc8=crc8_tabl[crc8 ^ str[temp]];
    temp++;
  }
  return crc8;
}
//---------------------------------------------------------------------------------------------------
#include <net/if.h>
//---------------------------------------------------------------------------------------------------
char* getip(char *ip)
{
 int fd;
 struct ifreq ifr;
 //char	ip[50];
 fd = socket(AF_INET, SOCK_DGRAM, 0);
 ifr.ifr_addr.sa_family = AF_INET;
 strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
 ioctl(fd, SIOCGIFADDR, &ifr);
 close(fd);

 sprintf(ip,"%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
 return ip;
}
//--------------------------------------------------------------------------------------
BOOL SetDeviceStatus(db dbase, UINT devtype, UINT devnum, UINT devadr, UINT type, CHAR* datatime)
{
 CHAR 	types[30];
 MYSQL_RES *res;
 MYSQL_ROW row;
 CHAR   	query[500];

 if (type==0) sprintf (types,"currents");
 if (type==1) sprintf (types,"hours");
 if (type==2) sprintf (types,"days");
 if (type==4) sprintf (types,"month");
 if (type==5) sprintf (types,"increments");

 sprintf (query,"UPDATE threads SET global=1,start=1,curr=%d,curr_adr=%d,status=1,type='%s',ctime='%s' WHERE thread=%d",devnum,devadr,types,datatime,devtype);
 ULOGW ("[set] %s",query);
 res=dbase.sqlexec(query); 
 if (res) mysql_free_result(res);
}
//---------------------------------------------------------------------------------------------------
// function store current data to database
BOOL StoreData (db dbase, UINT dv, UINT prm, UINT pipe, UINT status, FLOAT value, uint8_t dest)
{
 MYSQL_RES *res;
 MYSQL_ROW row;
 CHAR   	query[500];

 if (dest==0) sprintf (query,"SELECT * FROM prdata WHERE type=0 AND prm=%d AND device=%d AND pipe=%d",prm,dv,pipe);
 else sprintf (query,"SELECT * FROM data WHERE type=0 AND prm=%d AND flat=0 AND source=%d",prm,pipe);

 res=dbase.sqlexec(query);
 if (row=mysql_fetch_row(res)) 
    {
     if (dest==0) sprintf (query,"UPDATE prdata SET value=%f,status=%d,date=NULL WHERE type=0 AND prm=%d AND device=%d AND pipe=%d",value,status,prm,dv,pipe);
     else sprintf (query,"UPDATE data SET value=%f,status=%d,date=NULL WHERE type=0 AND prm=%d AND flat=%d AND source=%d",value,status,prm,dv,pipe);
    }
 else 
    {
     if (dest==0) sprintf (query,"INSERT INTO prdata(device,prm,type,value,status,pipe) VALUES('%d','%d','0','%f','%d','%d')",dv,prm,value,status,pipe);
     else sprintf (query,"INSERT INTO data(flat,prm,type,value,status,source) VALUES('0','%d','0','%f','%d','%d')",prm,value,status,pipe);
    }
 if (debug>2) ULOGW("[db] %s",query);
 if (res) mysql_free_result(res);  
 res=dbase.sqlexec(query);
 if (res) mysql_free_result(res);  
 return true;
}
//---------------------------------------------------------------------------------------------------
// function store current data to database
BOOL StoreData (db dbase, UINT dv, UINT prm, UINT pipe, UINT status, FLOAT value, uint8_t dest, uint16_t chan)
{
 MYSQL_RES *res;
 MYSQL_ROW row;
 CHAR   	query[500];

 if (dest==0) sprintf (query,"SELECT * FROM prdata WHERE type=0 AND prm=%d AND device=%d AND pipe=%d AND channel=%d",prm,dv,pipe,chan);
 else sprintf (query,"SELECT * FROM data WHERE type=0 AND prm=%d AND flat=0 AND source=%d",prm,pipe);
 if (debug>3) ULOGW("[db] %s",query);
 res=dbase.sqlexec(query);
 if (row=mysql_fetch_row(res)) 
    {
     if (dest==0) sprintf (query,"UPDATE prdata SET value=%f,status=%d,date=NULL WHERE type=0 AND prm=%d AND device=%d AND pipe=%d AND channel=%d",value,status,prm,dv,pipe,chan);
     else sprintf (query,"UPDATE data SET value=%f,status=%d,date=NULL WHERE type=0 AND prm=%d AND flat=%d AND source=%d",value,status,prm,dv,pipe);
    }
 else 
    {
     if (dest==0) sprintf (query,"INSERT INTO prdata(device,prm,type,value,status,pipe,channel) VALUES('%d','%d','0','%f','%d','%d','%d')",dv,prm,value,status,pipe,chan);
     else sprintf (query,"INSERT INTO data(flat,prm,type,value,status,source) VALUES('0','%d','0','%f','%d','%d')",prm,value,status,pipe);
    }
 if (debug>2) ULOGW("[db] %s",query);
 if (res) mysql_free_result(res);  
 res=dbase.sqlexec(query);
 if (res) mysql_free_result(res);  

 sprintf (query,"UPDATE channels SET lastcurrents=NULL WHERE id=%d",chan);
 res=dbase.sqlexec(query);
 if (res) mysql_free_result(res);  
 return true;
}
//---------------------------------------------------------------------------------------------------
// function store archive data to database
BOOL StoreData (db dbase, UINT dv, UINT prm, UINT pipe, UINT type, UINT status, FLOAT value, CHAR* data, uint8_t dest)
{
 MYSQL_RES *res;
 MYSQL_ROW row;
 CHAR   	query[500];

 if (dest==0) 
    {
     if (type==TYPE_HOURS || type==9) sprintf (query,"SELECT * FROM hours WHERE device=%d AND type=%d AND prm=%d AND pipe=%d AND date=%s",dv,type,prm,pipe,data);
     else sprintf (query,"SELECT * FROM prdata WHERE device=%d AND type=%d AND prm=%d AND pipe=%d AND date=%s",dv,type,prm,pipe,data);
    }
 else sprintf (query,"SELECT * FROM data WHERE source=%d AND type=%d AND prm=%d AND flat=0 AND date=%s",pipe,type,prm,data);

 res=dbase.sqlexec(query);
 //if (debug>2) ULOGW("[db] %s",query);
 if (row=mysql_fetch_row(res))
    {
     if (dest==0) 
        { 
         if (type==TYPE_HOURS || type==9) sprintf (query,"UPDATE hours SET value=%f,date=date WHERE pipe='%d' AND type='%d' AND prm=%d AND date='%s' AND device=%d",value,pipe,type,prm,data,dv);
         else sprintf (query,"UPDATE prdata SET value=%f,date=date WHERE pipe='%d' AND type='%d' AND prm=%d AND date='%s' AND device=%d",value,pipe,type,prm,data,dv);
        }
     else sprintf (query,"UPDATE data SET value=%f,status=%d,date=date WHERE source='%d' AND type='%d' AND prm=%d AND flat='0' AND date='%s'",value,status,pipe,type,prm,data);

     if (atof(row[3])==0 && value>0)
    	{
	 if (debug>2) ULOGW("[db] %s",query);
	 res=dbase.sqlexec(query);
	}
     if (res) mysql_free_result(res);
     return true;
    }
 else 
    {
     if (dest==0) 
        {
         if (type==TYPE_HOURS || type==9) sprintf (query,"INSERT INTO hours(device,prm,type,value,status,date,pipe) VALUES('%d','%d','%d','%f','%d','%s','%d')",dv,prm,type,value,0,data,pipe);
         else sprintf (query,"INSERT INTO prdata(device,prm,type,value,status,date,pipe) VALUES('%d','%d','%d','%f','%d','%s','%d')",dv,prm,type,value,0,data,pipe);
        }
     else sprintf (query,"INSERT INTO data(flat,prm,type,value,status,date,source) VALUES('0','%d','%d','%f','%d','%s','%d')",prm,type,value,status,data,pipe); 
    }
 if (debug>2) ULOGW("[db] %s",query);
 if (res) mysql_free_result(res);
 res=dbase.sqlexec(query);
 if (res) mysql_free_result(res);
 return true;
}
//---------------------------------------------------------------------------------------------------
// function store archive data to database
BOOL StoreData (db dbase, UINT dv, UINT prm, UINT pipe, UINT type, UINT status, FLOAT value, CHAR* data, uint8_t dest, uint16_t channel)
{
 MYSQL_RES *res;
 MYSQL_ROW row;
 CHAR   	query[500];

 if (dest==0) 
    {
     if (type==TYPE_HOURS || type==9) sprintf (query,"SELECT * FROM hours WHERE device=%d AND type=%d AND prm=%d AND pipe=%d AND date=%s AND channel=%d",dv,type,prm,pipe,data,channel);
     else sprintf (query,"SELECT * FROM prdata WHERE device=%d AND type=%d AND prm=%d AND pipe=%d AND date=%s AND channel=%d",dv,type,prm,pipe,data,channel);
    }
 else sprintf (query,"SELECT * FROM data WHERE source=%d AND type=%d AND prm=%d AND flat=0 AND date=%s",pipe,type,prm,data);

 res=dbase.sqlexec(query);
 if (debug>3) ULOGW("[db] %s",query);
 if (row=mysql_fetch_row(res))
    {
     if (dest==0) 
        { 
         if (type==TYPE_HOURS || type==9) sprintf (query,"UPDATE hours SET value=%f,date=date WHERE pipe='%d' AND type='%d' AND prm=%d AND date='%s' AND device=%d AND channel=%d",value,pipe,type,prm,data,dv,channel);
         else sprintf (query,"UPDATE prdata SET value=%f,date=date WHERE pipe='%d' AND type='%d' AND prm=%d AND date='%s' AND device=%d AND channel=%d",value,pipe,type,prm,data,dv,channel);
        }
     else sprintf (query,"UPDATE data SET value=%f,status=%d,date=date WHERE source='%d' AND type='%d' AND prm=%d AND flat='0' AND date='%s'",value,status,pipe,type,prm,data);

//     if (type==4 && atof(row[3])==0 && value>0)
     if (type==4 && value!=atof(row[3]) && prm==14 && 0)
    	{
	 if (debug>2) ULOGW("[db] %s",query);
	 res=dbase.sqlexec(query);
	}
     if (res) mysql_free_result(res);
     return true;
    }
 else 
    {
     if (dest==0) 
        {
         if (type==TYPE_HOURS || type==9) sprintf (query,"INSERT INTO hours(device,prm,type,value,status,date,pipe,channel) VALUES('%d','%d','%d','%f','%d','%s','%d','%d')",dv,prm,type,value,0,data,pipe,channel);
         else sprintf (query,"INSERT INTO prdata(device,prm,type,value,status,date,pipe,channel) VALUES('%d','%d','%d','%f','%d','%s','%d','%d')",dv,prm,type,value,0,data,pipe,channel);
        }
     else sprintf (query,"INSERT INTO data(flat,prm,type,value,status,date,source) VALUES('0','%d','%d','%f','%d','%s','%d')",prm,type,value,status,data,pipe); 
    }
 if (debug>2) ULOGW("[db] %s",query);
 if (res) mysql_free_result(res);
 res=dbase.sqlexec(query);
 if (res) mysql_free_result(res);
 return true;
}

//---------------------------------------------------------------------------------------------------
BOOL UpdateThreads (db dbase, uint16_t thread_id, uint8_t global, uint8_t start, uint8_t curr, uint16_t curr_adr, uint8_t status, uint8_t type, CHAR* datatime)
{
 MYSQL_RES *res;
 MYSQL_ROW row;
 CHAR   	query[500],types[40];
 switch (type)
    {
     case 0: sprintf (types,"currents");
     case 1: sprintf (types,"hours");
     case 2: sprintf (types,"days");
     case 4: sprintf (types,"month");
     case 7: sprintf (types,"increments");   }
 sprintf (query,"SELECT * FROM threads WHERE thread=%d",thread_id);
 res=dbase.sqlexec(query);
 if (row=mysql_fetch_row(res)) 
    {
     if (strlen (datatime)<5) sprintf (query,"UPDATE threads SET global=%d, start=%d, curr=%d, curr_adr=%d, status=%d, type='%s' WHERE thread=%d",global,start,curr,curr_adr,status,types,thread_id);
     else sprintf (query,"UPDATE threads SET global=%d, start=%d, curr=%d, curr_adr=%d, status=%d, type='%s', ctime='%s' WHERE thread=%d",thread_id,global,start,curr,curr_adr,status,types,ctime);
    }
 else 
    {
     sprintf (query,"INSERT INTO threads SET global=%d, start=%d, curr=%d, curr_adr=%d, status=%d, type='%s',thread=%d",global,start,curr,curr_adr,status,types,thread_id);
    }
 if (debug>4) ULOGW("[db] %s",query);
 if (res) mysql_free_result(res);  
 res=dbase.sqlexec(query);
 if (res) mysql_free_result(res);  
 return true;
}

//---------------------------------------------------------------------------------------------------
BOOL OpenTTY (UINT fd, SHORT blok, UINT speed)
{
 CHAR devp[50];
 snprintf (devp,49,"/dev/ttyS%d",blok);
 if (debug>0) ULOGW("[tty%d] attempt open com-port %s on speed %d",blok,devp,speed);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[tty%d] %serror open com-port %s [%s]%s",blok,kernel_color,devp,strerror(errno),nc); 
     return false;
    }
 else if (debug>1) ULOGW("[tty%d] open com-port %ssuccess%s",blok,success_color,nc); 

 tcflush(fd, TCIOFLUSH); //Clear send & recive buffers
 tcgetattr(fd, &tios);
 cfsetospeed(&tios, baudrate(speed));
 tios.c_cflag = CS8|CREAD|baudrate(speed)|CLOCAL;
// tio.c_cflag &= ~(CSIZE|PARENB|PARODD);
// tio.c_cflag &= ~CSTOPB;
// tio.c_cflag &= ~CRTSCTS;
// tio.c_lflag = ICANON;
 tios.c_lflag = 0;
// tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
 tios.c_iflag = IGNPAR| ICRNL;
 tios.c_oflag = 0;
// tio.c_iflag &= ~(INLCR | IGNCR | ICRNL);
// tio.c_iflag &= ~(IXON | IXOFF | IXANY);
 
// tio.c_iflag = IGNCR | IGNPAR;
// tio.c_oflag &= ~(ONLCR);
 
 tios.c_cc[VMIN] = 0;
 tios.c_cc[VTIME] = 5; //Time out in 10e-1 sec
 cfsetispeed(&tios, baudrate(speed));
 fcntl(fd, F_SETFL, FNDELAY);
 //fcntl(fd, F_SETFL, 0);
 tcsetattr(fd, TCSANOW, &tios);
 tcsetattr (fd,TCSAFLUSH,&tios);

 return true;
}

//-----------------------------------------------------------------------------    
UINT  GetChannelNum (db dbase, uint16_t prm, uint16_t pipe, UINT device)
    {
     MYSQL_RES *res;
     MYSQL_ROW row;
     CHAR   	query[500];
     UINT       chan=0;	
     sprintf (query,"SELECT * FROM channels WHERE prm=%d AND pipe=%d AND device=%d",prm,pipe,device);
     if (debug>3) ULOGW("[db] %s",query);
     res=dbase.sqlexec(query); 
     if (row=mysql_fetch_row(res))
	{
          //if (debug>2) ULOGW("[db] chan=%d",atoi(row[0]));
          return atoi(row[0]);
	}
     return 0;
    }
//-----------------------------------------------------------------------------    
uint8_t BCD (uint8_t dat)
{
    uint8_t data=0;
    data=((dat&0xf0)>>4)*10+(dat&0xf);
    return	data;
}