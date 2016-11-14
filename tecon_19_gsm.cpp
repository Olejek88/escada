//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "tecon_19_gsm.h"
#include "db.h"
#include <fcntl.h>
//-----------------------------------------------------------------------------
#include "sys/types.h"
#include "sys/socket.h"
#include <linux/ip.h>
#include <linux/icmp.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>

#define COMM_BUFFER_SIZE 1024
//-----------------------------------------------------------------------------
static INT  current_sock_max=0;
static INT  server_socket;
static INT  http_cln_socket;

static sockaddr_in 	si;
static sockaddr_in 	so[MAX_CLIENTS];
static CHAR		address[MAX_CLIENTS][20];

static	pthread_t thr;
//-----------------------------------------------------------------------------
static  MYSQL_RES *res;
static  db      dbase;
static  MYSQL_ROW row;

static  CHAR    query[500];
static  INT     fd;
static  termios tio;

extern  "C" UINT device_num;    // total device quant
extern  "C" UINT tekon_num;     // total device quant
extern  "C" BOOL WorkRegim;     // work regim flag
extern  "C" tm *currenttime;    // current system time

extern  "C" DeviceR     dev[MAX_DEVICE];
extern  "C" DeviceTekonGSM tekon[MAX_DEVICE_TEKON];
extern  "C" DeviceDK	dk;
extern  "C" BOOL	tek_thread;
extern  "C" UINT	debug;

static  union fnm fnum[5];
static UINT    chan_num[MAX_DEVICE_TEKON]={0};

static INT WaitForClientConnections(INT server_socket);
static INT StartWebServer();
static VOID* HandleICMPRequest (VOID *data);
unsigned short in_cksum(unsigned short *addr, int len);
	BOOL FindTekon (CHAR* addr, INT id, INT conn);
        UINT    read_tekon (BYTE* dat);
static  BYTE    CRC(const BYTE* const Data, const BYTE DataSize);
        VOID    ULOGW (const CHAR* string, ...);              // log function
        VOID    Events (DWORD evnt, DWORD device);      // events 
static  BOOL    LoadTekonConfig();                      // load tekon configuration
static  BOOL    StoreData (UINT dv, UINT prm, UINT pipe, UINT status, FLOAT value);             // store data to DB
static  BOOL    StoreData (UINT dv, UINT prm, UINT pipe, UINT type, UINT status, FLOAT value, CHAR* data);
//-----------------------------------------------------------------------------
void * tekonGSMDeviceThread (void * devr)
{
 dbase.sqlconn("dk","root","");                 // connect to database

 tek_thread=TRUE;
 debug=3;
 //BOOL hConnection (CHAR tP, INT _s_port, INT sck, INT cli)
 if (debug>1) ULOGW ("[tek-gsm] tekonGSMDeviceThread ()");
 server_socket = StartWebServer();
 if (debug>1) ULOGW ("[tek-gsm] socket=%d",server_socket);

// if(pthread_create(&thr,NULL,&HandleICMPRequest,(void *)server_socket) != 0) 
//    ULOGW ("[tek-gsm] error create ICMP thread");
     
 if (server_socket)
	{
         WaitForClientConnections(server_socket);
	 close(server_socket);
	}
 else ULOGW ("[tek-gsm] Error in StartWebServer()");

 int devdk=*((int*) devr);                      // DK identificator
 LoadTekonConfig();

 for (UINT d=0;d<tekon_num;d++)
    {
     so[d].sin_family = AF_INET;
     so[d].sin_port = htons(atoi(UDP_PORT));		// port
     so[d].sin_addr.s_addr = inet_addr(tekon[d].number);
     if (debug>1) ULOGW ("[tek-gsm] [%d] [%d / %d] open socket AF_INET | %d | %s",d,tekon[d].adr,tekon[d].port, atoi(UDP_PORT),tekon[d].number);
    }
// for (UINT d=0;d<=tekon_num;d++)
// for (UINT r=0;r<chan_num[d];r++)
// if (tekon[d].prm[r]==14)
//	 tekon[d].ReadAllArchive (r,60);

 while (WorkRegim)
 for (UINT d=0;d<tekon_num;d++)
 for (UINT r=0;r<chan_num[d];r++)
    {
     if (debug>1) ULOGW ("[tek] ReadDataCurrent (%d)",r);
     tekon[d].ReadDataCurrent (r); 
     sleep (5);
     if (currenttime->tm_min>0)  
        {
         if (debug>1) ULOGW ("[tek] ReadDataArchive (%d)",r);
         tekon[d].ReadAllArchive (r,1);
	}	
     //tekon[d].ReadDataArchive (r);
     // Device address in the network 
     else sleep (6);
     if (!dk.pth_tek)
        {
	 if (debug>0) ULOGW ("[tek] tekon thread stopped");
	 //dbase.sqldisconn();
	 tek_thread=FALSE;	// we are finished
	 pthread_exit (NULL);
	 return 0;
	}
    }
 dbase.sqldisconn();
 if (debug>0) ULOGW ("[tek] tekon thread end");
 tek_thread=FALSE;	// we are finished
 pthread_exit (NULL); 
}
//--------------------------------------------------------------
//	Creates server sock and binds to ip address and port
//--------------------------------------------------------------
INT StartWebServer()
{
 INT s, rv;
 if (debug>2) ULOGW ("[tek-gsm] create socket [%s]",UDP_PORT);
 //struct hostent *host=gethostbyname (hostname);
 s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
 if (s==-1)
    {
     ULOGW ("[tek-gsm] error > can't open socket %d",UDP_PORT);
     return (0);
    }
 struct addrinfo hints, *servinfo, *p;
 memset(&hints, 0, sizeof hints);
 hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
 hints.ai_socktype = SOCK_DGRAM;
 hints.ai_flags = AI_PASSIVE; // use my IP
 if ((rv = getaddrinfo(NULL, UDP_PORT, &hints, &servinfo)) != 0)
    {
	ULOGW ("[tek-gsm] getaddrinfo: %s",gai_strerror(rv));
        return 1;
    }
 // sockaddr_in si;
 si.sin_family = AF_INET;
 si.sin_port = htons(atoi(UDP_PORT));		// port
 si.sin_addr.s_addr = htonl(INADDR_ANY);

 DWORD vala = 6000;
 setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &vala, sizeof (DWORD));
 
 if (debug>2) ULOGW ("[tek-gsm] bind socket %d [%s]",s,UDP_PORT);
 if (bind(s,(const sockaddr *) &si,sizeof(si)))
	{
 	 if (debug>1) ULOGW ("[tek-gsm] error in bind(%d)",errno);
	 close(s);
	 sleep (1);
	 return(0);
	}
	
 return(s);
}
//--------------------------------------------------------------
void *get_in_addr(struct sockaddr *sa)
{
 if (sa->sa_family == AF_INET) 
    {
     return &(((struct sockaddr_in*)sa)->sin_addr);
    }
 return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
//--------------------------------------------------------------
// WaitForClientConnections()
//		Loops forever waiting for client connections. On connection
//		sta'rts a thread to handling the http transaction
//--------------------------------------------------------------
INT WaitForClientConnections(INT server_socket)
{
// INT client_socket;
 INT size;
// struct sockaddr_in client_address;
 struct sockaddr *cp;
 BYTE receivebuffer[1024];
 BYTE sendbuffer[1024];
 char s[INET6_ADDRSTRLEN];
 socklen_t addr_len = sizeof so[0];
 pthread_t thr;
 INT ret=0,count=2;
 UINT devtype=0,id=0;
 CHAR iid[10];

 sleep (1);
 while (count)
    {
     if (debug>2) ULOGW ("[tek-gsm] listen incoming connection (%d)",server_socket);
     size = recvfrom(server_socket,receivebuffer,1000, 0, (struct sockaddr *)&so[current_sock_max], &addr_len);
     if (size==-1) ULOGW("[tek-gsm] error in recvfrom");
     else
	{
	 devtype=receivebuffer[9]*0x100+receivebuffer[8];
	 sprintf (iid,"%x",receivebuffer[11]*0x100+receivebuffer[10]);
	 id=atoi(iid);
         if (debug>1) ULOGW("[tek-gsm] listener: got packet from %s devtype[%x] ID[%d]", inet_ntop(so[current_sock_max].sin_family, get_in_addr((struct sockaddr *)&so[current_sock_max]), s, sizeof s),devtype,id);	 
	 strcpy (address[current_sock_max],inet_ntop(so[current_sock_max].sin_family, get_in_addr((struct sockaddr *)&so[current_sock_max]), s, sizeof s));
	 for (UINT d=0;d<size;d++)
            { 
	     //if (debug>0) ULOGW ("[tek-gsm] [%d] receivebuffer [%x][%c]",d, receivebuffer[d], receivebuffer[d]);
            } 
    	 FindTekon (address[current_sock_max], id, current_sock_max);
	 //current_sock_max++;
	 if (debug>1) ULOGW("[tek-gsm] sendto (%s) (%d)",address[current_sock_max],size);
         sendto(server_socket, receivebuffer,1000, 0, (struct sockaddr *)&so[current_sock_max], addr_len);
	 current_sock_max++;
        }
     count--;
    }
}
//--------------------------------------------------------------
VOID* HandleICMPRequest (VOID *data)
{
 struct iphdr* ip;
 struct iphdr* ip_reply;
 struct icmphdr* icmp;
 struct sockaddr_in connection;
 char* packet;
 char* buffer;
 int sockfd;
 int optval;

 char dst_addr[15];
 char src_addr[15];
 socklen_t addr_len = sizeof so[0];
 char s[INET6_ADDRSTRLEN];

 ip = (struct iphdr*) packet;
 icmp = (struct icmphdr*) (packet + sizeof(struct iphdr));
    						      
 if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
    {
	perror("socket");
    }
 ULOGW("[tek-gsm] open socket for ICMP exchange");
 setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(int));
			        
// icmp->type			= ICMP_ECHO;
// icmp->code			= 0;
// icmp->un.echo.id		= 0;
// icmp->un.echo.sequence	= 0;
// icmp->checksum 		= 0;
// icmp->checksum			= in_cksum((unsigned short *)icmp, sizeof(struct icmphdr));
// ip->check			= in_cksum((unsigned short *)ip, sizeof(struct iphdr));

 strcpy (dst_addr,"62.165.35.90");
 strcpy (src_addr,"62.165.59.75");
 
 connection.sin_family = AF_INET;
 connection.sin_addr.s_addr = inet_addr(dst_addr);
 
// sendto(sockfd, packet, ip->tot_len, 0, (struct sockaddr *)&connection, sizeof(struct sockaddr));
// printf("Sent %d byte packet to %s\n", sizeof(packet), dst_addr);
 addr_len = sizeof(connection); 
 
 while (1)
    {
     ULOGW("[tek-gsm] ready for recieve ICMP packets");
     if (recvfrom(sockfd, buffer, sizeof(struct iphdr) + sizeof(struct icmphdr), 0, (struct sockaddr *)&connection, &addr_len) == -1)
        {
         perror("recv");
	 continue;
	}
     else
        {
         printf("[ICMP] Received %d byte reply from %s:\n", sizeof(buffer), inet_ntop(connection.sin_family, get_in_addr((struct sockaddr *)&connection), s, sizeof s));
         ip_reply = (struct iphdr*) buffer;
         printf("[ICMP] ID: %d | TTL: %d\n", ntohs(ip_reply->id), ip_reply->ttl);
        }    
     connection.sin_family = AF_INET;
     connection.sin_addr.s_addr = ip_reply->daddr; 
    
     ULOGW("[tek-gsm] send ICMP ping back");
     sendto(sockfd, buffer, sizeof(struct iphdr) + sizeof(struct icmphdr), 0, (struct sockaddr *)&connection, sizeof(struct sockaddr));
    }
}
//-----------------------------------------------------------------------------
// ReadDataCurrent - read single device. Readed data will be stored in DB
int DeviceTekonGSM::ReadDataCurrent (UINT  sens_num)
{
 UINT   rs;
 float  fl;
 BYTE   data[400];
 CHAR   date[20];
 this->qatt++;  // attempt
 
 if (!sens_num)
    {
     if (this->adr==1) send_tekon (CMD_SET_ACCESS_LEVEL, 0xf017, 0, 5);
     this->read_tekon(data);
     
     rs=send_tekon (CMD_READ_PARAM, 0xf017, 0, 0);
     if (rs)  rs = this->read_tekon(data); 
     if (rs)  sprintf (this->devtim,"%d%02d%02d",2000+(10*(data[3]>>4)+data[3]&0xf),(10*(data[2]>>4)+data[2]&0xf),(10*(data[1]>>4)+data[1]&0xf));
     rs=send_tekon (CMD_READ_PARAM, 0xf018, 0, 0);
     if (rs)  rs = this->read_tekon(data);
     if (rs)
        {
         sprintf (this->devtim+8,"%02d%02d%02d",(10*(data[3]>>4)+data[3]&0xf),(10*(data[2]>>4)+data[2]&0xf),(10*(data[1]>>4)+data[1]&0xf));
         sprintf (query,"UPDATE device SET lastdate=NULL,conn=1,devtim=%s WHERE id=%d",this->devtim,this->iddev);
         if (debug>2) ULOGW ("[tek] [%s]",query);
         res=dbase.sqlexec(query); if (res) mysql_free_result(res); 
        }
    }
 
 rs=send_tekon (CMD_READ_PARAM, this->cur[sens_num], 0, 0);
 if (rs)  rs = this->read_tekon(data); 

 if (rs<5) 
    {
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>15) this->akt=0; // !!! 5 > normal quant from DB
     if (debug>2) ULOGW ("[tek] Events[%d]",((3<<28)|(TYPE_INPUTTEKON<<24)|SEND_PROBLEM));
     if (this->qerrors==16) Events ((3<<28)|(TYPE_INPUTTEKON<<24)|SEND_PROBLEM,this->device);
    }
 else
    { 
     this->akt=1;
     this->qerrors=0;
     this->conn=1;
     sprintf (this->lastdate,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,1+currenttime->tm_mon,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);
     fl=*(float*)(data);
     if (debug>2) ULOGW ("[tek] [%d] [%f]",sens_num,fl);
     if (fl) StoreData (this->device, this->prm[sens_num], this->pipe[sens_num], 0, fl);
    }
 return 0;
}
//-----------------------------------------------------------------------------
// ReadDataArchive - read single device. Readed data will be stored in DB
int DeviceTekonGSM::ReadAllArchive (UINT  sens_num, UINT tp)
{
 bool   rs;
 BYTE   data[400];
 CHAR   date[20];
 UINT	month,year,index;
 float  value;
 UINT   code,vsk=0;
 time_t tims;
 tims=time(&tims);
 struct 	tm *prevtime;		// current system time 
 
 this->qatt++;  // attempt

 if (this->adr==2) send_tekon (CMD_READ_PARAM, 0xf017, 0, 2);
 this->read_tekon(data);
 usleep (25000);
 if (this->adr==2) send_tekon (CMD_READ_PARAM, 0xf017, 0, 3);
 this->read_tekon(data);

 //usleep (15000);
 //if (this->adr==2) send_tekon (CMD_READ_PARAM, 0xf017, 0, 4); 
 //usleep (25000);
 //this->read_tekon(data);

 if (this->n_hour[sens_num])
    {    
     prevtime=localtime(&tims); 	// get current system time
     if (prevtime->tm_year%4==0) vsk=0; else vsk=1;
     index=24*(((prevtime->tm_year-100)*365+(prevtime->tm_year-100)/4+prevtime->tm_yday+vsk)%64)+prevtime->tm_hour;
     //tims-=1*60*60;
     //if (debug>2) ULOGW ("[tekon] [%d] [%f]",sens_num,value);
     if (index>tp*4) month=index-tp*4; else month=0;
     while (index>=month)
	 {
	  //usleep (8000);
     	  //rs = this->read_tekon(data);
     	  rs=send_tekon (CMD_READ_INDEX_PARAM, this->n_hour[sens_num], index, 1);
	  usleep (28000);
     	  rs = this->read_tekon(data);
          
          prevtime=localtime(&tims); 	// get current system time
	  if (rs)  sprintf (date,"%04d%02d%02d%02d0000",prevtime->tm_year+1900,prevtime->tm_mon+1,prevtime->tm_mday,prevtime->tm_hour);
	  if (currenttime->tm_year<prevtime->tm_year || currenttime->tm_year<prevtime->tm_year) break;	
	  if (rs)  value=*(float*)(data); 
          if (rs)  if (debug>2) ULOGW ("[tek] [1] [%d] [%d] [%f]",index,sens_num,value);
          if (rs)  StoreData (this->device, this->prm[sens_num], this->pipe[sens_num], 1, 0, value, date);
	  if (index==0) break;
	  tims-=60*60;
	  index--;
	 }
    }

 if (this->n_day[sens_num])
    {     
     tims=time(&tims);
     //tims-=24*60*60;
     prevtime=localtime(&tims); 	// get current system time
     index=prevtime->tm_yday-1;
     if (index>tp) month=index-tp; else month=0;

     while (index>month)
	 {
     	  rs=send_tekon (CMD_READ_INDEX_PARAM, this->n_day[sens_num], index, 1);
	  usleep (20000);
	  rs = this->read_tekon(data);
	  
	  prevtime=localtime(&tims); 	// get current system time
	  if (rs)  sprintf (date,"%04d%02d%02d000000",+1900,prevtime->tm_mon+1,prevtime->tm_mday);
	  if (currenttime->tm_year<prevtime->tm_year || currenttime->tm_year<prevtime->tm_year || index>365) break;	
	  if (rs)  value=*(float*)(data); 
	  if (rs)  if (debug>2) ULOGW ("[2] [%d] [%d] [%f]",sens_num,index,value);
	  if (rs)  StoreData (this->device, this->prm[sens_num], this->pipe[sens_num], 2, 0, value, date);
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
	  if (rs)  value=*(float*)(data); 
	  if (rs)  if (debug>2) ULOGW ("[4] [%d] [%d] [%f] [%s]",sens_num,index,value,date);
	  if (rs)  StoreData (this->device, this->prm[sens_num], this->pipe[sens_num], 4, 0, value, date);
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
int DeviceTekonGSM::ReadDataArchive (UINT  sens_num)
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
     if (rs)  StoreData (this->device, this->prm[sens_num], this->pipe[sens_num], 1, 0, value, date);
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
     if (rs)  StoreData (this->device, this->prm[sens_num], this->pipe[sens_num], 2, 0, value, date);
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
     if (rs)  StoreData (this->device, this->prm[sens_num], this->pipe[sens_num], 4, 0, value, date);
    }

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
    {
     sprintf (query,"SELECT * FROM dev_tekon WHERE idd=%d",dev[d].idd);
     res=dbase.sqlexec(query); 
     UINT nr=mysql_num_rows(res);
     for (UINT r=0;r<nr;r++)
	{
         row=mysql_fetch_row(res);
	 tekon[tekon_num].idt=tekon_num;
         if (!r)
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
            }    
	 tekon[tekon_num].pipe[r]=atoi(row[1]);
         tekon[tekon_num].cur[r]=atoi(row[2]);
         tekon[tekon_num].prm[r]=atoi(row[3]);
         tekon[tekon_num].n_hour[r]=atoi(row[4]);
         tekon[tekon_num].n_day[r]=atoi(row[5]);
         tekon[tekon_num].n_month[r]=atoi(row[6]);
	 chan_num[tekon_num]++;     
        } 
     if (debug>0) ULOGW ("[tek] device [0x%x],adr=%d total %d channels",tekon[tekon_num].device,tekon[tekon_num].adr, chan_num[tekon_num]);
     tekon_num++;
    }
 if (res) mysql_free_result(res);
}
//--------------------------------------------------------------------------------------
BOOL FindTekon (CHAR* addr, INT id, INT conn)
{
 tekon_num=0;
 for (UINT d=0;d<device_num;d++)
 if (dev[d].type==TYPE_INPUTTEKON)
    {
     if (dev[d].port==id)
        {
	 //if (debug>0) ULOGW ("[tek] [%d]/[%d] addr=%s",d,device_num,addr);
         strcpy (tekon[d].number,addr);
	 strcpy (dev[d].number,addr);
	 tekon[d].conn=conn;
	 if (debug>0) ULOGW ("[tek] [%d] device [%s][%d]",d, tekon[d].number, dev[d].id);
	 sprintf (query,"UPDATE device SET number=\"%s\" WHERE id=\"%d\"",addr,dev[d].id);
	 //if (debug>0) ULOGW ("[tek] %s",query);
	 dbase.sqlexec(query); 
	 if (res) mysql_free_result(res);
        } 
    }
 ULOGW ("[tek] exit findtekon");
// if (res) mysql_free_result(res);
}
//---------------------------------------------------------------------------------------------------
BOOL StoreData (UINT dv, UINT prm, UINT pipe, UINT status, FLOAT value)
{
 sprintf (query,"SELECT * FROM prdata WHERE type=0 AND prm=%d AND device=%d AND pipe=%d",prm,dv,pipe);
 res=dbase.sqlexec(query); 
 if (row=mysql_fetch_row(res)) sprintf (query,"UPDATE prdata SET value=%f,status=%d WHERE type=0 AND prm=%d AND device=%d AND pipe=%d",value,status,prm,dv,pipe);
 else 
    {
     sprintf (query,"INSERT INTO prdata(device,prm,type,value,status,pipe) VALUES('%d','%d','0','%f','%d','%d')",dv,prm,value,status,pipe);
     if (debug>2) ULOGW("[tek] %s",query);
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
	 if (debug>3) ULOGW("[tek] %s",query);
	 if (res) mysql_free_result(res);
	 res=dbase.sqlexec(query);
	}
     if (res) mysql_free_result(res);
     return true;     
    }
 else sprintf (query,"INSERT INTO data(flat,prm,type,value,status,date,source) VALUES('0','%d','%d','%f','%d','%s','%d')",prm,type,value,status,data,pipe); 
 if (debug>2) ULOGW("[tek] %s",query);

 if (res) mysql_free_result(res);
 res=dbase.sqlexec(query); 
 if (res) mysql_free_result(res);
 return true;
}
//-----------------------------------------------------------------------------
BOOL DeviceTekonGSM::send_tekon (UINT op, UINT prm, UINT index, UINT frame)
    {
     UINT       crc=0;          //(* CRC checksum *)
     UINT       nbytes = 0;     //(* number of bytes in send packet *)
     BYTE       data[100];      //(* send sequence *)
     BYTE	sn[2];
     socklen_t addr_len = sizeof so[0];
     this->adr=2;
     
     if (frame==0) // for 3 bytes
        {          // 10 4D 01 01 17 F0 00 56 16        
	 // 10 40 01    01 8F 81 00 52 16 (1)
	 // 10 40 01 11 02 6F 80 43 16 (>1) 
         data[0]=0x10; 
         data[1]=0x4D;
         if (this->adr==1)
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
         if (debug>2) ULOGW("[tek-gsm] sendto [0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
	 sendto(server_socket, data, 100, 0, (struct sockaddr *)&so[this->idt], addr_len);
        }     
     if (frame==1) // for 4 bytes only
        {          // 68 07 07 68 46 01 15 01 09 FF 00 65 16
	// 68 a a 68 40 1
         data[0]=0x68; 
         data[1]=0x7;
         data[2]=0x7;
         data[3]=0x68;
         data[4]=0x46;
         data[5]=this->adr&0xFF;
         if (this->adr==1)
	    	{			 
	         data[5]=this->adr&0xFF;
             	 data[6]=(CHAR)op;       
	         data[7]=prm&0xff;
        	 data[8]=(prm&0xff00)>>8;
	         data[9]=index&0xff;
		 data[10]=(index&0xff00)>>8;
	         data[11]=CRC (data+4, 7);
        	 data[12]=0x16;	   
        	 if (debug>2) ULOGW("[tek-gsm] sendto [0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
		 sendto(server_socket, data, 100, 0, (struct sockaddr *)&so[this->idt], addr_len);
	         //write (fd,&data,13);
		}
	 else
	    	{
	         data[0]=0x68;
	         data[1]=0x0a;
	         data[2]=0x0a;
	         data[3]=0x68;		 
		 
	         data[4]=0x40;		 
		 data[5]=0x1;
             	 data[6]=CMD_WRITE_PARAM_SLAVE;
	         data[7]=0x6;
        	 data[8]=0x2;		 
	         data[9]=0x8;
	         data[10]=prm&0xff;
        	 data[11]=(prm&0xff00)>>8;
	         data[12]=index&0xff;
		 data[13]=(index&0xff00)>>8;
		 //write (fd,&data,14);
		 
	         data[14]=CRC (data+4, 10);
        	 data[15]=0x16;
        	 if (debug>2) ULOGW("[tek-gsm] sendto [0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15]);
		 //usleep(10000);
		 sendto(server_socket, data, 100, 0, (struct sockaddr *)&so[this->idt], addr_len);
        	 //write (fd,data,16);
		}
	}
     if (frame==2)
        {
         data[0]=0x68; 
         data[1]=0x7;
         data[2]=0x7;
         data[3]=0x68;
	 
         data[4]=0x40;
         data[5]=0x01;
         data[6]=0x14;
         data[7]=0x03;
         data[8]=0x02;
         data[9]=0x05;
         data[10]=0x02;
         data[11]=0x61;
         data[12]=0x16;
	 if (debug>2) ULOGW("[tek-gsm] sendto[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12]);
	 //write (fd,&data,13);
	 ULOGW("[tek-gsm] sendto");	 
	 sendto(server_socket, data, 100, 0, (struct sockaddr *)&so[this->idt], addr_len);
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
	 if (debug>2) ULOGW("[tek] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
	 //write (fd,&data,9);
	 sendto(server_socket, data, 100, 0, (struct sockaddr *)&so[this->idt], addr_len);
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
	 if (debug>2) ULOGW("[tek] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
	 //write (fd,&data,9);
	 sendto(server_socket, data, 100, 0, (struct sockaddr *)&so[this->idt], addr_len);
        }     
     if (frame==5)
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
	 if (debug>2) ULOGW("[tek] wr[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
	 //write (fd,&data,9);
	 sendto(server_socket, data, 100, 0, (struct sockaddr *)&so[this->idt], addr_len);
        }     

     return true;     
    }
//-----------------------------------------------------------------------------    
UINT  DeviceTekonGSM::read_tekon (BYTE* dat)
    {
     UINT       crc=0;          //(* CRC checksum *)
     INT        nbytes = 0;     //(* number of bytes in recieve packet *)
     INT        bytes = 0;      //(* number of bytes in packet *)
     BYTE       data[300];      //(* recieve sequence *)
     UINT       i=0;            //(* current position *)
     UCHAR      ok=0xFF;        //(* flajochek *)
     CHAR       op=0;           //(* operation *)
     socklen_t addr_len = sizeof so[0];

     if (debug>1) ULOGW("[tek-gsm] read tekon");
     usleep (8000);
     nbytes = recvfrom(server_socket,data, 300, 0, (struct sockaddr *)&so[this->idt], &addr_len);
     if (debug>2) ULOGW("[tek] nbytes=%d %x",nbytes,data[0]);
     //  0  1  2  3  4  5  6  7  8  9 10 11 12 13
     // 10 00 01 02 00 00 00 03 16
     // 68 08 08 68 46 01 15 29 F0 B0 01 08 2E 16 
     if (nbytes>5)
        {
         if (debug>3) ULOGW("[tek] [%d][0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]",nbytes,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);  
         if (data[0]==0x10)
            {
             crc=CRC (data, 7);
             if (crc==data[7]) ok=0;
             memcpy (dat,data+0x3,4);
             return nbytes;
            }
         if (data[0]==0x68)
            {
             crc=CRC (data, 12);
             if (crc==data[12]) ok=0;
             if (data[1]==data[2]) ok=0;
             memcpy (dat,data+0x6,data[1]);
             return nbytes;
            }
         return 0;
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
//-----------------------------------------------------------------------------
unsigned short in_cksum(unsigned short *addr, int len)
{
 register int sum = 0;
 u_short answer = 0;
 register u_short *w = addr;
 register int nleft = len;
 
 while (nleft > 1)
    {
      sum += *w++;
      nleft -= 2;
      }

 if (nleft == 1)
      {
      *(u_char *) (&answer) = *(u_char *) w;
      sum += answer;
      }
 sum = (sum >> 16) + (sum & 0xffff);		/* add hi 16 to low 16 */
 sum += (sum >> 16);				/* add carry */
 answer = ~sum;				/* truncate to 16 bits */
 return (answer);
}


													  