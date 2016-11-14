#include "types.h"
#include "main.h"
#include "errors.h"
#include "db.h"
#include "func.h"
#include "lk.h"
#include "irp.h"
#include "eval_flats.h"
//#include "kernel.h"
//----------------------------------------------------------------------------
static	db	dbase2;
static	 MYSQL_RES *res;
static	 MYSQL_ROW row;
static	MYSQL_RES *res_;
static	MYSQL_RES *res2;
static 	MYSQL_ROW row_;
static 	MYSQL_ROW row2;
static	CHAR querys[2500];
extern	uint16_t debug;
extern  Device2IP ip2[MAX_DEVICE_2IP];
extern	uint16_t	bit_num;
extern	uint16_t	irp_num;
extern	uint16_t	ip2_num;
extern	uint16_t	mee_num;

extern 	DeviceBIT 	bit[MAX_DEVICE_BIT];
extern 	Device2IP 	ip2[MAX_DEVICE_2IP];
extern 	DeviceIRP 	irp[MAX_DEVICE_IRP];
extern 	DeviceMEE 	mee[MAX_DEVICE_MEE];
extern 	Flats		flat[MAX_FLATS];
extern 	Fields		field[MAX_DEVICE_BIT];
extern 	Datas		data[10000];

extern	uint16_t	flat_num;
extern	uint16_t	field_num;
extern	uint16_t	data_num;

//----------------------------------------------------------------------------
bool	CalculateFlats	(UINT	type,  CHAR* date);
static  bool StoreDataFlats (UINT flat, UINT prm, UINT status, FLOAT value);
static  bool StoreDataFlats (UINT flat, UINT prm, UINT type, UINT status, FLOAT value, CHAR* date);
//----------------------------------------------------------------------------
// type = 0 - current values
// type = 1 - hours values
// type = 2 - days values
// type = 4 - month values
bool	CalculateFlats	(UINT	type,  CHAR* date)
{
 // 1. select all flats from db
 // 2. calculate all flats > current
 // 3. calculate all flats > archive
 // 4. calculate leaks
 // 5. distribute leaks on flats 
 //BOOL	napr[MAX_LEVEL+2][MAX_STRUTS+2]={0};
 //UINT	FL[MAX_LEVEL+2][MAX_STRUTS+2]={0};
 FLOAT	H[MAX_LEVEL+2][MAX_STRUTS+2]={0};
 FLOAT	H2[MAX_LEVEL+2][MAX_STRUTS+2]={0};
 FLOAT	SdH[MAX_STRUTS+2]={0};
 UINT	QN[MAX_STRUTS+2]={0};
 FLOAT	SQ[MAX_STRUTS+2]={0}; 
 FLOAT	Q[MAX_LEVEL+2][MAX_STRUTS+2]={0}; 
 FLOAT	K[MAX_LEVEL+2][MAX_STRUTS+2]={0}; 
 FLOAT	KS[MAX_FLATS+2]={0}; 
 FLOAT	Qflats[MAX_FLATS+2]={0};
 //FLOAT	Wflats[MAX_FLATS+2]={0};
 //FLOAT	Hflats[MAX_FLATS+2]={0}; 
 //FLOAT	Gflats[MAX_FLATS+2]={0}; 
 FLOAT	AVGQ=0,QSUM=0,QSUM2=0,QTEK=0,QPOT=0,QFL=0,GVSUM=0,SS=0,Hv[2]={0},Hn[2]={0},rr=0;
 int	k=0;
 flat_num=0;
 field_num=0;
 data_num=0;
 UINT 	nr,nr2,nr3;
 UINT 	y=0,t=0,r=0,st=0,lv=0; 
 CHAR	buf[500],dat2[30];

 if (debug>2) ULOGW ("CalculateFlats (%d, %s)",type,date);
 if (!dbase2.sqlconn("escada","root","")) return 0;

 if (type>0) sprintf (querys,"SELECT * FROM prdata WHERE type=%d AND prm=1 AND date=%s",type,date);
 if (debug>2) ULOGW ("%s",querys);
 res=dbase2.sqlexec(querys); 
 if (res)
    {
     nr=mysql_num_rows(res);
     for (r=0;r<nr;r++)
        {
	 row=mysql_fetch_row(res);
	 data[data_num].device=atoi(row[1]);
	 data[data_num].prm=atoi(row[2]);
	 data[data_num].value=atof(row[5]);
	 data[data_num].pipe=atoi(row[7]);
	 data_num++;
	}
     if (res) mysql_free_result(res);
    }

 // load all flats
 sprintf (querys,"SELECT * FROM flats ORDER BY flat");
 res=dbase2.sqlexec(querys); 
 if (res)
    {
     nr=mysql_num_rows(res);
     for (r=0;r<nr;r++)
        {
	 row=mysql_fetch_row(res);
	 flat[flat_num].id=atoi(row[0]);
	 flat[flat_num].flatd=atoi(row[1]);
	 flat[flat_num].level=atoi(row[2]);
	 flat[flat_num].rooms=atoi(row[3]);
	 flat[flat_num].nstrut=atoi(row[4]);
	 flat[flat_num].square=atoi(row[8]);
	 strcpy (flat[flat_num].name,row[5]);
	 strcpy (flat[flat_num].lasthour,row[6]);
	 strcpy (flat[flat_num].lastday,row[7]);
	 if (debug>3) ULOGW ("[%d] Flat (%s) [%d][%d|%d|%d])",r,flat[flat_num].name,flat[flat_num].flatd,flat[flat_num].level,flat[flat_num].rooms,flat[flat_num].nstrut);
	 flat_num++;
	}
     if (res) mysql_free_result(res);
    }
 sprintf (querys,"SELECT * FROM field WHERE type=1 ORDER BY flat");
 res=dbase2.sqlexec(querys); 
 if (res)
    { 
     nr=mysql_num_rows(res);
     for (r=0;r<nr;r++)
        {
	 row=mysql_fetch_row(res);
	 field[field_num].id=atoi(row[0]);
	 field[field_num].mnem=atoi(row[1]);
	 field[field_num].id1=atoi(row[4]);
	 field[field_num].id2=atoi(row[5]);
	 field[field_num].flat=atoi(row[6]);
	 field[field_num].pip=atoi(row[8]);

         if (flat_num>0) for (t=0;t<flat_num;t++)
         if (field[field_num].flat==flat[t].flatd) lv=flat[t].level;

	 if (debug>3) ULOGW ("[%d] Field [%d][%d|%d|%d])",r,field[field_num].id1,field[field_num].id2,field[field_num].flat,field[field_num].pip);
	 Hn[0]=Hn[1]=Hv[0]=Hv[1]=0;
	 if (data_num>0)
         for (t=0;t<data_num;t++)
	    {
    	     if (data[t].device==field[field_num].id1 && data[t].pipe<2) Hv[data[t].pipe]=data[t].value;
	     if (data[t].device==field[field_num].id2 && data[t].pipe<2) Hn[data[t].pipe]=data[t].value;
	    }

	 if (bit_num>0)
         for (t=0;t<bit_num;t++)
	    {
    	     if (bit[t].device==field[field_num].id1)
    		{
        	 if (field[field_num].pip==0) H[lv][bit[t].strut_number]=Hv[0]*24;
    	         else H2[lv][bit[t].strut_number]=Hv[0]*24; st=bit[t].strut_number;
		 //napr[lv][bit[t].strut_number]=field[field_num].pip;
	         //FL[lv][bit[t].strut_number]=field[field_num].flat;
		}
    	     if (bit[t].device==field[field_num].id2)
    		{
        	 if (field[field_num].pip==0) H[lv][bit[t].strut_number]=Hn[0]*24;
    	         else H2[lv][bit[t].strut_number]=Hn[0]*24; st=bit[t].strut_number;
	         //napr[lv][bit[t].strut_number]=field[field_num].pip;
		 //FL[lv][bit[t].strut_number]=field[field_num].flat;
		}
	    }
	 if (irp_num>0)
         for (t=0;t<irp_num;t++)
	    {
    	     if (irp[t].device==field[field_num].id1) { H[MAX_LEVEL+1][irp[t].strut]=Hv[1]; H[0][irp[t].strut]=Hv[0];}
    	     if (irp[t].device==field[field_num].id2) { H[0][irp[t].strut]=Hn[0]; H[MAX_LEVEL+1][irp[t].strut]=Hn[1];}
	    }
	 //if (field[field_num].pip==0) SdH[st]+=Hv[0]-Hn[1];
         //if (debug>2) ULOGW ("[ker][%d][%d] [%f][%f][%f][%f](%f)(%f)",lv,st,Hv[0],Hv[1],Hn[0],Hn[1],Hv[0]-Hn[0],SdH[st]);
	 while (1)
	    {
             //if (debug>2) ULOGW ("[kernel][x][%d][%d] [%f][%f][%f][%f]",lv,st,Hv[0],Hv[1],Hn[0],Hn[1]);
             if (debug>2) ULOGW ("[kernel][x][%d][%d] [%f][%f]",lv,st,Hv[0],Hn[0]);
	     if ((Hv[0]-Hn[0])>0 && (Hv[0]-Hn[0])<500)
		{
    	         SdH[st]+=Hv[0]-Hn[0]; QN[st]++;
	         if (debug>2) ULOGW ("[kernel][%d][%d] [%f][%f](%f)(%f)[%d]",lv,st,Hv[0],Hn[0],Hv[0]-Hn[0],SdH[st],QN[st]);
		 break;
		}
	     break;
	    }
	 field_num++;
	}
     if (res) mysql_free_result(res);
    }
 if (debug>2) ULOGW ("bit_num=[%d] | data_num=[%d] | flat_num=[%d] | field_num=[%d]",bit_num,data_num,flat_num,field_num);

 sprintf (querys,"[SdH] ");
// [SdH] [88.77 (177.53)(3)][466.94 (933.89)(3)][241.41 (193.13)(5)][355.92 (444.91)(4)][470.02 (940.03)(3)][453.84 (1588.44)(2)][nan (nan)(0)][268.11 (536.22)(3)][634.60 (1269.21)(3)][323.07 (403.83)(4)][229.07 (801.73)(2)][432.18 (345.74)(5)][289.09 (144.55)(6)][342.75 (685.51)(3)][452.20 (904.39)(3)][52.00 (182.01)(2)][308.03 (385.03)(4)][575.19 (718.99)(4)][490.01 (1715.05)(2)][428.71 (857.41)(3)][337.59 (96.45)(7)][186.14 (148.91)(5)][628.09 (2198.32)(2)][406.91 (813.83)(3)][359.35 (718.70)(3

 for (t=1;t<=MAX_STRUTS;t++)
    {
     if (SdH[t]==0 || QN[t]==0) SdH[t]=888;
     else SdH[t]=(MAX_LEVEL-1)*SdH[t]/QN[t];
     sprintf (querys,"%s[%.2f (%d)]",querys,SdH[t],QN[t]);
    }
 if (debug>2) ULOGW ("%s",querys);

 if (type>0) sprintf (querys,"SELECT AVG(value) FROM prdata WHERE type=%d AND prm=13 AND date=%s AND pipe=0",type,date);
 if (debug>2) ULOGW ("%s",querys);
 res=dbase2.sqlexec(querys);
 if (res)
    {
     row=mysql_fetch_row(res);
     AVGQ=atof(row[0]);
     if (res) mysql_free_result(res);
    }

 // select Q strut from prom data
 if (type>0) sprintf (querys,"SELECT * FROM prdata WHERE type=%d AND prm=13 AND date<=%s AND pipe=0 ORDER BY date DESC LIMIT 500",type,date);
 if (debug>2) ULOGW ("%s",querys);
 for (t=0;t<irp_num;t++) SQ[irp[t].strut]=0;
 res=dbase2.sqlexec(querys);
 if (res)
    {
     nr=mysql_num_rows(res);
     sprintf (querys,"SQ[%d]",irp_num);
     for (y=0;y<nr;y++)
	{
	 row=mysql_fetch_row(res);
	 if (irp_num>0)
         for (t=0;t<irp_num;t++)
	 if (irp[t].device==atoi(row[1]) && SQ[irp[t].strut]==0)
	    {
    	     SQ[irp[t].strut]=atof(row[5]);
	     //if (SQ[irp[t].strut]<=0 || SQ[irp[t].strut]>1000000) SQ[irp[t].strut]=AVGQ;
	     QSUM2+=SQ[irp[t].strut];
	     sprintf (querys,"%s[(%d)%.4f]",querys,irp[t].strut,SQ[irp[t].strut]);
	    }
	}
     if (res) mysql_free_result(res);
    }
 for (t=0;t<irp_num;t++)
     if (SQ[irp[t].strut]<=10 || SQ[irp[t].strut]>1000000) 
        {
    	 SQ[irp[t].strut]=AVGQ;
    	 QSUM2+=SQ[irp[t].strut];
    	}
// if (res) mysql_free_result(res);
 if (debug>2) ULOGW ("%s",querys);

 //solver_sle(MAX_LEVEL+1, H, SQ, Q);
 //print_vector(N, x);
 for (r=0;r<flat_num;r++)	Qflats[flat[r].flatd]=0;

 // calculate delta entalpy and restore bad data
 for (y=0;y<field_num;y++)
    {
     if (bit_num>0)  for (t=0;t<bit_num;t++)
     if (bit[t].device==field[y].id1 || bit[t].device==field[y].id2) st=bit[t].strut_number;
     K[lv][st]=0;
     Hn[0]=Hn[1]=Hv[0]=Hv[1]=0;
     if (flat_num>0) for (t=0;t<flat_num;t++)
     if (field[y].flat==flat[t].flatd) lv=flat[t].level;

     if (data_num>0)
     for (t=0;t<data_num;t++)
	    {
    	     if (data[t].device==field[y].id1 && data[t].pipe<2) Hv[data[t].pipe]=data[t].value;
	     if (data[t].device==field[y].id2 && data[t].pipe<2) Hn[data[t].pipe]=data[t].value;
	    }
     if (bit_num>0)
     for (t=0;t<bit_num;t++)
	    {
    	     if (bit[t].device==field[y].id1)  st=bit[t].strut_number;
    	     if (bit[t].device==field[y].id2)  st=bit[t].strut_number;
	    }
     if (irp_num>0)
     for (t=0;t<irp_num;t++)
	    {
    	     if (irp[t].device==field[y].id1) { H[MAX_LEVEL+1][irp[t].strut]=Hv[1]; H[0][irp[t].strut]=Hv[0];}
    	     if (irp[t].device==field[y].id2) { H[0][irp[t].strut]=Hn[0]; H[MAX_LEVEL+1][irp[t].strut]=Hn[1];}
	    }

     SdH[MAX_STRUTS+1]=0;
     while (1)
        {
         if ((Hv[0]-Hn[0])>0 && (Hv[0]-Hn[0])<500) { SdH[MAX_STRUTS+1]=Hv[0]-Hn[0]; break; }
	 //if ((Hv[0]-Hn[1])>0 && (Hv[0]-Hn[1])<90) { SdH[MAX_STRUTS+1]=Hv[0]-Hn[1]; break; }
         //if ((Hn[0]-Hv[0])>0 && (Hn[0]-Hv[0])<90) { SdH[MAX_STRUTS+1]=Hn[0]-Hv[0]; break; }
         //if ((Hn[0]-Hv[1])>0 && (Hn[0]-Hv[1])<90) { SdH[MAX_STRUTS+1]=Hn[0]-Hv[1]; break; }
	 SdH[MAX_STRUTS+1]=SdH[st]/(MAX_LEVEL-1); break;
	}
     if (SdH[st]>0) K[lv][st]=(SdH[MAX_STRUTS+1])/SdH[st];
     else K[lv][st]=0;
     Q[lv][st]=K[lv][st]*SQ[st];		// Qi = Ki * Qst
     //10-17 15:25:53 K(1)[2][3]=0.000000(-nan/-nan) SQ[3]=232.475357 Q=0.000000
     //10-17 15:25:53 K(1)[2][4]=0.100000(172.858398/1728.583984) SQ[4]=197.360092 Q=19.736010
     if (debug>2) ULOGW ("K(%d)[%d][%d]=%f(%f/%f) SQ[%d]=%f Q=%f",field[y].flat,lv,st,K[lv][st],SdH[MAX_STRUTS+1],SdH[st],st,SQ[st],Q[lv][st]);
     Qflats[field[y].flat]+=Q[lv][st];
    }
 // for all flats

 for (r=0;r<flat_num;r++)
    {
     KS[flat[r].flatd]=flat[r].square; SS+=flat[r].square;
     QFL+=Qflats[flat[r].flatd];
     if (debug>2) ULOGW ("flats [%d] Q=%f",flat[r].flatd,Qflats[flat[r].flatd]);
     if (type==0) StoreDataFlats (flat[r].flatd, 13, 0, Qflats[flat[r].flatd]);	// change status to real
     if (type>0)  StoreDataFlats (flat[r].flatd, 13, type, 0, Qflats[flat[r].flatd], date);
     usleep(20000);
    }
 if (debug>3) ULOGW ("QSUM=%f, QSUM2=%f",QSUM,QSUM2); 

 if (type>0) sprintf (querys,"SELECT * FROM data WHERE flat=0 AND type=%d AND date=%s",type,date);
 if (debug>2) ULOGW ("%s",querys);
 res=dbase2.sqlexec(querys);
 if (res) 
    {
     nr=mysql_num_rows(res); 
     for (y=0;y<nr;y++)
	{
        row=mysql_fetch_row(res);
        if (atoi(row[8])==13 && atoi(row[6])==0) QTEK=4186*atof(row[3]);
        if (atoi(row[8])==13 && atoi(row[6])==2) QPOT=4186*atof(row[3]);
        }
     if (res) mysql_free_result(res);
    }

 if (debug>1) ULOGW ("Qtec=%f Qsum=%f",QTEK,QSUM2);

 for (r=1;r<=flat_num;r++)
    {
     //k=(6-flat[r-1].level);
     if (debug>2) ULOGW ("[kernel] [%d] Q=%f[%f]",r,(KS[r]/SS)*(QTEK-QFL),Qflats[r]);
     if ((QTEK-QFL)>0 && QFL>0) StoreDataFlats (r, 13, type, 1, (KS[r]/SS)*(QTEK-QFL),date);
    }

 if (debug>2) ULOGW ("[kernel] end Calculate Flats");
 dbase2.sqldisconn();
}
//---------------------------------------------------------------------------------------------------
BOOL StoreDataFlats (UINT flat, UINT prm, UINT status, FLOAT value)
{
 sprintf (querys,"SELECT * FROM data WHERE type=0 AND prm=%d AND flat=%d",prm,flat);
 if (debug>3) ULOGW ("[kernel] %s",querys);
 res2=dbase2.sqlexec(querys); 
 if (res2)
 if (row2=mysql_fetch_row(res2))
     sprintf (querys,"UPDATE data SET value=%f,status=%d WHERE type=0 AND prm=%d AND flat=%d",value,status,prm,flat);
 else sprintf (querys,"INSERT INTO data(flat,prm,type,value,status) VALUES('%d','%d','0','%f','%d')",flat,prm,value,status);
 if (debug>3) ULOGW ("[kernel] %s",querys);
 if (res2) mysql_free_result(res2);  
 res2=dbase2.sqlexec(querys); 
 if (res2) mysql_free_result(res2);  
 return true;
}
//---------------------------------------------------------------------------------------------------
BOOL StoreDataFlats (UINT flat, UINT prm, UINT type, UINT status, FLOAT value, CHAR* date)
{
 sprintf (querys,"SELECT * FROM data WHERE type=%d AND prm=%d AND flat=%d AND date=%s AND source=%d",type,prm,flat,date,status);
 if (debug>3) ULOGW ("[kernel] %s",querys);
 res2=dbase2.sqlexec(querys); 
 if (res2)
 if (row2=mysql_fetch_row(res2))
     sprintf (querys,"UPDATE data SET value=%f,source=%d,date=%s WHERE type='%d' AND prm='%d' AND flat='%d' AND date='%s' AND source='%d'",value,status,date,type,prm,flat,date,status);
 else sprintf (querys,"INSERT INTO data(flat,prm,type,value,source,date) VALUES('%d','%d','%d','%f','%d','%s')",flat,prm,type,value,status,date);
 if (debug>3) ULOGW ("[kernel] %s",querys);
 if (res2) mysql_free_result(res2); 
 res2=dbase2.sqlexec(querys); 
 if (res2) mysql_free_result(res2);  
 return true;
}
//---------------------------------------------------------------------------------------------------
