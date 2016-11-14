//----------------------------------------------------------------------------
#define	TIC_TIMER	1
//----------------------------------------------------------------------------
#include "types.h"
#include "main.h"
#include "errors.h"

//#include <libgtop-2.0/glibtop.h>
//#include <libgtop-2.0/glibtop/cpu.h>

#include "crq.h"
#include "db.h"
#include "func.h"

#include "elf2.h"
#include "spg741.h"
#include "spg742.h"
#include "tsrv.h"
#include "lk.h"
#include "irp.h"
#include "mercury230.h"
#include "vkt.h"
#include "vis-t.h"
#include "panel.h"
#include "km_5.h"
#include "ce102.h"
#include "ce303.h"
#include "tecon_19.h"
//#include "hart.h"
//#include "tecon_19_gsm.h"

//#include "solver_sle.h"
#include <sys/utsname.h>
#include "eval_flats.h"
#include "kernel.h"
#include "service.h"
#include "version/version.h"
#include <sys/time.h>
#include <sys/resource.h>

//----------------------------------------------------------------------------
extern	bool	CalculateFlats	(UINT	type,  CHAR* date);
//----------------------------------------------------------------------------
int main ()
{
 FILE *Log;
 time_t tim;
 tim=time(&tim);
 currenttime=localtime(&tim); // get current system time
 sprintf (kernellog,"log/kernel-%04d%02d%02d_%02d%02d.log",currenttime->tm_year+1900,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min);

 Log = fopen(kernellog,"w"); fclose (Log);
 ULOGW ("%skernel v.%s started%s",kernel_color,version,nc);
 if (initkernel())
    {
     if(pthread_create(&thr2,NULL,dispatcher,NULL) != 0) ULOGW ("error create thread");
    }
 else
    {
     ULOGW ("%skernel finished, because mysql database not available%s",kernel_color,nc);
     return 0;
    }
 while (WorkRegim) sleep(1);
 sleep (3600*1572);
 ULOGW ("%skernel finished%s",kernel_color,nc);
 return 0;
}
//----------------------------------------------------------------------------
int initkernel (void)
{
 //-------------------------------------------------------------------------------
 if (debug>0) ULOGW("initkernel started...");
 // connect to database
 if (!dbase.sqlconn("escada","root","")) return 0;
 //----------------------------------------------------------------------------
 uname(&ubuf);
 sprintf (querys,"UPDATE dev_dk SET regim=3");
 if (debug>2) ULOGW ("%s%s%s",sql_color,querys,nc);
 res_=dbase.sqlexec(querys);

 sprintf (querys,"UPDATE threads SET global=0");
 res_=dbase.sqlexec(querys);

 sprintf (querys,"UPDATE info SET log='%s',linux='%s %s',hardware='%s',base_name='escada',software='%s',ip='1.1.1.1'",kernellog,ubuf.sysname,ubuf.release,ubuf.machine,version);
 if (debug>2) ULOGW ("%s%s%s",sql_color,querys,nc);
 res_=dbase.sqlexec(querys);
 if (res_) mysql_free_result(res_);
 //----------------------------------------------------------------------------
 dbase.sqlexec("UPDATE device SET conn=0"); 

 res_=dbase.sqlexec("SELECT * FROM device"); 
 if (res_)
    {
     isConfigurated=TRUE;
     device_num=mysql_num_rows(res_);
     if (debug>1) ULOGW ("total device num [%d] ",device_num);
     //DeviceD* dev= new DeviceD();
     //DeviceD dev[device_num];
     UINT devnum=0;
     for (UINT r=0;r<device_num;r++)
        {
	 row=mysql_fetch_row(res_);
	 dev[devnum].id=atoi(row[0]);
	 dev[devnum].idd=atoi(row[1]);
	 dev[devnum].SV=atoi(row[2]);
	 dev[devnum].interface=atoi(row[3]);
	 dev[devnum].protocol=atoi(row[4]);
	 dev[devnum].port=atoi(row[5]);
	 dev[devnum].speed=atoi(row[6]);
	 dev[devnum].adr=atoi(row[7]);
	 dev[devnum].type=atoi(row[8]);
	 strcpy(dev[devnum].number,row[9]);
	 dev[devnum].flat=atoi(row[10]);
	 dev[devnum].akt=atoi(row[11]);
	 strcpy(dev[devnum].lastdate,row[12]);
	 dev[devnum].qatt=atoi(row[13]);
	 dev[devnum].qerrors=atoi(row[14]);
	 dev[devnum].conn=atoi(row[15]);
	 strcpy(dev[devnum].devtim,row[16]);
	 dev[devnum].chng=atoi(row[17]);
	 dev[devnum].req=atoi(row[18]);
	 dev[devnum].source=atoi(row[19]);
	 strcpy(dev[devnum].name,row[20]);
	 dev[devnum].ust=atoi(row[21]);

	 //dev[devnum].rasknt=atof(row[31]);
	 //dev[devnum].pottrt=atof(row[32]);
	 //dev[devnum].potlep=atof(row[33]);

	 if (dev[devnum].type==7)	// dk
	    {
	     dk.iddk=dev[devnum].idd;
	     dk.adr=dev[devnum].idd;
	     dk.SV=dev[devnum].SV;
    	     dk.interface=dev[devnum].interface;
    	     dk.protocol=dev[devnum].protocol;
	     dk.port=dev[devnum].port;
	     dk.speed=dev[devnum].speed;
	     dk.adr=dev[devnum].adr;
	     dk.type=dev[devnum].type;
	     strcpy(dk.number,dev[devnum].number);
	     dk.flat=dev[devnum].flat;
	     dk.akt=dev[devnum].akt;
	     strcpy(dk.lastdate,dev[devnum].lastdate);
	     dk.qatt=dev[devnum].qatt;
	     dk.qerrors=dev[devnum].qerrors;
	     dk.conn=dev[devnum].conn;
	     strcpy(dk.devtim,dev[devnum].devtim);
	     dk.chng=dev[devnum].chng;
	     dk.req=dev[devnum].req;
	     dk.source=dev[devnum].source;
	     strcpy(dk.name,dev[devnum].name);
	    }
	 if (debug>0) ULOGW ("[%05d][0x%x] %s (%d,%d)type=%d,Adr=%d,flat=%d",dev[devnum].id,dev[devnum].idd,dev[devnum].name,dev[devnum].port,dev[devnum].speed,dev[devnum].type,dev[devnum].adr,dev[devnum].flat);
	 devnum++;
	}
     if (res_) mysql_free_result(res_);
     LoadDKConfig(); 
     return 1;
    }
 else 
    {
     dbase.sqldisconn();
     return 0;
    }
//--------------------------------------------------------------------------------------    
 dbase.sqldisconn();
 return 1;
}
//----------------------------------------------------------------------------
// create thread read variable from channal
// create thread evaluate
void * dispatcher (void * thread_arg)
{
 time_t tim;
 float	ct=0;
 int who = RUSAGE_SELF; 
 struct rusage usage; 

 tim=time(&tim);
 currenttime=localtime(&tim); // get current system time
 WorkRegim=TRUE;
 if (debug>0) ULOGW ("starting dispatcher........success");

 while (!isConfigurated) sleep(10);	// sleep before configuring 
 StartThreads ();

 DWORD	secc=3600*172;
 CHAR	dat[20]={0};
 dbase.sqlconn("dk","root","");			// connect to database
 sprintf (querys,"set character_set_client='koi8r'"); dbase.sqlexec(querys); 
 sprintf (querys,"set character_set_results='koi8r'"); dbase.sqlexec(querys);
 sprintf (querys,"set collation_connection='koi8r_general_ci'"); dbase.sqlexec(querys);

 sleep (10);
 
 while (secc--)
    {
     tim=time(&tim);
     currenttime=localtime(&tim); // get current system time
     sprintf (dk.devtim,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1930,currenttime->tm_mon+1,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);
     res=dbase.sqlexec("UPDATE device SET lastdate=lastdate,devtim=NULL WHERE type=7"); 
     if (res) mysql_free_result(res);
     LoadDKConfig();
     //glibtop_init();
     //glibtop_get_cpu (&cpu1);
     //sleep (1);
     //glibtop_get_cpu (&cpu2);
     //ct=100*(((unsigned long)cpu2.user-(unsigned long)cpu1.user)+((unsigned long)cpu2.nice-(unsigned long)cpu1.nice)+((unsigned long)cpu2.sys-(unsigned long)cpu1.sys));
     //ct/=((unsigned long)cpu2.total-(unsigned long)cpu1.total);
     getrusage(who,&usage);
     //ULOGW ("%ld %ld %ld %ld %ld %ld %ld %ld",(unsigned long)cpu2.user,(unsigned long)cpu1.user,(unsigned long)cpu2.nice,(unsigned long)cpu1.nice,(unsigned long)cpu2.sys,(unsigned long)cpu1.sys,(unsigned long)cpu2.total,(unsigned long)cpu1.total);
     sprintf (querys,"INSERT INTO stat(type,cpu,mem) VALUES('1','%f','%d')",ct,usage.ru_maxrss);
     //ULOGW ("%s",querys);
     dbase.sqlexec(querys);

     StartThreads ();
     if (debug>0) ULOGW ("create all device thread");
     // start all device type thread
     sprintf (querys,"UPDATE info SET date=NULL");
     res_=dbase.sqlexec(querys);

     if (dk.regim==0) break;
     sleep(60);
    }

 dbase.sqldisconn();
 WorkRegim=FALSE;
 sleep(5);
 if (debug>0) ULOGW ("dispatcher finished");
}
//----------------------------------------------------------------------------
void StartThreads (void)
{
 if (debug>3) ULOGW ("create all device thread");
 // start all device type thread
 //ULOGW ("[kernel] pthread [km5-%d] [lk-%d] [irp-%d] [tek-%d] [ce-%d] | thread  [km5-%d] [lk-%d] [irp-%d] [tek-%d] [ce-%d]",dk.pth_km5,dk.pth_lk,dk.pth_irp,dk.pth_tek,dk.pth_ce,km5_thread,lk_thread,irp_thread,tek_thread,ce_thread);

 if (dk.pth[TYPE_INPUTKM] && !km5_thread)
 if(pthread_create(&thr,NULL,kmDeviceThread,(void *)&thrdnum) != 0)
    ULOGW ("%serror create server thread%s",kernel_color,nc);
 else { if (thr) pthread_detach (thr); km5_thread=TRUE; thrdnum++; }

 if (dk.pth[TYPE_LK] && !lk_thread)
 if(pthread_create(&thr,NULL,lkDeviceThread,(void *)&dk.device) != 0)
    ULOGW ("%serror create server thread%s",kernel_color,nc);
 else { if (thr) pthread_detach (thr); lk_thread=TRUE; thrdnum++; }

 if (dk.pth[TYPE_IRP] && !irp_thread)
 if(pthread_create(&thr,NULL,irpDeviceThread,(void *)&dk.device) != 0)    
    ULOGW ("%serror create server thread%s",kernel_color,nc);
 else { if (thr) pthread_detach (thr); irp_thread=TRUE; thrdnum++; }

 if (dk.pth[TYPE_INPUTVKT] && !vkt_thread)
 if(pthread_create(&thr,NULL,vktDeviceThread,(void *)&dk.device) != 0)
    ULOGW ("%serror create server thread%s",kernel_color,nc);
 else { if (thr) pthread_detach (thr); vkt_thread=TRUE; thrdnum++; }

 if (dk.pth[TYPE_INPUTELF] && !elf_thread)
 if(pthread_create(&thr,NULL,elfDeviceThread,(void *)&dk.device) != 0)
    ULOGW ("%serror create server thread%s",kernel_color,nc);
 else { if (thr) pthread_detach (thr); elf_thread=TRUE; thrdnum++; }

 if (dk.pth[TYPE_INPUTTEKON] && !tek_thread)
 if(pthread_create(&thr,NULL,tekonDeviceThread,(void *)&dk.device) != 0)
    ULOGW ("%serror create server thread%s",kernel_color,nc);
 else { if (thr) pthread_detach (thr); tek_thread=TRUE; thrdnum++; }

 if (dk.pth[TYPE_INPUTCE] && !ce_thread)
 if(pthread_create(&thr,NULL,ceDeviceThread,(void *)&dk.device) != 0)
    ULOGW ("%serror create server thread%s",kernel_color,nc);
 else { if (thr) pthread_detach (thr); ce_thread=TRUE; thrdnum++; }

 if (dk.pth[TYPE_CE303] && !threads[TYPE_CE303])
 if(pthread_create(&thr,NULL,cemDeviceThread,(void *)&dk.device) != 0)
    ULOGW ("%serror create server thread%s",kernel_color,nc);
 else { if (thr) pthread_detach (thr); threads[TYPE_CE303]=TRUE; thrdnum++; }

 if (dk.pth[TYPE_MERCURY230] && !threads[TYPE_MERCURY230])
 if(pthread_create(&thr,NULL,merDeviceThread,(void *)&dk.device) != 0)
    ULOGW ("%serror create server thread%s",kernel_color,nc);
 else { if (thr) pthread_detach (thr); threads[TYPE_MERCURY230]=TRUE; thrdnum++; }

 if (dk.pth[TYPE_SET_4TM] && !threads[TYPE_MERCURY230])
 if(pthread_create(&thr,NULL,merDeviceThread,(void *)&dk.device) != 0)
    ULOGW ("%serror create server thread%s",kernel_color,nc);
 else { if (thr) pthread_detach (thr); threads[TYPE_MERCURY230]=TRUE; thrdnum++; }

 if (dk.pth[TYPE_INPUTTSRV] && !threads[TYPE_INPUTTSRV])
 if(pthread_create(&thr,NULL,tsrvDeviceThread,(void *)&dk.device) != 0)
    ULOGW ("%serror create TSRV thread%s",kernel_color,nc);
 else { if (thr) pthread_detach (thr); threads[TYPE_INPUTTSRV]=TRUE; thrdnum++; }

// if (dk.formxml && !srv_thread)
// if(pthread_create(&thr,NULL,serviceThread,(void *)&thrdnum) != 0)
//    ULOGW ("%serror create server thread%s",kernel_color,nc);
// else { if (thr) pthread_detach (thr); srv_thread=TRUE; thrdnum++; }

// if (dk.pth[TYPE_SPG761] && !lk_thread)
// if(pthread_create(&thr,NULL,lkDeviceThread,(void *)&dk.device) != 0)
//     ULOGW ("error create local concentrator thread");
// else { if (thr) pthread_detach (thr); lk_thread=TRUE; thrdnum++; }

// if (dk.pth[10] && !panel_thread)
// if(pthread_create(&thr,NULL,panelDeviceThread,(void *)&dk.device) != 0)
//    ULOGW ("%serror create server thread%s",kernel_color,nc);
// else { if (thr) pthread_detach (thr); panel_thread=TRUE; thrdnum++; }

 if (dk.crq_enabl && !crq_thread)
 if(pthread_create(&thr,NULL,StartHttpSrv,NULL) != 0) 
    ULOGW ("%serror create server thread%s",kernel_color,nc);
 else { if (thr) pthread_detach (thr); crq_thread=TRUE; thrdnum++; }
}
//----------------------------------------------------------------------------
// load all configuration from DB to class DK
BOOL LoadDKConfig()
{
 // load from db all lk devices
 // if (res) mysql_free_result(res);
 res_=dbase.sqlexec("SELECT * FROM dev_dk"); 
 if (res_)
    { 
     UINT nr=mysql_num_rows(res_);
     row=mysql_fetch_row(res_);
     if (row)
        {
         dk.iddk=atoi(row[0]);
	 dk.device=atoi(row[1]);
	 dk.adr=atoi(row[2]);
	 strcpy (dk.ip,row[3]);
	 dk.build=atoi(row[4]);
	 dk.regim=atoi(row[5]);
	 dk.qzapr=atoi(row[6]);
	 dk.deep=atoi(row[7]);
	 dk.tmdt=atoi(row[8]);
	 dk.intlk=atoi(row[9]);
	 dk.inttek=atoi(row[10]);
	 dk.interp=atoi(row[11]);
	 strcpy (dk.last_date,row[12]);
	 dk.chng=(BOOL)atoi(row[13]);
	 strncpy (dk.address,row[14],50);
	 dk.log=atoi(row[15]);
	 debug=dk.log;
	 dk.knt_ent=atof(row[16]);
	 dk.knt_wat=atof(row[17]);
	 dk.emul=atoi(row[18]);
	 dk.irp_approx=atoi(row[19]);
	 dk.irp_addr_1=atoi(row[20]);
	 dk.irp_addr_2=atoi(row[21]);
	 dk.irp_addr_3=atoi(row[22]);
	 dk.irp_addr_4=atoi(row[23]);
	 dk.irp_addr_5=atoi(row[24]);
	 dk.nstrut=atoi(row[25]);
	 dk.nlevels=atoi(row[26]);
	 dk.nentr=atoi(row[27]);
	 dk.nflats=atoi(row[28]);
	 dk.maxent=atof(row[29]);
	 dk.maxtemp=atof(row[30]);
	 dk.datamin=atof(row[31]);
	 dk.datamax=atof(row[32]);
	 dk.main_ele=atoi(row[33]);
	 dk.main_heat=atoi(row[34]);
	 dk.main_wat=atoi(row[35]);
	 dk.heat=atoi(row[36]);
	 dk.formxml=atoi(row[37]);
	 dk.crq_enabl=atoi(row[38]);
	 dk.maxcrqlen=atoi(row[39]);
	 dk.crqport=atoi(row[40]);
         //dk.pth_ce=atoi(row[41]);
	 //dk.pth_irp=atoi(row[42]);
	 //dk.pth_km5=atoi(row[43]);
	 //dk.pth_lk=atoi(row[44]);
	 //dk.pth_tek=atoi(row[45]);
	 for (uint8_t pt=1; pt<30; pt++)    	    dk.pth[pt]=atoi(row[40+pt]);
	} 
     if (debug>0) ULOGW ("[dk] [0x%x][%s](%d,%d,%d,%d,%d,%d) [%s]",dk.device,dk.ip,dk.qzapr,dk.deep,dk.tmdt,dk.intlk,dk.inttek,dk.interp,dk.last_date);
     if (res_) mysql_free_result(res_);
    }
 if (debug>3) ULOGW ("[dk] spec (1) (%s)log[%d] ent|wat[%f|%f] emul[%d])",dk.address,dk.log,dk.knt_ent,dk.knt_wat,dk.emul);
 if (debug>3) ULOGW ("[dk] spec (2) irp approx(%d) addr[0x%x | 0x%x | 0x%x | 0x%x | 0x%x]",dk.irp_approx,dk.irp_addr_1,dk.irp_addr_2,dk.irp_addr_3,dk.irp_addr_4,dk.irp_addr_5);
 if (debug>3) ULOGW ("[dk] spec (3) irp str[%d] lev[%d] ent[%d] flt[%d]",dk.nstrut,dk.nlevels,dk.nentr,dk.nflats);
 if (debug>3) ULOGW ("[dk] spec (4) irp maxent=%f | maxtemp=%f | datamin=%f | datamax=%f",dk.maxent,dk.maxtemp,dk.datamin,dk.datamax);
 if (debug>3) ULOGW ("[dk] spec (5) irp main ele[%d] heat[%d] wat[%d] heat_alg[%d] | crq_enabl=%d | crqport=%d",dk.main_ele,dk.main_heat,dk.main_wat,dk.heat,dk.crq_enabl,dk.crqport);
}
