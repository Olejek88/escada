//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "irp.h"
#include "lk.h"
#include "service.h"
#include "db.h"
#include "version/version.h"
#include <iconv.h>

//-----------------------------------------------------------------------------
static  MYSQL_RES *res;
static  MYSQL_RES *res2;
static  db      dbase;
static  MYSQL_ROW row;
static  MYSQL_ROW row2;

static  CHAR    query[500];
static  float   flatval[MAX_FLATS][4]={1.0};
static  float   flatdel[MAX_FLATS][4]={0.0};

extern  "C" UINT device_num;    // total device quant
extern  "C" UINT lk_num;        // LK sensors quant
extern  "C" UINT bit_num;       // BIT sensors quant
extern  "C" UINT ip2_num;       // 2IP sensors quant
extern  "C" BOOL WorkRegim;     // work regim flag
extern  "C" tm *currenttime;    // current system time

extern  "C" DeviceR     dev[MAX_DEVICE];        // device class
extern  "C" DeviceLK    lk[MAX_DEVICE_LK];      // local concantrator class
extern  "C" DeviceBIT   bit[MAX_DEVICE_BIT];    // BIT class
extern  "C" Device2IP   ip2[MAX_DEVICE_2IP];    // 2IP class
extern  "C" DeviceIRP   irp[MAX_DEVICE_IRP];    // IRP class

extern  "C" DeviceDK	dk;

static  union fnm fnum[5];
extern  "C" UINT	debug;

extern	BOOL	srv_thread;

BOOL    StoreTopDown();
BOOL    StoreStat();
BOOL    StoreDevices();
BOOL    StoreEvents();
BOOL    StoreCurrent();
BOOL    StoreFlats();
BOOL    StoreCurrent(const CHAR* name,const CHAR* edizm, UINT tid, UINT prm, UINT pipe);
BOOL    StoreLast();
BOOL    Autobackup();
BOOL 	StoreData();

FILE    *Dat;
CHAR    currbuf[5000];

VOID    ULOGW (const CHAR* string, ...);              // log function
VOID    Events (DWORD evnt, DWORD device);      // events 
//-----------------------------------------------------------------------------
// 1. auto backup and rescue
// 2. form xml from db
void * serviceThread (void * devr)
{
 int devdk=*((int*) devr);                      // DK identificator
 dbase.sqlconn("dk","root","");                 // connect to database
 sprintf (query,"set character_set_client='utf8'"); dbase.sqlexec(query); 
 sprintf (query,"set character_set_results='utf8'"); dbase.sqlexec(query);
 sprintf (query,"set collation_connection='utf8_general_ci'"); dbase.sqlexec(query);
  
 // load from db all lk devices
 // lk[0].LoadLKConfig();
 // load from db all bit devices > store to class members
 // LoadBITConfig();
 // load from db all 2ip devices > store to class members
 // Load2IPConfig(); 
 
 while (WorkRegim)
    {
     //if (debug>0) ULOGW ("[service] StoreTopDown()");
     //StoreTopDown();
     if (debug>0) ULOGW ("[service] StoreData()");
     StoreData();
     //if (debug>0) ULOGW ("[service] StoreDevices()");
     //StoreDevices();
     //if (debug>0) ULOGW ("[service] StoreEvents()");
     //StoreEvents();
     //if (debug>0) ULOGW ("[service] StoreLast(%d)",device_num);
     //if (device_num==0) break;
     //StoreLast();     
     //if (debug>0) ULOGW ("[service] StoreCurrent()");
     //StoreCurrent();
     //if (currenttime->tm_sec<10 && currenttime->tm_min%5==2) { if (debug>0) ULOGW ("[service] StoreFlats()"); StoreFlats(); }
     //if (currenttime->tm_hour==0 && currenttime->tm_min==0) Autobackup();
     sleep (60);
     if (!dk.formxml)// && !srv_thread)
        {
         if (debug>0) ULOGW ("[srv] service threads stopped");
         srv_thread=0;
         pthread_exit (NULL);
         return 0;	 
        }
    }
 dbase.sqldisconn();
 
 if (debug>0) ULOGW ("[service] service thread end");
}
//---------------------------------------------------------------------------------------------------
//<?xml version="1.0" encoding="koi8-r"?>
BOOL StoreCurrent()
{
 //var/www/html/current.xml
 CHAR   buf[300];
 UINT   tid=0;
 sprintf (currbuf,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<data>\n");
// ULOGW ("[service] device=%d",device_num); 
 for (UINT s=0;s<device_num;s++)
 if (dev[s].type==TYPE_INPUTTEKON)
    { 
     tid=dev[s].idd;
     //ULOGW ("[service] dev[%d].idd=%d",s,dev[s].idd); 
     //ULOGW ("[service] tid=%d",tid); 
     if (tid==84606977)
        {
         StoreCurrent("Qpr", "m3", tid, 11, 1);
         StoreCurrent("Qobr", "m3", tid, 11, 0);
         StoreCurrent("Q", "m3", tid, 11, 2);

         StoreCurrent("Tpod", "C", tid, 4, 0);
         StoreCurrent("Tobr", "C", tid, 4, 1);
         StoreCurrent("Tpr,CO", "C", tid, 4, 3);
         StoreCurrent("Tobr,CO", "C", tid, 4, 4);

         StoreCurrent("Pobr", "MPa", tid, 16, 1);
         StoreCurrent("Ppod", "MPa", tid, 16, 0);
 
         StoreCurrent("Qpod.", "GKal", tid, 13, 2);
         StoreCurrent("Qobr.", "GKal", tid, 13, 1);
         StoreCurrent("Q 3 co", "GKal", tid, 13, 0);

         StoreCurrent("Hpod", "MG/t", tid, 1, 1);
	 StoreCurrent("Hobr", "MG/t", tid, 1, 0);

         StoreCurrent("Gobr.", "t.", tid, 12, 1);
         StoreCurrent("Gpod.", "t.", tid, 12, 0);
         StoreCurrent("Gobr.CO", "t.", tid, 12, 3);
         StoreCurrent("Gpod.CO", "t.", tid, 12, 4);

	}
     if (tid==84606978)
        {
         StoreCurrent("Tpod.", "C", tid, 4, 5);
         StoreCurrent("Tobr.", "C", tid, 4, 6);
         StoreCurrent("H", "MJ/t", tid, 1, 5);
         StoreCurrent("Ggvs", "t.", tid, 12, 5);
         StoreCurrent("Qhvs", "m3", tid, 11, 6);
         StoreCurrent("Ggvs", "t.", tid, 12, 6);
	}	
    }
 sprintf (currbuf,"%s</data>\n",currbuf);
 Dat =  fopen(CURRENT_XML,"w"); fprintf (Dat, currbuf);
 fclose (Dat);
}
//---------------------------------------------------------------------------------------------------
//<?xml version="1.0" encoding="koi8-r"?>
//<devices>
//<device id="1" name="¯-T¦+-19 [+L¦-]" status="1" conn="0" time="16.10.2008 14:43:11" error="5623/234"></device>
BOOL StoreDevices()
{
 //var/www/html/device.xml
 CHAR   buf[300],devname[50],devtime[50];
 UINT   flats=0,qatt=0,qerrors=0,conn=0,nd=0;
 
 Dat =  fopen(DEVICE_XML,"w");
 sprintf (buf,"<?xml version=\"1.0\" encoding=\"koi8-r\"?>\n"); fprintf (Dat, buf);
 sprintf (buf,"<devices>\n"); fprintf (Dat, buf);
 for (UINT r=0;r<3000;r++)
// if (dev[r].type==5)
     {
      //nd++; if (nd>19) break;
      sprintf (buf,"<device id=\"%d\" name=\"%s\" status=\"%d\" conn=\"%d\" time=\"%s\" error=\"%d/%d\"></device>\n",dev[r].idd,dev[r].name,dev[r].akt,dev[r].conn,dev[r].devtim,dev[r].qerrors,dev[r].qatt);
      fprintf (Dat, buf); 
     }
 sprintf (buf,"</devices>\n"); fprintf (Dat, buf);
 fclose (Dat);
}

//---------------------------------------------------------------------------------------------------
BOOL StoreData()
{
 CHAR   buf[300],devname[50],devtime[50],name[100];
 UINT   counters=0,qatt=0,qerrors=0,conn=0,nd=0,nr2=0;
 
// iconv_t cd;
// cd = iconv_open("koi8-r", "UTF-8");

 Dat =  fopen(DATA_XML,"w");
 sprintf (buf,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"); fprintf (Dat, buf);
 sprintf (buf,"<object id=\"1\" name=\"churilovo-agro\">\n"); fprintf (Dat, buf);

 sprintf (query,"SELECT COUNT(id) FROM device"); 
 res=dbase.sqlexec(query);
 UINT nr=mysql_num_rows(res);
 row=mysql_fetch_row(res);
 counters=atoi(row[0]);
 sprintf (buf,"\t<counter_num>%d</counter_num>\n",counters); fprintf (Dat, buf);
 sprintf (buf,"\t\t<uzels>\n"); fprintf (Dat, buf);
 if (res) mysql_free_result(res); 

 sprintf (query,"SELECT * FROM device WHERE req=1");
 res=dbase.sqlexec(query);
 nr=mysql_num_rows(res);
 for (UINT r=0;r<nr;r++)
     { 
      row=mysql_fetch_row(res);
      //ULOGW ("[service] %s",row[20]); 

//      iconv(cd, row[20], sizeof(row[20]), name, sizeof(name));
//      ULOGW ("[service] %s > %s",row[20],name); 

      if (atoi(row[19])==4)
          sprintf (buf,"\t\t<uzel id=\"%d\" serial=\"%s\" name=\"%s\" source=\"energy\">\n",atoi(row[1]),row[29],row[20]);
      else if (atoi(row[19])==1)
          sprintf (buf,"\t\t<uzel id=\"%d\" serial=\"%s\" name=\"%s\" source=\"heat\">\n",atoi(row[1]),row[29],row[20]); 
      else sprintf (buf,"\t\t<uzel id=\"%d\" serial=\"%s\" name=\"%s\" source=\"heat\">\n",atoi(row[1]),row[29],row[20]); 
      switch (atoi(row[19]))
	    {
	     case 1: sprintf (buf,"\t\t<uzel id=\"%d\" serial=\"%s\" name=\"%s\" source=\"energy\">\n",atoi(row[1]),row[29],row[20]); break;
	     case 2: sprintf (buf,"\t\t<uzel id=\"%d\" serial=\"%s\" name=\"%s\" source=\"water\">\n",atoi(row[1]),row[29],row[20]); break;
	     case 3: sprintf (buf,"\t\t<uzel id=\"%d\" serial=\"%s\" name=\"%s\" source=\"gas\">\n",atoi(row[1]),row[29],row[20]); break;
	     case 4: sprintf (buf,"\t\t<uzel id=\"%d\" serial=\"%s\" name=\"%s\" source=\"heat\">\n",atoi(row[1]),row[29],row[20]); break;
	     default: break;
	    }
      fprintf (Dat, buf);
      sprintf (buf,"\t\t<tpname>%s</tpname>\n",row[20]); fprintf (Dat, buf);
      //sprintf (buf,"\t\t\t<fider>%s</fider>\n",row[27]); fprintf (Dat, buf);
      //sprintf (buf,"\t\t\t<tarif>%s</tarif>\n",row[28]); fprintf (Dat, buf);
      //sprintf (buf,"\t\t\t<Kr>%s</Kr>\n",row[31]); fprintf (Dat, buf);
      //sprintf (buf,"\t\t\t<Ktr>%s</Ktr>\n",row[32]); fprintf (Dat, buf);
      //sprintf (buf,"\t\t\t<Klp>%s</Klp>\n",row[33]); fprintf (Dat, buf);

      switch (atoi(row[19]))
	    {
	     case 1: sprintf (buf,"\t\t\t<data ed=\"KWt\" ed_code=\"87\">\n"); break;
	     case 2: sprintf (buf,"\t\t\t<data ed=\"m3\" ed_code=\"12\">\n"); break;
	     case 3: sprintf (buf,"\t\t\t<data ed=\"m3\" ed_code=\"12\">\n"); break;
	     case 4: sprintf (buf,"\t\t\t<data ed=\"GKal\" ed_code=\"83\">\n"); break;
	     default: sprintf (buf,"\t\t\t<data ed=\"unknown\" ed_code=\"0\">\n");
	    }
      fprintf (Dat, buf);

      sprintf (buf,"\t\t\t\t<month>\n"); fprintf (Dat, buf);
      switch (atoi(row[19]))
	    {
	     case 1: sprintf (query,"SELECT * FROM prdata WHERE value>=0 AND type=4 AND prm=14 AND pipe=0 AND device=%d ORDER BY date DESC LIMIT 5",atoi(row[1])); break; 
    	     case 2: sprintf (query,"SELECT * FROM prdata WHERE value>=0 AND type=4 AND prm=11 AND pipe=0 AND device=%d ORDER BY date DESC LIMIT 5",atoi(row[1])); break; 
    	     case 3: sprintf (query,"SELECT * FROM prdata WHERE value>=0 AND type=4 AND prm=11 AND pipe=0 AND device=%d ORDER BY date DESC LIMIT 5",atoi(row[1])); break; 
    	     case 4: sprintf (query,"SELECT * FROM prdata WHERE value>=0 AND type=4 AND prm=13 AND pipe=0 AND device=%d ORDER BY date DESC LIMIT 5",atoi(row[1])); break;
	     default: break;
	    }
      res2=dbase.sqlexec(query);
      nr2=mysql_num_rows(res2);
      for (UINT r=0;r<nr2;r++)
        { 
         row2=mysql_fetch_row(res2);
	 sprintf (buf,"\t\t\t\t\t<rec date=\"%s\">%s</rec>\n",row2[4],row2[5]); fprintf (Dat, buf);
	}
      if (res2) mysql_free_result(res2); 
      sprintf (buf,"\t\t\t\t</month>\n"); fprintf (Dat, buf);

      sprintf (buf,"\t\t\t\t<days>\n"); fprintf (Dat, buf);
      switch (atoi(row[19]))
	    {
	     case 1: sprintf (query,"SELECT * FROM prdata WHERE value>=0 AND type=2 AND prm=14 AND pipe=0 AND device=%d ORDER BY date DESC LIMIT 25",atoi(row[1])); break; 
    	     case 2: sprintf (query,"SELECT * FROM prdata WHERE value>=0 AND type=2 AND prm=11 AND pipe=0 AND device=%d ORDER BY date DESC LIMIT 25",atoi(row[1])); break; 
    	     case 3: sprintf (query,"SELECT * FROM prdata WHERE value>=0 AND type=2 AND prm=11 AND pipe=0 AND device=%d ORDER BY date DESC LIMIT 25",atoi(row[1])); break; 
    	     case 4: sprintf (query,"SELECT * FROM prdata WHERE value>=0 AND type=2 AND prm=13 AND pipe=0 AND device=%d ORDER BY date DESC LIMIT 25",atoi(row[1])); break;
	     default: break;
	    }
      res2=dbase.sqlexec(query);
      nr2=mysql_num_rows(res2);
      for (UINT r=0;r<nr2;r++)
        { 
         row2=mysql_fetch_row(res2);
	 sprintf (buf,"\t\t\t\t\t<rec date=\"%s\">%s</rec>\n",row2[4],row2[5]); fprintf (Dat, buf);
	}
      if (res2) mysql_free_result(res2); 
      sprintf (buf,"\t\t\t\t</days>\n"); fprintf (Dat, buf);
      sprintf (buf,"\t\t\t</data>\n"); fprintf (Dat, buf);
      sprintf (buf,"\t\t</uzel>\n"); fprintf (Dat, buf);
    }
 sprintf (buf,"\t</uzels>\n"); fprintf (Dat, buf);
 sprintf (buf,"</object>\n"); fprintf (Dat, buf);
 if (res) mysql_free_result(res); 
 fclose (Dat); 
 return 0;
}
//---------------------------------------------------------------------------------------------------
//<?xml version="1.0" encoding="koi8-r"?>
//<event id="1004" date="15.10.2008 15:14:45" cat="error" name="ÿ-ãTT+ L+T-ã L üû[0x104032], T¦¦ã¦+LL+¦ ¦¦+L¦T¦+ [12]"></event>
BOOL StoreEvents()
{
 //var/www/html/events.xml
 CHAR   cats[10],descr[300];
 CHAR   buf[300];
 UINT   s=0;

 Dat =  fopen(EVENTS_XML,"w");
 sprintf (buf,"<?xml version=\"1.0\" encoding=\"koi8-r\"?>\n"); fprintf (Dat, buf);
 sprintf (buf,"<events>\n"); fprintf (Dat, buf); 

 sprintf (query,"SELECT * FROM register ORDER BY date DESC LIMIT 10"); 
 res=dbase.sqlexec(query);
 UINT nr=mysql_num_rows(res);
 for (UINT r=0;r<nr;r++)
     { 
      row=mysql_fetch_row(res);
      if (atoi(row[1])>>28==0) strcpy (cats,"success");
      if (atoi(row[1])>>28==1) strcpy (cats,"info");
      if (atoi(row[1])>>28==2) strcpy (cats,"warning");
      if (atoi(row[1])>>28==3) strcpy (cats,"error");
      if (atoi(row[1])>>28==4) strcpy (cats,"fault");
      
      for (s=0;s<device_num;s++)
      if (dev[s].idd==atoi(row[2])) break;
      
      sprintf (buf,"<event id=\"%d\" date=\"%s\" cat=\"%s\" name=\"Recieve code [0x%x] from device %s[0x%x]\"></event>\n",row[0],row[3],cats,row[1],dev[s].name,row[2]);
      fprintf (Dat, buf);
     }
 sprintf (buf,"</events>\n"); fprintf (Dat, buf); 
 if (res) mysql_free_result(res); 
 fclose (Dat); 
}
//---------------------------------------------------------------------------------------------------
//<?xml version="1.0" encoding="koi8-r"?>
//<lasts>
//<last date="14-10-2008 10:00" P1="92.34" P2="81.78" P3="122.54" P4="120.22" P5="22.23" P6="20.98" P7="19.42" P8="100.74"></last>
BOOL StoreLast()
{
 //var/www/html/last.xml
 CHAR   cats[10],descr[300],date[30];
 CHAR   buf[25000];
 UINT   s=0;
 sprintf (buf,"<?xml version=\"1.0\" encoding=\"koi8-r\"?>\n");
 sprintf (buf,"%s<lasts>\n",buf);

 sprintf (date,"%04d%02d%02d%02d0000",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour-2);
 sprintf (query,"SELECT * FROM data WHERE type=1 AND flat=0 AND date<%s ORDER BY date DESC,prm,source LIMIT 1500",date);
 
 res=dbase.sqlexec(query);
 UINT nr=mysql_num_rows(res);
 for (UINT r=0;r<nr;r++)
     {      
      row=mysql_fetch_row(res);
      // ULOGW ("[service] 1=%s 2=%s",date,row[2]); 
      if (r==0 || !strstr (date,row[2]))
        {
          if (r>0) 
            {
             sprintf (buf,"%s></last>\n",buf); s++;
	     if (s>60) break;
            }
          strcpy (date,row[2]);	  
          sprintf (buf,"%s<last date=\"%c%c-%c%c %c%c:%c%c\" P%d_%d=\"%.2f\" ",buf,date[8],date[9],date[5],date[6],date[11],date[12],date[14],date[15],atoi(row[8]),atoi(row[6]),atof(row[3]));
        }
      else
        {
         sprintf (buf,"%s P%d_%d=\"%.3f\" ",buf,atoi(row[8]),atoi(row[6]),atof(row[3]));
        }
     }
 sprintf (buf,"%s\n</lasts>\n",buf);
 if (res) mysql_free_result(res); 
 Dat =  fopen(LAST_XML,"w"); fprintf (Dat, buf);
 fclose (Dat); 
}

//---------------------------------------------------------------------------------------------------
//<?xml version="1.0" encoding="koi8-r"?>
//<flats>
//<flat id="1" flat="1" level="1" rooms="3" side="1" name="ýãLã+ ñ.ñ." Q="15.352" dQ="+6.35%" V="3.12" dV="+2.61%" W="10.231" dW="+8.77%" ></flat>
BOOL StoreFlats()
{
 CHAR   buf[300];
 UINT   fl[4];
 CHAR   name[100];
 
 Dat =  fopen(FLATS_XML,"w");
 sprintf (buf,"<?xml version=\"1.0\" encoding=\"koi8-r\"?>\n"); fprintf (Dat, buf);
 sprintf (buf,"<flats>\n"); fprintf (Dat, buf); 

 sprintf (query,"SELECT * FROM data WHERE type=0 AND prm=13 ORDER BY value DESC");
 res=dbase.sqlexec(query);
 UINT nr=mysql_num_rows(res);
 if (res)
 for (UINT r=0;r<nr;r++)
     {
      row=mysql_fetch_row(res);
      sprintf (query,"SELECT * FROM flats WHERE flat=\"%s\"",row[4]);
      res2=dbase.sqlexec(query);
      row2=mysql_fetch_row(res2); 
      if (res2) mysql_free_result(res2);
      fl[0]=atoi(row2[1]); fl[1]=atoi(row2[2]); fl[2]=atoi(row2[3]); fl[3]=atoi(row2[4]);
      strcpy(name,row2[5]);
      
      if (atoi (row[4])>0 && atoi (row[4])<MAX_FLATS)
        {        
         sprintf (query,"SELECT * FROM data WHERE type=0 AND flat=%s ORDER BY date DESC",row[4]);
         res2=dbase.sqlexec(query);
         
         if (res2)
            { 
             UINT nr2=mysql_num_rows(res2);
             for (UINT r2=0;r2<nr2;r2++)
                {
                 row2=mysql_fetch_row(res2);
                 if (debug>3) ULOGW ("[service] %d %f",atoi(row2[8]),atof(row2[3]));
                 if (atoi(row2[8])==7) { if (flatval[atoi (row[4])][0]>0) flatdel[atoi (row[4])][0]=(atof(row2[3])-flatval[atoi (row[4])][0])*100/flatval[atoi (row[4])][0]; flatval[atoi (row[4])][0]=atof(row2[3]);}
                 if (atoi(row2[8])==5) { if (flatval[atoi (row[4])][1]>0) flatdel[atoi (row[4])][1]=(atof(row2[3])-flatval[atoi (row[4])][1])*100/flatval[atoi (row[4])][1]; flatval[atoi (row[4])][1]=atof(row2[3]);}
                 if (atoi(row2[8])==13){ if (flatval[atoi (row[4])][2]>0) flatdel[atoi (row[4])][2]=(atof(row2[3])/1000-flatval[atoi (row[4])][2])*100/flatval[atoi (row[4])][2]; flatval[atoi (row[4])][2]=atof(row2[3])/1000;}
                 if (atoi(row2[8])==14){ if (flatval[atoi (row[4])][3]>0) flatdel[atoi (row[4])][3]=(atof(row2[3])-flatval[atoi (row[4])][3])*100/flatval[atoi (row[4])][3]; flatval[atoi (row[4])][3]=atof(row2[3]);}
                 if (debug>3) ULOGW ("[service] %d %f %f",atoi(row2[8]),flatval[atoi (row[4])][3],flatdel[atoi (row[4])][3]);
                }            
            }           
         sprintf (buf,"<flat id=\"%d\" flat=\"%d\" level=\"%d\" rooms=\"%d\" name=\"%s\" ",r,fl[0],fl[1],fl[3],name);
         sprintf (buf,"%sQ=\"%.2f\" dQ=\"%.2f%%\" V=\"%.2f\" dV=\"%.2f%%\" V2=\"%.2f\" dV2=\"%.2f%%\" W=\"%.2f\" dW=\"%.2f%%\"></flat>\n",buf,flatval[atoi (row[4])][2],flatdel[atoi (row[4])][2],flatval[atoi (row[4])][1],flatdel[atoi (row[4])][1],flatval[atoi (row[4])][0],flatdel[atoi (row[4])][0],flatval[atoi (row[4])][3],flatdel[atoi (row[4])][3]);
         fprintf (Dat, buf); if (res2) mysql_free_result(res2);
        }
     }
 sprintf (buf,"</flats>\n"); fprintf (Dat, buf); 
 if (res) mysql_free_result(res); 
 fclose (Dat);  
}
//---------------------------------------------------------------------------------------------------
//<?xml version="1.0" encoding="koi8-r"?>
//<top>
//<tops id="[0x2070001]" address="ëÏÓÍÏÍÏÌØÓËÉÊ ÐÒÏÓÐÅËÔ 11" flats="120" ip="172.16.1.203" name="äÏÍÏ×ÏÊ ËÏÎÃÅÎÔÒÁÔÏÒ" build="1" regim="2"></tops>
//<down lastdate="22-09-2008 12:33:06" devtime="22-09-2008 12:33:05" qatt="0" qerr="0" conn="0" qzapr="5" int1="3600" int2="600" int3="3600"></down>
//</top>
BOOL StoreTopDown()
{
 //var/www/html/td.xml
 CHAR   buf[300],devname[50],devtime[50];
 UINT   flats=0,qatt=0,qerrors=0,conn=0;
 
 Dat =  fopen(TD_XML,"w");
 sprintf (buf,"<?xml version=\"1.0\" encoding=\"koi8-r\"?>\n"); fprintf (Dat, buf);
 sprintf (buf,"<top>\n"); fprintf (Dat, buf); 

 sprintf (query,"SELECT COUNT(id) FROM flats"); 
 res=dbase.sqlexec(query);
 row=mysql_fetch_row(res); flats=atoi(row[0]);
 if (res) mysql_free_result(res); 
 
 sprintf (query,"SELECT * FROM device WHERE type=%d",TYPE_DK); 
 res=dbase.sqlexec(query);
 row=mysql_fetch_row(res); strcpy(devname,row[20]); strcpy(devtime,row[16]);
 qatt=atoi(row[13]); qerrors=atoi(row[14]); conn=atoi(row[15]);
 if (res) mysql_free_result(res); 

 sprintf (query,"SELECT * FROM dev_dk"); 
 res=dbase.sqlexec(query);
 row=mysql_fetch_row(res);
 sprintf (buf,"<tops id=\"[0x%x]\" version=\"%s\" address=\"%s\" flats=\"%d\" ip=\"%s\" name=\"%s\" build=\"%d\" regim=\"%d\"></tops>\n",atoi(row[1]),version,row[14],flats,row[3],devname,atoi(row[4]),atoi(row[5])); fprintf (Dat, buf);
 sprintf (buf,"<down lastdate=\"%s\" devtime=\"%s\" qatt=\"%d\" qerr=\"%d\" conn=\"%d\" qzapr=\"%d\" int1=\"%d\" int2=\"%d\" int3=\"%d\"></down>\n",row[12],devtime,qatt,qerrors,conn,atoi(row[6]),atoi(row[9]),atoi(row[10]),atoi(row[11])); fprintf (Dat, buf); 
 sprintf (buf,"</top>\n"); fprintf (Dat, buf); 
 if (res) mysql_free_result(res); 

 fclose (Dat);
}
//---------------------------------------------------------------------------------------------------
//<?xml version="1.0" encoding="koi8-r"?>
//<?xml version="1.0" encoding="koi8-r"?>
//<stats>
//<stat id="1" name="ìë" quant="2" conn="1" obmen="345"></stat>
//<stat id="1" name="âéô" quant="20" conn="1" obmen="225"></stat>
//<stat id="1" name="2éð" quant="8" conn="1" obmen="45"></stat>
//<stat id="1" name="íüü" quant="8" conn="0" obmen="29"></stat>
//<stat id="1" name="éòð" quant="10" conn="1" obmen="110"></stat>
//<stat id="1" name="ôÜËÏÎ" quant="1" conn="0" obmen="67"></stat>
//</stats>
BOOL StoreStat()
{
 //var/www/html/stat.xml
 CHAR   buf[300];
 UINT   quant=0,qatt=0,qerrors=0,conn=0;
 
 Dat =  fopen(STAT_XML,"w");
 sprintf (buf,"<?xml version=\"1.0\" encoding=\"koi8-r\"?>\n"); fprintf (Dat, buf);
 sprintf (buf,"<stats>\n"); fprintf (Dat, buf); 

 sprintf (query,"SELECT COUNT(id),SUM(conn),SUM(qatt) FROM device WHERE type=6");
 res=dbase.sqlexec(query); 
 if (res) 
    {
     row=mysql_fetch_row(res); 
     if (atoi(row[0])>0) 
        {
         quant=atoi(row[0]); conn=atoi(row[1]); qatt=atoi(row[2]);    
         sprintf (buf,"<stat id=\"6\" name=\"ìë\" quant=\"%d\" conn=\"%d\" obmen=\"%d\"></stat>\n",quant,conn,qatt); fprintf (Dat, buf);
        }
     mysql_free_result(res);
    }
 sprintf (query,"SELECT COUNT(id),SUM(conn),SUM(qatt) FROM device WHERE type=1");
 res=dbase.sqlexec(query); 
 if (res)
    {
     row=mysql_fetch_row(res); 
     if (atoi(row[0])>0) 
        {
         sprintf (buf,"<stat id=\"1\" name=\"âéô\" quant=\"%s\" conn=\"%s\" obmen=\"%s\"></stat>\n",row[0],row[1],row[2]); fprintf (Dat, buf);
        }
     mysql_free_result(res);    
    }
 sprintf (query,"SELECT COUNT(id),SUM(conn),SUM(qatt) FROM device WHERE type=2");
 res=dbase.sqlexec(query); 
 if (res)
    {
     row=mysql_fetch_row(res); 
     if (atoi(row[0])>0) 
        {
         quant=atoi(row[0]); conn=atoi(row[1]); qatt=atoi(row[2]);
         sprintf (buf,"<stat id=\"2\" name=\"2éð\" quant=\"%d\" conn=\"%d\" obmen=\"%d\"></stat>\n",quant,conn,qatt); fprintf (Dat, buf);
        }
     mysql_free_result(res); 
    }
 sprintf (query,"SELECT COUNT(id),SUM(conn),SUM(qatt) FROM device WHERE type=4");  
 res=dbase.sqlexec(query);
 if (res)
    {
     row=mysql_fetch_row(res); 
     if (atoi(row[0]))
        {
         quant=atoi(row[0]); conn=atoi(row[1]); qatt=atoi(row[2]);
         sprintf (buf,"<stat id=\"4\" name=\"íüü\" quant=\"%d\" conn=\"%d\" obmen=\"%d\"></stat>\n",quant,conn,qatt); fprintf (Dat, buf);
        }
     mysql_free_result(res); 
    }
 sprintf (query,"SELECT COUNT(id),SUM(conn),SUM(qatt) FROM device WHERE type=5");
 res=dbase.sqlexec(query); 
 if (res)
    {
     row=mysql_fetch_row(res); 
     if (atoi(row[0]))
        {
         quant=atoi(row[0]); conn=atoi(row[1]); qatt=atoi(row[2]);
         sprintf (buf,"<stat id=\"5\" name=\"éòð\" quant=\"%d\" conn=\"%d\" obmen=\"%d\"></stat>\n",quant,conn,qatt); fprintf (Dat, buf);
        }
     mysql_free_result(res); 
    }
 sprintf (query,"SELECT COUNT(id),SUM(conn),SUM(qatt) FROM device WHERE type=11");
 res=dbase.sqlexec(query); 
 if (res)
    {
     row=mysql_fetch_row(res); 
     if (atoi(row[0]))
        {
         quant=atoi(row[0]); conn=atoi(row[1]); qatt=atoi(row[2]);
         sprintf (buf,"<stat id=\"11\" name=\"ôÜËÏÎ\" quant=\"%d\" conn=\"%d\" obmen=\"%d\"></stat>\n",quant,conn,qatt); fprintf (Dat, buf);
        }
     mysql_free_result(res); 
    }
 sprintf (buf,"</stats>\n"); fprintf (Dat, buf); 
 fclose (Dat);
}
//---------------------------------------------------------------------------------------------------
BOOL    StoreCurrent(const CHAR* name,const CHAR* edizm, UINT tid, UINT prm, UINT pipe)
{
 CHAR   buf[300];
 sprintf (query,"SELECT * FROM prdata WHERE type=0 AND device=%d AND prm=%d AND pipe=%d ORDER BY date DESC LIMIT 1",tid,prm,pipe);
 res=dbase.sqlexec(query); row=mysql_fetch_row(res); 
 if (res && row) 
    { 
     mysql_free_result(res); 
     sprintf (currbuf,"%s<tag date=\"%s\" name=\"%s\" P=\"%s\" E=\"%s\"></tag>\n",currbuf,row[4],name,row[5],edizm);
     //fprintf (Dat, buf); 
    }
}
//---------------------------------------------------------------------------------------------------
BOOL    Autobackup()
{
 CHAR   buf[300];
 sprintf (buf,"/tmp/prdata_%04d%02d%02d%02d%02d00.sql",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min);
 sprintf (query,"SELECT * INTO OUTFILE \"%s\" FROM prdata",buf);
 res=dbase.sqlexec(query);  if (res) mysql_free_result(res); 
 sprintf (buf,"/tmp/data_%04d%02d%02d%02d%02d00.sql",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min);
 sprintf (query,"SELECT * INTO OUTFILE \"%s\" FROM data",buf);
 res=dbase.sqlexec(query);  if (res) mysql_free_result(res);
 sprintf (buf,"/tmp/devices_%04d%02d%02d%02d%02d00.sql",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min);
 sprintf (query,"SELECT * INTO OUTFILE \"%s\" FROM devices",buf);
 res=dbase.sqlexec(query);  if (res) mysql_free_result(res);
 sleep(60);
 sprintf (query,"DELETE FROM prdata WHERE value>100000000 OR date<20090101000000 OR value<-1000 OR type>4");
 //res=dbase.sqlexec(query);  if (res) mysql_free_result(res);
 sprintf (query,"DELETE FROM prdata WHERE date<%04d%02d01000000 AND (prm=2 OR prm=31) AND type=1",currenttime->tm_year+1900,currenttime->tm_mon);
 ULOGW ("[service] %s",query);
 //res=dbase.sqlexec(query);  if (res) mysql_free_result(res);
 return true;
}
//---------------------------------------------------------------------------------------------------
BOOL    SaveTemp()
{
 sprintf (query,"SELECT * INTO OUTFILE \"temp/datas.sql\" FROM prdata");
 res=dbase.sqlexec(query);  if (res) mysql_free_result(res); 
 return true;
}
