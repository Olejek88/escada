//-------------------------------------------------------------------------------------
#include "types.h"
#include "main.h"
#include "pp7000.h"
#include "sys/types.h"
#include "sys/socket.h"

#define HTTP_PORT	4881	// http-port
#define HOMEDIRFIL "/home/user"
#define COMM_BUFFER_SIZE 1024
//--------------------------------------------------------------------
static MYSQL *mysql; 
static MYSQL_ROW row;
static MYSQL_RES *res;
//--------------------------------------------------------------------
extern  "C" DeviceR device[20];
extern	"C" chData  chd[200];
extern	"C" Channels  chan[200];
extern	"C" UINT channum;

static CHAR 	query[200];
static CHAR	buf[300];
//-----------------------------------------------------------------------------
SHORT ver_major=2;
SHORT ver_minor=11;
CHAR  ver_time[50];
CHAR  ver_desc[100];
CHAR  sernum[50];
CHAR  softver[50],wsoftver[50];
SHORT maxmodule=2,kA=0,wkA=0;

static struct tm *ttime;
static struct tm *pttime;
//SYSTEMTIME time1,time2,time3,st,fn;
//FILETIME fst,ffn;
//-----------------------------------------------------------------------------
//UCHAR sBuf1[7000],sBuf2[400];
//-----------------------------------------------------------------------------
INT  server_socket;
INT  http_cln_socket;
CHAR wwwroot[250];
RepExch rep;
//CHAR hostname[250];
//SHORT empt=0;
//-----------------------------------------------------------------------------
VOID ULOGW (CHAR* string, ...);
BOOL hConnection (CHAR tP, INT _s_port, INT sck, INT cli);
VOID formParam (VOID);
//-----------------------------------------------------------------------------
VOID *StartHttpSrv (VOID* par)
{
 ULOGW ("[https] ARK communicator v0.51.11 (linux version) started");
 if (debug>0) ULOGW ("[https] working on protocol CRQ 5.56 (january 2006)");

 formParam ();
 BOOL eC = hConnection (1, HTTP_PORT, server_socket, http_cln_socket);
}
//----------------------------------------------------------------------------
VOID formParam (VOID)
{
 ver_major=2;
 ver_minor=14;
 sprintf (ver_time,"31-01-2008 10:00:01");
 strcpy (ver_desc,"ARK communicator");
 strcpy (sernum,"ARK-3381-2S0A2E");
 strcpy (softver,"0.4.17");
 strcpy (wsoftver,"0.83.21");
 maxmodule=10;
 kA=0; wkA=16;
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
	 pthread_t thr;
	 if(pthread_create(&thr,NULL,&HandleHTTPRequest,(void *)client_socket) != 0) 
               ULOGW ("[https] error create Ecom-3000 handle request thread");
	 else  if (debug>2) ULOGW ("[https] create CRQ thread");

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
	 if (requestheader.url[1]=='c' && requestheader.url[2]=='r' && requestheader.url[3]=='q')
		{
		 CHAR *pdest;
		 pdest = strstr(requestheader.url,"req=");
		 if(pdest!=NULL)
			{
			 CHAR buffer[50000];
			 BOOL rightreq=FALSE;
			 //printf ("%s\n",pdest);
			 #define MAX_PARAMETRS 8
		         if (strstr (pdest,"version"))
			    	{
				 rightreq=TRUE;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
				 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 sprintf (buffer,"5 65 %s EKOM",ver_time);
			  	 //sprintf(sendbuffer+strlen(sendbuffer),"Content-Length: %ld\r\n",strlen(buffer));
				 sprintf(sendbuffer+strlen(sendbuffer),"%s\r\n",buffer);
				 //strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 if (debug>2) ULOGW ("[https] %s",buffer);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"gettime"))
				{
				 rightreq=TRUE;
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
				 rightreq=TRUE;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
				 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 if (strstr (pdest+6,"&time="))
					{
					 strncpy (buffer,pdest+17,20);
					}
				 else OutputHTTPError(client_socket, 400);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"getfile"))
				{
				 if (strstr (pdest+6,"&name="))
					{
					 rightreq=TRUE;
					 FILE *in;
					 CHAR *filebuffer;
					 long filesize;
					 if (strstr(buffer,".cfg") || strstr(buffer,".txt"))
						 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
					 else strncat(sendbuffer,"Content-Type: data/octet-stream\r\n",COMM_BUFFER_SIZE);
					 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
					 sprintf (buffer,HOMEDIRFIL,pdest+17);
					 if (debug>2) ULOGW ("[https] client request file: %s",buffer);
					 in = fopen(buffer,"rb");  // read binary
					 if (!in)
						{
						 if (debug>2) ULOGW ("[https] file not found\n");
						 OutputHTTPError(client_socket, 404);   // 404 - not found
						}
					 else 
						{
						 // determine file size
						 fseek(in,0,SEEK_END);
			 			 filesize = ftell(in);
						 fseek(in,0,SEEK_SET);
						 // allocate buffer and read in file contents
						 filebuffer = new char[filesize];
						 fread(filebuffer,sizeof(char),filesize,in);
						 fclose(in);
						 sprintf(sendbuffer+strlen(sendbuffer),"\r\nContent-Length: %ld\r\n",filesize);
						 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
			 			 send(client_socket,sendbuffer,strlen(sendbuffer),0);
						 send(client_socket,filebuffer,filesize,0);
						 delete [] filebuffer;
						}
					}
				 else OutputHTTPError(client_socket, 400);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"reboot"))
				{
				 if (strstr (pdest+6,"&init="))
					{
					 rightreq=TRUE;
					 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
					 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
					 strncpy (buffer,pdest+17,20);
					 if (!strcmp (buffer,"yes"))
						{
						 if (debug>2) ULOGW ("[https] reboot with archive erace no available");
						 sprintf (buffer,"reboot with archive erace no available");						 
						}
					 else
						{
						 ULOGW ("[https] recieve reboot command");
						 sprintf (buffer,"recieve reboot command");
						}
					 sprintf(sendbuffer+strlen(sendbuffer),"%s\r\n",buffer);
					 send(client_socket,sendbuffer,strlen(sendbuffer),0);
					 ULOGW ("[https] reboot after 5 sec"); sleep (1);
					 ULOGW ("[https] reboot after 4 sec"); sleep (1);
					 ULOGW ("[https] reboot after 3 sec"); sleep (1);
					 ULOGW ("[https] reboot after 2 sec"); sleep (1);
					 ULOGW ("[https] reboot after 1 sec"); sleep (1);
					 ULOGW ("[https] reboot");
					 system ("killall grep awx | grep kernel32");
					 //exec reboot;
					}
				 else OutputHTTPError(client_socket, 400);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"dev_info"))
				{
				 rightreq=TRUE;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 sprintf (buffer,"SerNum=%s\r\nSoftVer=%s\r\nMaxModule=%d\r\nA=%d\r\n",sernum,softver,maxmodule,kA+wkA);
				 send(client_socket,buffer,strlen(buffer),0);
				 if (debug>2) ULOGW ("[https] %s",buffer);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"exit") || strstr (pdest,"quit"))
				{
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 sprintf (buffer,"now server end work. bye.\r\n");
				 send(client_socket,buffer,strlen(buffer),0);
				 //system ("killall grep awx | grep kernel32");
				 exit(0);
				 rightreq=TRUE;
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"chan_info"))
				{
				 rightreq=TRUE;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
				 strncat(sendbuffer,"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1251\">\r\n",COMM_BUFFER_SIZE);\
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 if (debug>2) ULOGW("[https] %s",buffer);
				 for (int ch=0;ch<channum;ch++)
			    	    {
				     sprintf (buffer,"[%s]\r\n",chan[ch].descr);
    				     sprintf (buffer,"%sName=%s\r\nUnits=\r\nModule=%d\r\nNumInModule=%d\r\nFill=Fixed\r\nArc=NO\r\nSumShift=0\r\n",buffer,chan[ch].name,chan[ch].device,chan[ch].target%32);
				     sprintf (buffer,"%sMinLimit=%f\r\nMaxLimit=%f\r\nKbdControl=NO\r\n",buffer,chan[ch].minVh,chan[ch].maxVh);
				     //sprintf (buffer,"%sLastSession=%02d-%02d-%04d %02d:%02d:%02d\r\n",buffer,time1.wDay,time1.wMonth,time1.wYear,time1.wHour,time1.wMinute,time1.wSecond);
				     //sprintf (buffer,"%sArcPtr=%02d-%02d-%04d %02d:%02d:%02d\r\n",buffer,time2.wDay,time2.wMonth,time2.wYear,time2.wHour,time2.wMinute,time2.wSecond);
				     send(client_socket,buffer,strlen(buffer),0);
				    }
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"module_info"))
				{
				 rightreq=TRUE;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);
				 for (int ch=0;ch<channum;ch++)
			    	    {
				     sprintf (buffer,"[MODULE %d]\r\n",ch+1);
				     sprintf (buffer,"%sType=%s\r\n",buffer,chan[ch].name);
				     sprintf (buffer,"%sPort=Com%d\r\n",buffer,chan[ch].device);
				     sprintf (buffer,"%sAddr=%d\r\n",buffer,chan[ch].device);
				     sprintf (buffer,"%sSerNum=??\r\n",buffer);
				     sprintf (buffer,"%sState=1\r\n",buffer);
				     //sprintf (buffer,"%sLastSession=%02d-%02d-%04d %02d:%02d:%02d\r\n",buffer,time1.wDay,time1.wMonth,time1.wYear,time1.wHour,time1.wMinute,time1.wSecond);
				     //sprintf (buffer,"%sArcPtr=%02d-%02d-%04d %02d:%02d:%02d\r\n",buffer,time2.wDay,time2.wMonth,time2.wYear,time2.wHour,time2.wMinute,time2.wSecond);
				     send(client_socket,buffer,strlen(buffer),0);
				    }
				 if (debug>1) ULOGW("[https] %s",buffer);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"change") || strstr (pdest,"rights_info") || strstr (pdest,"start_time") || strstr (pdest," modules"))
				{
				 rightreq=TRUE;
				 if (debug>1) ULOGW ("[https] recieve not supported command");
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 sprintf (buffer,"command not supported\r\n");
				 send(client_socket,buffer,strlen(buffer),0);
				 //OutputHTTPError(client_socket, 400);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"connect"))
				{
				 rightreq=TRUE;
				 if (debug>1) ULOGW("[https] recieve connect command");
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
			 	 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 send(client_socket,sendbuffer,strlen(sendbuffer),0);	
				 sprintf(buffer,"Ok\r\n");
				 send(client_socket,buffer,strlen(buffer),0);
				 close(client_socket);
				 //poll device for instant data");
				 if (debug>2) ULOGW("[https] poll device for archive data");
				 pthread_exit(0);
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"current"))
				{
				 rightreq=TRUE;
				 strncat(sendbuffer,"Content-Type: text/plain\r\n",COMM_BUFFER_SIZE);
				 strncat(sendbuffer,"\r\n",COMM_BUFFER_SIZE);
				 CHAR type=0; CHAR *ddest;
				 UINT n1=0,n2=0,pp=0; UINT g[20]={0};
				 if (strstr (pdest+7,"&type=") || strstr (pdest+6,"&n1=") || strstr (pdest+6,"&n2="))
					{
					 ddest = strstr (pdest+7,"&type=");
					 if (ddest) type=ddest[6];
					 ddest = strstr (pdest+7,"&n1=");
					 if (ddest)
						{
						 if (*(ddest+4)>='0' && *(ddest+4)<='9')
							{
							 buffer[0]=*(ddest+4); buffer[1]=0;
							 if (*(ddest+5)>='0' && *(ddest+5)<='9') buffer[1]=*(ddest+5);
							 buffer[2]=0;
							}
						 else
						 if (*(ddest+5)>='0' && *(ddest+5)<='9')
							{
							 buffer[0]=*(ddest+5); buffer[1]=0;
							 if (*(ddest+6)>='0' && *(ddest+6)<='9') buffer[1]=*(ddest+6);
							 buffer[2]=0;
							}
						 n1=(SHORT)atoi(buffer);
						}
					 ddest = strstr (pdest+7,"&n2=");
					 if (ddest)
						{
						 if (*(ddest+4)>='0' && *(ddest+4)<='9')
							{
							 buffer[0]=*(ddest+4); buffer[1]=0;
							 if (*(ddest+5)>='0' && *(ddest+5)<='9') buffer[1]=*(ddest+5);
							 buffer[2]=0;
							}
						 else
						 if (*(ddest+5)>='0' && *(ddest+5)<='9')
							{
							 buffer[0]=*(ddest+5); buffer[1]=0;
							 if (*(ddest+6)>='0' && *(ddest+6)<='9') buffer[1]=*(ddest+6);
							 buffer[2]=0;
							}
						 n2=(SHORT)atoi(buffer);
						}
					}
				 if (strstr (pdest+6,"&g"))
					{
					 //printf ("current values: sequence\n"); ULOGW ("current values: sequence");
					 for (pp=1; pp<channum; pp++)
						{
						 sprintf (buffer,"&g%d",pp);
						 ddest = strstr (pdest+2,buffer);
						 if (ddest && pp<10)	//&g1=a1
							{
							 if (*(ddest+4)>='0' && *(ddest+4)<='9')
								{
								 buffer[0]=*(ddest+4); buffer[1]=0;
								 if (*(ddest+5)>='0' && *(ddest+5)<='9')
									buffer[1]=*(ddest+5);
								 buffer[2]=0;
								}
							 g[pp]=(SHORT)atoi(buffer);
							}
						 if (ddest && pp>9)	//&g1=a1
							{
							 if (*(ddest+5)>='0' && *(ddest+5)<='9')
								{
								 buffer[0]=*(ddest+5); buffer[1]=0;
								 if (*(ddest+6)>='0' && *(ddest+6)<='9')
									buffer[1]=*(ddest+6);
								 buffer[2]=0;
								}
							 g[pp]=(SHORT)atoi(buffer);
							}
						}
					}
				 sprintf (buffer,"ShortChanName,Value,State\r\n");
				 if (debug>2) ULOGW ("g[1]=%d n[%d-%d]",g[1],n1,n2);
				 if (g[1] || (n1>0 && n2>0))
					{
					 for (int ch=0;ch<channum;ch++)
					 if (g[ch])
			    		    {
					     sprintf (buffer,"%s%s,%f,%d\r\n",buffer,chan[ch].descr,chd[ch].current,chd[ch].sts_cur);
					    }
					 if (n1>=0 && n2>=0)
					 for (int ch=n1;ch<=n2;ch++)
			    		    {
					     sprintf (buffer,"%s%s,%f,%d\r\n",buffer,chan[ch].descr,chd[ch].current,chd[ch].sts_cur);
					    }
					 send(client_socket,buffer,strlen(buffer),0);
					}
				 else
					{
					 send(client_socket,sendbuffer,strlen(sendbuffer),0);
					 send(client_socket,buffer,strlen(buffer),0);
					 printf ("%s\n",buffer);
			    		}
				}
			 //-----------------------------------------------------------------------------------
			 if (strstr (pdest,"archive") || strstr (pdest,"total") || strstr (pdest,"sys_events") || strstr (pdest,"events") || strstr (pdest,"last_event"))
				{
				 // /crq?req=archive&type=A&n1=1&n2=4
				 // /crq?req=archive&g1=a1&g2=a2&g3=a3&g4=a4
				 CHAR type=0; CHAR *ddest; CHAR *dest;
				 UINT n1=0,n2=0,pp=0; UINT g[20]={0};
				 CHAR t1[20],t2[20];
				 INT interval;
				 if (strstr (pdest+6,"&type=") || strstr (pdest+6,"&n1=") || strstr (pdest+6,"&n2="))
				    {
				     //ULOGW ("archive values: diapason");
				     ddest = strstr (pdest+7,"&type=");
				     if (ddest) type=ddest[6];
				     ddest = strstr (pdest+7,"&n1=");
				     if (ddest)
				        {
				         if (*(ddest+4)>='0' && *(ddest+4)<='9')
						{
					         buffer[0]=*(ddest+4); buffer[1]=0;
					         if (*(ddest+5)>='0' && *(ddest+5)<='9') buffer[1]=*(ddest+5);
					         buffer[2]=0;
						}
					 else
					 if (*(ddest+5)>='0' && *(ddest+5)<='9')
						{
					         buffer[0]=*(ddest+5); buffer[1]=0;
					         if (*(ddest+6)>='0' && *(ddest+6)<='9') buffer[1]=*(ddest+6);
					         buffer[2]=0;
						}
					  n1=(SHORT)atoi(buffer);
    					}
				     ddest = strstr (pdest+7,"&n2=");
				     if (ddest)
				        {
				         if (*(ddest+4)>='0' && *(ddest+4)<='9')
					   {
					     buffer[0]=*(ddest+4); buffer[1]=0;
					     if (*(ddest+5)>='0' && *(ddest+5)<='9') buffer[1]=*(ddest+5);
						 buffer[2]=0;
					    }
					 else
					 if (*(ddest+5)>='0' && *(ddest+5)<='9')
					    {
					     buffer[0]=*(ddest+5); buffer[1]=0;
					     if (*(ddest+6)>='0' && *(ddest+6)<='9') buffer[1]=*(ddest+6);
						 buffer[2]=0;
					    }
				         n2=(SHORT)atoi(buffer);
				        }
				    }
				 if (strstr (pdest+6,"&g"))
				    {
				     for (pp=1; pp<20; pp++)
				        {
    				         sprintf (buffer,"&g%d",pp); // &g1=v11
				         ddest = strstr (pdest+2,buffer);
				         //ULOGW ("%s\n",ddest);
				         if (ddest && pp<10)	//&g1=a1
						{
						 //ULOGW ("%c %c",*(ddest+4),*(ddest+5));
						 if (*(ddest+5)>='0' && *(ddest+5)<='9')
						    {
						     buffer[0]=*(ddest+5); buffer[1]=0;
						     if (*(ddest+6)>='0' && *(ddest+6)<='9')
							buffer[1]=*(ddest+6);
						     buffer[2]=0;
						    }
						 g[pp]=(SHORT)atoi(buffer);
						 printf ("%d ",g[pp]);
						}
					 if (ddest && pp>9)		//&g1=a11
						{
						 //ULOGW ("%c %c",*(ddest+5),*(ddest+6));
						 if (*(ddest+6)>='0' && *(ddest+6)<='9')
							{
							 buffer[0]=*(ddest+6); buffer[1]=0;
							 if (*(ddest+7)>='0' && *(ddest+7)<='9') 
								buffer[1]=*(ddest+7);
							 buffer[2]=0;
							}
						 g[pp]=(SHORT)atoi(buffer);
						 printf ("%d ",g[pp]);
						}
					}
				     printf ("\n");
				    }
				if (!g[1])
				    {
				     for (UINT i=n1; i<=n2; i++)
					g[i-n1+1]=i;
				    }
				if ((dest=strstr (pdest+6,"&t1="))!=NULL)
				    {
				     for (pp=0; pp<20; pp++)
				     if (dest[4+pp]!='&' && dest[4+pp])
				    	 t1[pp]=dest[4+pp];							 						
				     else break;
				     if (debug>1) ULOGW ("[https] archive values: t1=%s",t1);
    				    }
    				if ((dest=strstr (pdest+6,"&t2="))!=NULL)
					{
				         for (pp=0; pp<20; pp++)
				         if (dest[4+pp]!='&' && dest[4+pp])
					    t2[pp]=dest[4+pp];
				         else break;
				         ULOGW ("[https] archive values: t2=%s",t2);
					}
				 interval=1;
				 if ((dest=strstr (pdest+6,"&interval="))!=NULL)
					{
				         buffer[0]=0;
				         for (pp=0; pp<20; pp++)
				         if (*(dest+10+pp)!='&' && *(dest+10+pp))
					    buffer[pp]=*(dest+10+pp);
				         else break;
				         buffer[pp]=0;
				         if (strstr (buffer,"short"))	interval=1;
				         if (strstr (buffer,"main")) 	interval=2;
				         if (strstr (buffer,"day")) 	interval=4;
				         if (strstr (buffer,"month"))   interval=8;
				         if (strstr (buffer,"year")) 	interval=3;
					}
				  if (strstr (pdest,"sys_events"))
					{
				    	 sprintf (buffer,"Time,Value,Ipar,Fpar,\r\n");
					}
				  if (strstr (pdest,"=events") || strstr (pdest,"last_event"))
					{
					 sprintf (buffer,"ShortChanName,Time,Value,Ipar,Fpar,Comment,\r\n");
					}
				  if (strstr (pdest,"total") && type!='v') sprintf (buffer,"ShortChanName,Time,Value,State\r\n"); 
				  if (strstr (pdest,"total") && (type=='v' || type==0))
					{
					 sprintf (buffer,"ShortChanName,Time,Value,State\r\n");
					}
				  if (strstr (pdest,"archive") && type!='v') sprintf (buffer,"ShortChanName,Time,Value,State\r\n"); 
				  if (strstr (pdest,"archive") && (type=='v' || type==0))
					{
					 short rws=0;
					 //long it1,it2;
					 //it1=atoi(t1);
					 //it2=atoi(t2);
					 //if (((atoi(t2)-atoi(t1))>20000) && interval==1) ;
					 // sql request
					 sprintf (buffer,"ShortChanName,Time,Value,State\r\n");
					 if (debug>2) sprintf (query,"SELECT * FROM arch WHERE type=%d AND tms>=%s AND tms<=%s ORDER BY chan,tms",interval,t1,t2);
					 ULOGW ("[%s]",query);
				         if (mysql_query(mysql, query))
					    {
				             if (debug>2) ULOGW ("error select from arch table [[%d] %s]",mysql_errno(mysql),mysql_error(mysql));
        				     mysql_close(mysql);
					    }
					 if (!(res=mysql_store_result(mysql)))
					    {
    	    				     if (debug>0) ULOGW ("error in function store_result");
	    				     mysql_close(mysql);
					    }
				         while ((row=mysql_fetch_row(res)))
					    {
					     for (int ch=0;ch<channum;ch++)  
					     for (pp=0; pp<20; pp++) 
					     if (chan[ch].id==g[pp])
					     if (chan[ch].id==atoi(row[1]))
					        {
					         sprintf (buffer,"%s%s,%s,%s,%s\r\n",buffer,chan[ch].descr,row[2],row[4],row[5]);
						}
					     rws++; if (rws>800) break;
		    			    }
					 if (strstr (pdest,"akt=print"))    
					    {
					     rws=0;
					     if (g[1])
					     for (pp=1; pp<20; pp++) 
					        {
						 if (g[pp]) { rep.idx[rws]=g[pp]; rws++; }
						 if (rws>7) break;
						}
					     strcpy (rep.start,t1);
					     strcpy (rep.finish,t2);
					     rep.type=interval;
					     rep.pipe=1;
					     ValuesToReport (rep);
					    }
					 mysql_free_result(res);
					}
			    rightreq=TRUE;
			    strncat(sendbuffer,"Content-Type: text/plain\r\n",1024);
		 	    strncat(sendbuffer,"\r\n",1024);
			    //sprintf(sendbuffer,"Content-Type: text/plain\r\n\r\n");
			    send(client_socket,sendbuffer,strlen(sendbuffer),0);
			    send(client_socket,buffer,strlen(buffer),0);
			    if (debug>0) ULOGW ("[https] send buffer (%d)(%d)",strlen (sendbuffer),strlen (buffer));
			    //ULOGW ("send buffer (%d)",strlen (buffer));
			    buffer[499]=0;
			    if (debug>1) ULOGW ("[https] %s",buffer);
			 //-----------------------------------------------------------------------------------
			 }
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
 if (pos == NULL) return(FALSE);
 strncpy(requestheader.method,pos,SMALL_BUFFER_SIZE);
 pos = strtok(NULL," ");
 if (pos == NULL) return(FALSE);
 strncpy(requestheader.url,pos,256);
 pos = strtok(NULL,"\r");
 if (pos == NULL) return(FALSE);
 strncpy(requestheader.httpversion,pos,SMALL_BUFFER_SIZE);
 if (debug>2) ULOGW ("[https] recieve header [%s : %s] ver: %s",requestheader.method,requestheader.url,requestheader.httpversion);
 //ULOGW ("[%s : %s] ver: %s",requestheader.method,requestheader.url,requestheader.httpversion);
 if (debug>1) ULOGW ("[https] [%s]",requestheader.url);
 // based on the url lets figure out the filename + path
 strncpy(requestheader.filepathname,wwwroot,256);
 strncat(requestheader.filepathname,requestheader.url,256);
 return(TRUE);
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

 mysql=mysql_init(NULL);
 if (!mysql_real_connect(mysql, "localhost", "root", "","",3306,NULL,0))
    { 
     if (debug>0) ULOGW ("[https] connecting to database........failed [[%d] %s]",mysql_errno(mysql),mysql_error(mysql));
     mysql_close(mysql);
     exit (1);
    }
 else if (debug>0) ULOGW ("[https] connecting to database........success");
 mysql_real_query(mysql, "USE ugaz",strlen("USE ugaz"));

 return(s);
}
