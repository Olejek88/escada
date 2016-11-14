//-------------------------------------------------------------------------------------
#include "types.h"
#include "main.h"
#include "errors.h"
#include "crq.h"
#include "irp.h"
#include "lk.h"
#include "db.h"
//#include "eval_flats.h"

#include "sys/types.h"
#include "sys/socket.h"
#include "version/version.h"

#define HTTP_PORT	4881	// http-port
#define HOMEDIRFIL 	"/home/user"
#define COMM_BUFFER_SIZE 1024
//--------------------------------------------------------------------
static	MYSQL_RES *res;
static	db	dbase;
static 	MYSQL_ROW row;
//--------------------------------------------------------------------
extern 	"C" UINT device_num;	// total device quant
extern 	"C" UINT lk_num;	// LK sensors quant
extern 	"C" UINT bit_num;	// BIT sensors quant
extern 	"C" UINT ip2_num;	// 2IP sensors quant
extern 	"C" BOOL WorkRegim;	// work regim flag
extern 	"C" tm *currenttime;	// current system time
extern 	"C" UINT mee_num;	// MEE sensors quant
extern 	UINT 	flat_num;	// flat quant

extern 	"C" DeviceR 	dev[MAX_DEVICE];	// device class
extern 	"C" DeviceLK 	lk[MAX_DEVICE_LK];	// local concantrator class
extern 	"C" DeviceBIT 	bit[MAX_DEVICE_BIT];	// BIT class
extern 	"C" Device2IP 	ip2[MAX_DEVICE_2IP];	// 2IP class
extern 	"C" DeviceMEE 	mee[MAX_DEVICE_MEE];	// 2IP class
extern 	Flats 	flat[MAX_FLATS];
extern  "C" UINT	debug;

static CHAR 	query[500];
static CHAR	buf[300];
//-----------------------------------------------------------------------------
CHAR  ver_time[50];
CHAR  ver_desc[100];
extern UINT channum;
extern BOOL crq_thread;

static struct tm *ttime;
static struct tm *pttime;
static struct tm bt, et; 

struct answer
{
 UINT	n1;
 UINT	n2; 
 UINT	d1;
 UINT	d2; 
 CHAR	begin[20];
 CHAR	end[20]; 
 UINT	type;
 UINT	interval;
} ans;
//-----------------------------------------------------------------------------
INT  server_socket;
INT  http_cln_socket;
CHAR wwwroot[250];
static FILE *Dt;
//-----------------------------------------------------------------------------
VOID ULOGW (const CHAR* string, ...);
BOOL ParseNChan (CHAR *pdest);
BOOL hConnection (CHAR tP, INT _s_port, INT sck, INT cli);
//-----------------------------------------------------------------------------
VOID *StartHttpSrv (VOID* par)
{
 ULOGW ("[https] ARK communicator (linux version) started");
  
 BOOL eC = hConnection (1, HTTP_PORT, server_socket, http_cln_socket);
 crq_thread=false;
}
//------------------------------------------------------------------------------
#define SMALL_BUFFER_SIZE 10
//--------------------------------------------------------------
struct HTTPRequestHeader
	{
	 CHAR method[SMALL_BUFFER_SIZE];
	 CHAR url[250];
	 CHAR filepathname[250];
	 CHAR httpversion[SMALL_BUFFER_SIZE];
         in_addr client_ip;
	};
struct ClientInfo
	{
	 INT client_socket;
	 in_addr client_ip;
	};
struct MimeAssociation
	{
	 CHAR *file_ext;
	 CHAR *mime;
	};
//---------------------------------------------------------------
INT StartWebServer();
INT WaitForClientConnections(INT server_socket);
VOID* HandleHTTPRequest(VOID *data );
BOOL ParseHTTPHeader(CHAR *receivebuffer, HTTPRequestHeader &requestheader);
VOID OutputHTTPError(INT client_socket, INT statuscode);
INT SocketRead(INT client_socket,CHAR *receivebuffer,INT buffersize);
//---------------------------------------------------------------
BOOL hConnection (CHAR tP, INT _s_port, INT sck, INT cli)
{
 // get name of webserver machine. needed for redirects
 // gethostname does not return a fully qualified host name
 dbase.sqlconn("dk","root","");			// connect to database

 ULOGW ("[https] start web server");
 sck = StartWebServer();
 if (sck)
	{
         WaitForClientConnections(sck);
	 close(sck);
	}
 else ULOGW ("[https] Error in StartWebServer()");
 return 0;
}
//--------------------------------------------------------------
// WaitForClientConnections()
//		Loops forever waiting for client connections. On connection
//		sta'rts a thread to handling the http transaction
//--------------------------------------------------------------
INT WaitForClientConnections(INT server_socket)
{
 INT client_socket;
 struct sockaddr_in client_address;
 struct sockaddr *cp;
 pthread_t thr;
 INT client_address_len,ret=0;
 cp=(struct sockaddr *)&client_address;
 client_address_len = sizeof(client_address);
 if (debug>2) ULOGW ("[https] listen incoming connection (%d)",server_socket);
 if ((ret=listen(server_socket,1)) == -1)
	{
	 ULOGW ("[https] error in listen");
	 close (server_socket);
	 return(0);
	}
 // loop forever accepting client connections. user ctrl-c to exit!
 while (1)
	{
 	 client_socket = accept(server_socket,cp,(socklen_t *)&client_address_len);
	 if (debug>2) ULOGW ("[https] client_socket create [%p]",client_socket);
	 if (client_socket == -1)
		{
		 if (debug>1)  ULOGW("[https] Error in accept()");
		 close(server_socket);
		 return(0);
		}
         // copy client ip and socket so the HandleHTTPRequest thread and process the request.
	 // for each request start a new thread!	 
	 if(pthread_create(&thr,NULL,&HandleHTTPRequest,(void *)client_socket) != 0) 
                ULOGW ("[https] error create Ecom-3000 handle request thread");
	 else  if (debug>2) ULOGW ("[https] create CRQ thread");
         if (thr) pthread_detach (thr);
	 //HandleHTTPRequest(&ci);
	 //ULOGW("close client_socket close [%p]",client_socket);
	 //close(client_socket);
	}
}
//--------------------------------------------------------------
// Executed in its own thread to handling http transaction
//--------------------------------------------------------------
VOID* HandleHTTPRequest (VOID *data)
{
 INT client_socket;
 HTTPRequestHeader requestheader;
 INT size;
 CHAR receivebuffer[1024];
 CHAR sendbuffer[1024];
 client_socket = (INT)data;
 ULOGW("[https] client socket (%p)",client_socket);
 size = SocketRead(client_socket,receivebuffer,1000);
 ULOGW("[https] socketread (%d)",size);
 if (size == -1 || size == 0)
	{
	 if (debug>1) ULOGW("[https] Error calling recv (%d)",size);
	 close(client_socket);
	 pthread_exit(0);
	}
 if (debug>1) ULOGW("[https] recv %s (%d)",receivebuffer,size);
 receivebuffer[size] = 0;
 if (!ParseHTTPHeader(receivebuffer,requestheader))
	{
	 OutputHTTPError(client_socket, 400);   // 400 - bad request
	 pthread_exit(0);
	}
 if (strstr(requestheader.method,"GET"))
	{
	 // send the http header and the file contents to the browser
	 strcpy(sendbuffer,"HTTP/1.0 200 OK\r\n");
	 // /crq?req=version
	 if (requestheader.url[1]=='d' && requestheader.url[2]=='k')
		{
		 CHAR *pdest;
		 pdest = strstr(requestheader.url,"req=");
		 if(pdest!=NULL)
			{
			 CHAR buffer[150000];
			 BOOL rightreq=false;
			 //printf ("%s\n",pdest);
		         if (strstr (pdest,"version"))
			    	{
				 rightreq=true;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
				 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 sprintf (buffer,"%s %s",version,buildtime);
				 sprintf(sendbuffer+strlen(sendbuffer),"%s\r\n",buffer);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 if (debug>2) ULOGW ("[https] %s",buffer);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"gettime"))
				{
				 rightreq=true;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 struct tm *pttime;
				 time_t tim;
				 tim=time(&tim);
			         pttime=localtime(&tim);
				 sprintf (buffer,"%02d-%02d-%04d %02d:%02d:%02d.%03d",pttime->tm_mday,pttime->tm_mon+1,pttime->tm_year+1900,pttime->tm_hour,pttime->tm_min,pttime->tm_sec,0);
				 sprintf(sendbuffer+strlen(sendbuffer),"%s\r\n",buffer);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 if (debug>2) ULOGW ("[https] %s",buffer);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"settime"))
				{
				 rightreq=true;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
				 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 if (strstr (pdest+6,"&time="))
					{
					 strncpy (buffer,pdest+17,20);
					}
				 else OutputHTTPError(client_socket, 400);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"signal"))
				{
				 rightreq=true;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
				 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 ULOGW ("[https] signalisation data");
				 buffer[0]=buffer[46]=buffer[47]=0;
				 //Dt =  fopen("/olog.dat","r");
//				 if (Dt) fread (buffer,sizeof(char),47,Dt);
//				 else ULOGW ("[https] file open error %p",Dt);
				 //ULOGW ("[https] %s",buffer);
//				 if (Dt) fclose (Dt);
				 ULOGW ("[https] signalisation end");
//				 if (Dt) snprintf(sendbuffer+strlen(sendbuffer),47,"%s\r\n",buffer);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				}

			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"reboot"))
				{
				 if (strstr (pdest+5,"&init="))
					{
					 rightreq=true;
					 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
					 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
					 ULOGW ("[https] recieve reboot command");
					 sprintf (buffer,"recieve reboot command");
					 sprintf(sendbuffer+strlen(sendbuffer),"%s\r\n",buffer);
					 send(client_socket,sendbuffer,strlen(sendbuffer),0);
					 ULOGW ("[https] reboot after 5 sec"); sleep (1);
					 ULOGW ("[https] reboot after 4 sec"); sleep (1);
					 ULOGW ("[https] reboot after 3 sec"); sleep (1);
					 ULOGW ("[https] reboot after 2 sec"); sleep (1);
					 ULOGW ("[https] reboot after 1 sec"); sleep (1);
					 ULOGW ("[https] reboot");
					 //system ("killall grep awx | grep kernel");
					 //exec reboot;
					}
				 else OutputHTTPError(client_socket, 400);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"list_dev"))
				{
				 rightreq=true;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 sprintf (buffer,"Device,Port,Speed,Adr,Type,Flat,Akt,Qatt,Qerrors,Conn,Devtim,Name,\r\n");
			         for (UINT r=0;r<device_num;r++)
			            {
				     if (dev[r].type>10) sprintf (buffer+strlen(buffer),"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,\r\n",
				     dev[r].idd, dev[r].port, dev[r].speed, dev[r].adr,
				     dev[r].type,dev[r].flat, dev[r].akt, dev[r].qatt,
				     dev[r].qerrors, dev[r].conn, dev[r].devtim, dev[r].name);
			             //send(client_socket,buffer,strlen(buffer),0);
				    }
				 send(client_socket,buffer,strlen(buffer),0);
				 if (debug>3) ULOGW ("[https] %s",buffer);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"exit") || strstr (pdest,"quit"))
				{
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 sprintf (buffer,"now server end work. bye.\r\n");
				 send(client_socket,buffer,strlen(buffer),0);
				 exit(0);
				 rightreq=true;
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"current_dev"))
				{
				 rightreq=true;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 sprintf (buffer,"Device,Prm,Value,Status,Pipe,\r\n");
				 send(client_socket,buffer,strlen(buffer),0);

				 ParseNChan (pdest);
				 ULOGW ("[%d|%d]",ans.n1,ans.n2);
				 sprintf (query,"SELECT * FROM prdata WHERE type=0 AND device>80000000 AND device>=%d AND device<=%d ORDER BY device DESC",ans.n1,ans.n2);
				 ULOGW ("[%s]",query);
				 res=dbase.sqlexec(query);
				 if (res)
				    { 
				     UINT nr=mysql_num_rows(res);
				     for (UINT r=0;r<nr;r++)
				        {
					 row=mysql_fetch_row(res);
    				         sprintf (buffer,"%d,%d,%f,%d,%d,\r\n",atoi(row[1]),atoi(row[2]),atof(row[5]),atoi(row[6]),atoi(row[7]));
    			            	 send(client_socket,buffer,strlen(buffer),0);					 
					}
				     if (res) mysql_free_result(res);
				    }
				 else 
					{
					 ULOGW("[https] reconnect");
					 dbase.sqldisconn();
					 dbase.sqlconn("dk","root","");			// connect to database
					}
				 if (debug>1) ULOGW("[https] %s",buffer);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"archive"))
				{
				 rightreq=true;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);

				 ParseNChan (pdest);				 
				 ULOGW ("[%d|%d]",ans.n1,ans.n2);
				 if (ans.type==1) sprintf (buffer,"Device,Time,Prm,Type,Value,Status,Pipe,\r\n");
				 if (ans.type==2) sprintf (buffer,"Flat,Time,Prm,Type,Value,Status,Source,\r\n");
				 send(client_socket,buffer,strlen(buffer),0);
				 if (ans.d1!=0 && ans.d2!=5)
				    {
    				     if (ans.type==1) sprintf (query,"SELECT * FROM prdata WHERE type=%d AND date>=%s AND date<=%s AND device>=%d AND device<=%d ORDER BY device,date",ans.interval,ans.begin,ans.end,ans.n1,ans.n2);
				     if (ans.type==2) sprintf (query,"SELECT * FROM data WHERE type=%d AND date>=%s AND date<=%s AND flat>=%d AND flat<=%d ORDER BY flat,date",ans.interval,ans.begin,ans.end,ans.n1,ans.n2);
				    }
				 else
				    {
    				     if (ans.type==1) sprintf (query,"SELECT * FROM prdata WHERE id>=%d AND id<=%d ORDER BY device,date",ans.d1,ans.d2);
				     if (ans.type==2) sprintf (query,"SELECT * FROM data WHERE id>=%d AND id<=%d ORDER BY flat,date",ans.d1,ans.d2);				     
				    }   
				 ULOGW ("[%s]",query);
				 res=dbase.sqlexec(query);
				 if (res)
				    { 
				     UINT nr=mysql_num_rows(res);
				     for (UINT r=0;r<nr;r++)
				        {
					 row=mysql_fetch_row(res);
    				         if (ans.type==1) sprintf (buffer,"%d,%s,%d,%d,%f,%d,%d,\r\n",atoi(row[1]),row[4],atoi(row[2]),atoi(row[3]),atof(row[5]),atoi(row[6]),atoi(row[7]));
    				         if (ans.type==2) sprintf (buffer,"%d,%s,%d,%d,%f,%d,%d,\r\n",atoi(row[4]),row[2],atoi(row[8]),atoi(row[1]),atof(row[3]),atoi(row[5]),atoi(row[6]));
    			            	 send(client_socket,buffer,strlen(buffer),0);					 
					}
				     if (res) mysql_free_result(res);		 				 			
				    }				 				 
				 if (debug>1) ULOGW("[https] %s",buffer);
				}

			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"current_flat"))
				{
				 rightreq=true;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 sprintf (buffer,"Flat,Prm,Value,Status,\r\n");
				 send(client_socket,buffer,strlen(buffer),0);

				 ParseNChan (pdest);				 
				 sprintf (query,"SELECT * FROM data WHERE type=0 AND flat>=%d AND flat<=%d ORDER BY flat,prm",ans.n1,ans.n2);
				 ULOGW ("[%s]",query);
				 res=dbase.sqlexec(query);
				 if (res)
				    { 
				     UINT nr=mysql_num_rows(res);
				     for (UINT r=0;r<nr;r++)
				        {
					 row=mysql_fetch_row(res);
    				         sprintf (buffer,"%d,%d,%f,%d,\r\n",atoi(row[4]),atoi(row[8]),atof(row[3]),atoi(row[5]));
    			            	 send(client_socket,buffer,strlen(buffer),0);					 
					}
				     if (res) mysql_free_result(res);		 				 			
				    }				 				 
				 if (debug>1) ULOGW("[https] %s",buffer);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"events"))
				{
				 rightreq=true;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 sprintf (buffer,"Device,Time,Code,Descr,\r\n");
				 send(client_socket,buffer,strlen(buffer),0);
				 
				 ParseNChan (pdest);				 
				 sprintf (query,"SELECT * FROM register WHERE date>=%s AND date<=%s AND device>=%d AND device<=%d ORDER BY date,device DESC",ans.begin,ans.end,ans.n1,ans.n2);
				 ULOGW ("[%s]",query);
				 res=dbase.sqlexec(query);
				 if (res)
				    { 
				     UINT nr=mysql_num_rows(res);
				     for (UINT r=0;r<nr;r++)
				        {
					 row=mysql_fetch_row(res);
    				         sprintf (buffer,"%d,%d,%d,no descr,\r\n",atoi(row[2]),atoi(row[3]),atof(row[1]));
    			            	 send(client_socket,buffer,strlen(buffer),0);					 
					}
				     if (res) mysql_free_result(res);		 				 			
				    }				 				 
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"dev_info"))
				{
				 UINT nfl=0, ndt=0, npr=0, ndv=0, mpr=0, mdt=0;
				 rightreq=true;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 sprintf (query,"SELECT COUNT(id) FROM flats");
				 res=dbase.sqlexec(query);
				 if (res) { row=mysql_fetch_row(res); nfl=atoi(row[0]); }
				 sprintf (query,"SELECT COUNT(id),MAX(id) FROM data");
				 res=dbase.sqlexec(query);
				 if (res) { row=mysql_fetch_row(res); ndt=atoi(row[0]); mdt=atoi(row[1]);}
				 sprintf (query,"SELECT COUNT(id),MAX(id) FROM prdata");
				 res=dbase.sqlexec(query);
				 if (res) { row=mysql_fetch_row(res); npr=atoi(row[0]); mpr=atoi(row[1]);}
				 sprintf (query,"SELECT COUNT(id) FROM devices");
				 res=dbase.sqlexec(query);
				 if (res) { row=mysql_fetch_row(res); ndv=atoi(row[0]); }
				 
				 sprintf (query,"SELECT * FROM device WHERE type=7");
				 res=dbase.sqlexec(query);
				 if (res)
				    { 
    				     row=mysql_fetch_row(res);
    				     sprintf (buffer,"NumFlats=%d\r\nDataVal=%d\r\nPrDataVal=%d\r\nNumDevice=%d\r\nQatt=%d\r\nQerrors=%d\r\nMaxPrdataID=%d\r\nMaxDataID=%d\r\n",nfl,ndt,npr,ndv,atoi(row[13]),atoi(row[14]),mpr,mdt);
    			             send(client_socket,buffer,strlen(buffer),0);
				     if (res) mysql_free_result(res);		 				 			
				    }				 				 
				}				
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"prm_info"))
				{
				 rightreq=true;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 sprintf (buffer,"Dt,Descr,Edizm,\r\n");
				 send(client_socket,buffer,strlen(buffer),0);

				 sprintf (query,"SELECT * FROM var");
				 res=dbase.sqlexec(query);
				 if (res)
				    { 
				     UINT nr=mysql_num_rows(res);
				     for (UINT r=0;r<nr;r++)
				        {
					 row=mysql_fetch_row(res);
    				         sprintf (buffer,"%d,%s,%d,\r\n",atoi(row[1]),row[2],atoi(row[3]));
    			            	 send(client_socket,buffer,strlen(buffer),0);					 
					}
				     if (res) mysql_free_result(res);		 				 			
				    }				 				 
				}				
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"flats_info"))
				{
				 rightreq=true;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 sprintf (buffer,"Flat,Level,Name,Square,Entrance,\r\n");
				 //send(client_socket,buffer,strlen(buffer),0);
				 
				 //ParseNChan (pdest);				 
			         for (UINT r=0;r<flat_num;r++)
			            {
				     sprintf (buffer+strlen(buffer),"%d,%d,%s,%f,%d,\r\n",
				     flat[r].flatd, flat[r].level, flat[r].name, flat[r].square, flat[r].ent);
			             //send(client_socket,buffer,strlen(buffer),0);
				    }
				 send(client_socket,buffer,strlen(buffer),0);
        			 if (debug>1) ULOGW("[https] %s",buffer);
				}				
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"edizm_info"))
				{
				 rightreq=true;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 sprintf (buffer,"ID,Name,Knt,\r\n");
				 send(client_socket,buffer,strlen(buffer),0);

				 sprintf (query,"SELECT * FROM edizm");
				 res=dbase.sqlexec(query);
				 if (res)
				    { 
				     UINT nr=mysql_num_rows(res);
				     for (UINT r=0;r<nr;r++)
				        {
					 row=mysql_fetch_row(res);
    				         sprintf (buffer,"%s,%s,%s,\r\n",row[0],row[1],row[2]);
    			            	 send(client_socket,buffer,strlen(buffer),0);					 
					}
				     if (res) mysql_free_result(res);
				    }				    			 				 
				}
			 //-----------------------------------------------------------------------------------
			} // no req
		 else OutputHTTPError(client_socket, 405);
		} // no crq
	 else OutputHTTPError(client_socket, 400);
	 if (debug>2) ULOGW ("[https] end get");
	} // no get
 else  OutputHTTPError(client_socket, 501);   // 501 not implemented
 if (client_socket) close(client_socket);
}
//--------------------------------------------------------------
// Reads data from the client socket until it gets a valid http
// header or the client disconnects.
//--------------------------------------------------------------
INT SocketRead(INT client_socket, CHAR *receivebuffer, INT buffersize)
{
 INT size=0,totalsize=0;
 do
    {
     size = recv(client_socket,receivebuffer+totalsize,buffersize-totalsize,0);
     if (size!=0 && size != -1)
    	{
	 totalsize += size;
 	 if (debug>2) ULOGW ("[https] recv (%s)",receivebuffer);
	 // are we done reading the http header?
	 if (strstr(receivebuffer,"\r\n\r\n")) break;
	}
     else totalsize = size;			// remember error state for return
    }
 while (size!=0 && size != -1);
 return(totalsize);
}
//--------------------------------------------------------------
// Sends an http header and html body to the client with error information.
//--------------------------------------------------------------
VOID OutputHTTPError(INT client_socket, INT statuscode)
{
 CHAR headerbuffer[1024];
 CHAR htmlbuffer[1024];
 sprintf(htmlbuffer,"<html><body><h2>Error: %d</h2></body></html>",statuscode);
 sprintf(headerbuffer,"HTTP/1.0 %d\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n",statuscode,strlen(htmlbuffer));
 send(client_socket,headerbuffer,strlen(headerbuffer),0);
 send(client_socket,htmlbuffer,strlen(htmlbuffer),0);
 if (debug>1) ULOGW ("[https] OutputHTTPError [%d]",statuscode);
 //close(client_socket);
}
//--------------------------------------------------------------
// Fills a HTTPRequestHeader with method, url, http version
// and file system path information.
//--------------------------------------------------------------
BOOL ParseHTTPHeader(CHAR *receivebuffer, HTTPRequestHeader &requestheader)
{
 CHAR *pos;
 // end debuggine
 pos = strtok(receivebuffer," ");
 if (pos == NULL) return(false);
 strncpy(requestheader.method,pos,SMALL_BUFFER_SIZE);
 pos = strtok(NULL," ");
 if (pos == NULL) return(false);
 strncpy(requestheader.url,pos,256);
 pos = strtok(NULL,"\r");
 if (pos == NULL) return(false);
 strncpy(requestheader.httpversion,pos,SMALL_BUFFER_SIZE);
 if (debug>2) ULOGW ("[https] recieve header [%s : %s] ver: %s",requestheader.method,requestheader.url,requestheader.httpversion);
 //ULOGW ("[%s : %s] ver: %s",requestheader.method,requestheader.url,requestheader.httpversion);
 if (debug>1) ULOGW ("[https] [%s]",requestheader.url);
 // based on the url lets figure out the filename + path
 strncpy(requestheader.filepathname,wwwroot,256);
 strncat(requestheader.filepathname,requestheader.url,256);
 return(true);
}
//--------------------------------------------------------------
//	Creates server sock and binds to ip address and port
//--------------------------------------------------------------
INT StartWebServer()
{
 INT s;
 if (debug>2) ULOGW ("[https] create socket");
 //struct hostent *host=gethostbyname (hostname);
 s = socket(PF_INET, SOCK_STREAM, 0);
 if (s==-1)
    {
     ULOGW ("[https] error > can't open socket %d",HTTP_PORT);
     return (0);
    }
 sockaddr_in si;
 si.sin_family = PF_INET;
 si.sin_port = htons(HTTP_PORT);		// port
 si.sin_addr.s_addr = htonl(INADDR_ANY);
 if (debug>2) ULOGW ("[https] bind socket %d [%d]",s,HTTP_PORT);
 if (bind(s,(const sockaddr *) &si,sizeof(si)))
	{
 	 if (debug>1) ULOGW ("[https] error in bind(%d)",errno);
	 close(s);
	 sleep (1);
	 return(0);
	}
 dbase.sqlconn("dk","root","");			// connect to database
 if (debug>0) ULOGW ("[https] connecting to database........success");
 sprintf (query,"set character_set_client='koi8r'"); dbase.sqlexec(query); 
 sprintf (query,"set character_set_results='koi8r'"); dbase.sqlexec(query);
 sprintf (query,"set collation_connection='koi8r_general_ci'"); dbase.sqlexec(query);

 return(s);
}
//--------------------------------------------------------------
BOOL ParseNChan (CHAR *pdest)
{
 CHAR type=0; CHAR *ddest; CHAR *dest; CHAR *sdest;
 UINT pp=0;   CHAR t1[20],t2[20];
 CHAR buffer[1000];
 if (debug>1) ULOGW ("[https] ParseNChan(%s)",pdest); 
 time_t tim;
 tim=time(&tim);
 ans.n1=0;
 ans.n2=0x55555555; 
 ans.d1=0;
 ans.d2=0x5; 
 localtime_r(&tim,&bt);
 localtime_r(&tim,&et);
 bt.tm_mday-=3;
 sprintf (ans.begin,"%4d%02d%02d%02d0000",bt.tm_year+1900,bt.tm_mon+1,bt.tm_mday,bt.tm_hour);
 sprintf (ans.end,"%4d%02d%02d%02d0000",et.tm_year+1900,et.tm_mon+1,et.tm_mday,et.tm_hour);
 ans.type=1;
  
 if (strstr (pdest+5,"&type=") || strstr (pdest+5,"&n1=") || strstr (pdest+5,"&n2="))
    {
     if (debug>1) ULOGW ("[https] archive values: t1=%s",t1);
     ddest = strstr (pdest+5,"&type=");
     if (strstr (ddest,"dev")) ans.type=1;
     if (strstr (ddest,"flats")) ans.type=2;
     
     ddest = strstr (pdest+5,"&n1=");
     if (ddest)
        {
         if (debug>1) ULOGW ("[https] ddest=%ld",ddest);
	 sdest = strstr (ddest+4,"&");
         if (debug>1) ULOGW ("[https] sdest=%ld",sdest);
	 if (sdest) 
	    {
	     strncpy (buffer,ddest+4,sdest-ddest+4);
             if (debug>1) ULOGW ("[https] buffer=%s",buffer);
	     ans.n1=(DWORD)atoi(buffer);
	    }
	}
     ddest = strstr (pdest+5,"&n2=");
     if (ddest)
        {
         if (debug>1) ULOGW ("[https] ddest=%ld",ddest);
	 sdest = strstr (ddest+5,"&");
         if (debug>1) ULOGW ("[https] sdest=%ld",sdest);
	 if (sdest) 
	    {
	     strncpy (buffer,ddest+4,sdest-ddest+4);
             if (debug>1) ULOGW ("[https] buffer=%s",buffer);
	     ans.n2=(DWORD)atoi(buffer);
	    }
	}

     ddest = strstr (pdest+5,"&d1=");
     if (ddest)
        {
         if (debug>1) ULOGW ("[https] ddest=%ld",ddest);
	 sdest = strstr (ddest+4,"&");
         if (debug>1) ULOGW ("[https] sdest=%ld",sdest);
	 if (sdest) 
	    {
	     strncpy (buffer,ddest+4,sdest-ddest+4);
             if (debug>1) ULOGW ("[https] buffer=%s",buffer);
	     ans.d1=(DWORD)atoi(buffer);
	    }
	}
     ddest = strstr (pdest+5,"&d2=");
     if (ddest)
        {
         if (debug>1) ULOGW ("[https] ddest=%ld",ddest);
	 sdest = strstr (ddest+5,"&");
         if (debug>1) ULOGW ("[https] sdest=%ld",sdest);
	 if (sdest) 
	    {
	     strncpy (buffer,ddest+4,sdest-ddest+4);
             if (debug>1) ULOGW ("[https] buffer=%s",buffer);
	     ans.d2=(DWORD)atoi(buffer);
	    }
	}
    }

 if ((dest=strstr (pdest+5,"&t1="))!=NULL)
    {
     sscanf (dest+4,"%[0-9]",t1);
     if (debug>1) ULOGW ("[https] archive values: t1=%s",t1);
     if (strlen(t1)<20) sprintf (ans.begin,"%s",t1);
    }
 if ((dest=strstr (pdest+5,"&t2="))!=NULL)
    {
     sscanf (dest+4,"%[0-9]",t2);
     if (debug>1) ULOGW ("[https] archive values: t2=%s",t2);
     if (strlen(t2)<20)sprintf (ans.end,"%s",t2);
    }

 ans.interval=1;
 if ((dest=strstr (pdest+5,"&interval="))!=NULL)
    {
     if (strstr (dest,"hour")) 	ans.interval=1;
     if (strstr (dest,"day")) 	ans.interval=2;
     if (strstr (dest,"month")) ans.interval=3;
     if (strstr (dest,"year")) 	ans.interval=4;
    }
 return true;
}
