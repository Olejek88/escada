//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "modbus/modbus.h"
#include "panel.h"
#include "db.h"
//-----------------------------------------------------------------------------
static	MYSQL_RES 	*res;
static	db		dbase;
static 	MYSQL_ROW 	row;

static 	CHAR   		query[500];
static 	INT		fd;
static 	termios 	tio;

static	UINT	data_adr;	// for test we remember what we send to device
static	UINT	data_len;	// data address and lenght
static	UINT	data_op;	// and last operation

extern 	"C" BOOL WorkRegim;	// work regim flag
extern 	"C" tm *currenttime;	// current system time
extern	float irpData	[MAX_DEVICE_IRP][6];

static 	union fnm fnum[5];

VOID    ULOGW (const CHAR* string, ...);              // log function
UINT 	baudrate (UINT baud);			// baudrate select
float	Qdom=0,Qst=0,Qpr=0;

extern  "C" UINT	debug;
extern  float   bitData	[MAX_DEVICE_BIT][6];
extern  float	ip2Data	[MAX_DEVICE_2IP][10];
extern	BOOL	panel_thread;

static 	float	FlatData[MAX_FLATS];
static  BOOL 	OpenCom (SHORT blok, UINT speed);	// open com function
static  FLOAT LoadData (UINT dv, UINT prm, UINT pipe, UINT type, CHAR* data);
bool 	LoadMainData ();
BOOL LoadDataAll (UINT type);
//-----------------------------------------------------------------------------
static 	modbus_param_t mb_param;
//-----------------------------------------------------------------------------
void * panelDeviceThread (void * devr)
{
 int devdk=*((int*) devr);
 BYTE	data[8000];				// output buffer
 uint16_t	cn=0;
 float rt=0;
 float	t1,t2,t3,t4,v1,m1,q1,q2,q3,q4,q5,v2,m2,p1,p2,q6,vg,vh,v1n,v2n,q1n,q2n,q3n,tisp,tnis,g1n,g2n; 
 dbase.sqlconn("","root","");			// connect to database
 // open port for work
 BOOL rs=OpenCom (0, 115200);
 sleep (5);
 while (WorkRegim)
    {
     t1=LoadData (4001, 4, 0, 0, (CHAR *)"20120101000000");
     t2=LoadData (4001, 4, 1, 0, (CHAR *)"20120101000000");
     //t3=LoadData (84606978, 4, 3, 0, (CHAR *)"20120101000000");
     //t4=LoadData (84606978, 4, 4, 0, (CHAR *)"20120101000000");
     p1=LoadData (4001, 16, 0, 0, (CHAR *)"20120101000000");
     p2=LoadData (4001, 16, 1, 0, (CHAR *)"20120101000000");
     v1=LoadData (4001, 12, 0, 0, (CHAR *)"20120101000000");
     v2=LoadData (4001, 12, 1, 0, (CHAR *)"20120101000000");
     q1=LoadData (4001, 13, 0, 0, (CHAR *)"20120101000000");
     q2=LoadData (4001, 13, 1, 0, (CHAR *)"20120101000000");
     q3=LoadData (4001, 13, 2, 0, (CHAR *)"20120101000000");
     q5=0;
     q6=0;
     //q4=LoadData (84606978, 13, 3, 0, (CHAR *)"20120101000000");
     //vg=LoadData (84606978, 12, 5, 0, (CHAR *)"20120101000000");
     //vh=LoadData (84606978, 12, 6, 0, (CHAR *)"20120101000000");
     v1n=LoadData (84606978, 12, 12, 0, (CHAR *)"20120101000000");
     v2n=LoadData (84606978, 12, 11, 0, (CHAR *)"20120101000000");
     //q1n=LoadData (84606978, 13, 11, 0, (CHAR *)"20120101000000");
     //q2n=LoadData (84606978, 13, 12, 0, (CHAR *)"20120101000000");
     q3n=LoadData (84606978, 13, 13, 0, (CHAR *)"20120101000000");
     //g1n=LoadData (84606977, 12, 12, 0, (CHAR *)"20120101000000");
     //g2n=LoadData (84606977, 12, 11, 0, (CHAR *)"20120101000000");

     tisp=LoadData (4000, 18, 0, 0, (CHAR *)"20120101000000");
     tnis=LoadData (4000, 18, 1, 0, (CHAR *)"20120101000000");

     LoadMainData();
     LoadDataAll (0);


//     ULOGW("[tek] t1=%f t2=%f t3=%f t4=%f v1=%f q1=%f q2=%f v2=%f q3=%f vg=%f vh=%f",t1,t2,t3,t4,v1,q1,q2,v2,q3,vg,vh);

     memcpy (data,&t1,sizeof(t1));
     memcpy (data+4,&t2,sizeof(t2));
     memcpy (data+8,&t3,sizeof(t3));
     memcpy (data+12,&t4,sizeof(t4));
     memcpy (data+16,&p1,sizeof(p1));
     memcpy (data+20,&p2,sizeof(p2));
     memcpy (data+24,&v1,sizeof(v1));
     memcpy (data+28,&v2,sizeof(v2));
     memcpy (data+32,&q1,sizeof(q1));
     memcpy (data+36,&q2,sizeof(q2));
     memcpy (data+40,&q3,sizeof(q3));
     memcpy (data+44,&q4,sizeof(q4));
     memcpy (data+48,&q5,sizeof(q5));
     memcpy (data+52,&q6,sizeof(q6));
     memcpy (data+56,&vg,sizeof(vg));
     memcpy (data+60,&vh,sizeof(vh));

     memcpy (data+64,&g1n,sizeof(v1n));
     memcpy (data+68,&g2n,sizeof(v2n));
     memcpy (data+72,&q1n,sizeof(q1n));
     memcpy (data+76,&q1n,sizeof(q1n));
     memcpy (data+80,&q2n,sizeof(q2n));
     memcpy (data+84,&q3n,sizeof(q3n));
     memcpy (data+88,&v1n,sizeof(v1n));
     memcpy (data+92,&v2n,sizeof(v2n));
     memcpy (data+96,&tisp,sizeof(tisp));
     memcpy (data+100,&tnis,sizeof(tnis));

     memcpy (data+104,&Qdom,sizeof(Qdom));
     memcpy (data+108,&Qst,sizeof(Qst));
     if (Qdom>0) Qpr=Qst/Qdom;

     preset_multiple_registers(&mb_param, 1, 10, 56, (uint16_t *)data);

     memcpy (data,&irpData,sizeof(irpData));
//     for (cn=0;cn<300;cn++)         ULOGW ("[mt][%d] data 0x%x",cn,data[cn]);
/*
     preset_multiple_registers(&mb_param, 1, 100, 40, (uint16_t *)data);
     preset_multiple_registers(&mb_param, 1, 100+40, 40, (uint16_t *)data+40);
     preset_multiple_registers(&mb_param, 1, 100+120, 60, (uint16_t *)data+120);
     preset_multiple_registers(&mb_param, 1, 100+180, 60, (uint16_t *)data+180);
     preset_multiple_registers(&mb_param, 1, 100+240, 60, (uint16_t *)data+240);
     preset_multiple_registers(&mb_param, 1, 100+300, 60, (uint16_t *)data+300);
     preset_multiple_registers(&mb_param, 1, 100+360, 60, (uint16_t *)data+360);
*/
     for (cn=0;cn<MAX_DEVICE_IRP/10;cn++)	{
    	    preset_multiple_registers(&mb_param, 1, 100+cn*100, 60, (uint16_t *)data+cn*100);
    	    preset_multiple_registers(&mb_param, 1, 100+cn*100+60, 60, (uint16_t *)data+cn*100+60);
    	    preset_multiple_registers(&mb_param, 1, 100+cn*100+120, 60, (uint16_t *)data+cn*100+120);
    	    preset_multiple_registers(&mb_param, 1, 100+cn*100+180, 60, (uint16_t *)data+cn*100+180);
	    }

     memcpy (data,&FlatData,sizeof(FlatData));
     preset_multiple_registers(&mb_param, 1, 3000, 60, (uint16_t *)data);
     preset_multiple_registers(&mb_param, 1, 3000+60, 60, (uint16_t *)data+60);
     preset_multiple_registers(&mb_param, 1, 3000+120, 60, (uint16_t *)data+120);
     preset_multiple_registers(&mb_param, 1, 3000+180, 60, (uint16_t *)data+180);
     preset_multiple_registers(&mb_param, 1, 3000+240, 60, (uint16_t *)data+240);

     //memcpy (data,&ip2Data,sizeof(ip2Data));
     //preset_multiple_registers(&mb_param, 1, 500, MAX_DEVICE_2IP, (uint16_t *)data);

     //memcpy (data,&bitData,sizeof(bitData));
     //preset_multiple_registers(&mb_param, 1, 1100, MAX_DEVICE_BIT*4, (uint16_t *)data);

     sleep (3);
    }
 dbase.sqldisconn();
 modbus_close(&mb_param);

 if (debug>0) ULOGW ("[mt] panel thread end");
 panel_thread=FALSE;
 pthread_exit (NULL);
}
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 speed=115200;
 sprintf (devp,"/dev/ttyS0",blok);
 modbus_init_rtu(&mb_param, devp, speed, "even", 8, 1);
 modbus_set_debug(&mb_param, FALSE);
 if (modbus_connect(&mb_param) == -1) 
    {
     if (debug>0) ULOGW("[mt] error open com-port %s",devp); 
     return FALSE;
    }
 else if (debug>1) ULOGW("[mt] open com-port success"); 
 return TRUE;
}
//---------------------------------------------------------------------------------------------------
BOOL LoadDataAll (UINT type)
{
 UINT	bitn=0,ip2n=0,r,nr,flat;
 float rt;

 for (r=0;r<150;r++) FlatData[r]=0;

 sprintf (query,"SELECT * FROM data WHERE flat>0 AND prm=13 AND date>=20140101000000");
 //if (debug>2)  ULOGW("[mt] %s",query);
 res=dbase.sqlexec(query); 
 if (res)
    {
     nr=mysql_num_rows(res);
     for (r=0;r<nr;r++)
        {
	 row=mysql_fetch_row(res);
         rt=atof(row[3]);
	 flat=atoi(row[4]);
         if (flat<150) FlatData[flat-1]+=rt/4168;
         //ULOGW ("FlatData[%d]=%f",flat,FlatData[flat]);
        }
     if (res) mysql_free_result(res);
    }
 return rt;
}
//---------------------------------------------------------------------------------------------------
FLOAT LoadData (UINT dv, UINT prm, UINT pipe, UINT type, CHAR* data)
{
 float	rt=-1;
 if (type>0) sprintf (query,"SELECT * FROM data WHERE source=%d AND type=%d AND prm=%d AND flat=0 AND date=%s",pipe,type,prm,data);
 else sprintf (query,"SELECT * FROM prdata WHERE pipe=%d AND type=%d AND prm=%d AND device=%d",pipe,type,prm,dv);
 //if (debug>2)  ULOGW("[mt] %s",query);
 res=dbase.sqlexec(query); 
 if (row=mysql_fetch_row(res))
    {     
     if (type>0) rt=atof(row[3]);
     if (type==0) rt=atof(row[5]);
     //ULOGW("[mt] %s [%f]",query,rt);
    }
 if (res) mysql_free_result(res);
 return rt;
}

//---------------------------------------------------------------------------------------------------
bool LoadMainData ()
{
 float	rt=-1;
 UINT	nr=0,r;
 sprintf (query,"SELECT * FROM data WHERE source=0 AND type=2 AND prm=13 AND flat=0 ORDER BY date DESC LIMIT 1");
 if (debug>2)  ULOGW("[mt] %s",query);
 res=dbase.sqlexec(query); 
 if (row=mysql_fetch_row(res))
    {     
     Qdom=atof(row[3]);
     //ULOGW("[mt] %s [%f]",query,rt);
    }
 if (res) mysql_free_result(res);
 sprintf (query,"SELECT * FROM prdata WHERE pipe=0 AND type=2 AND prm=13 AND device<80000000 ORDER BY date DESC LIMIT 12");
 if (debug>2)  ULOGW("[mt] %s",query);
 res=dbase.sqlexec(query); 
 if (res)
    {
     nr=mysql_num_rows(res);
     for (r=0;r<nr;r++)
        {
	 row=mysql_fetch_row(res);
         Qst+=atof(row[5])/4168;
        }
     //ULOGW("[mt] %s [%f]",query,rt);
    }
 if (res) mysql_free_result(res);
 return rt;
}
