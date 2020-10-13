//----------------------------------------------------------------------------------------------------------
#include "types.h"
#include "main.h"
#include "dk8072.h"
//----------------------------------------------------------------------------------------------------------
static INT	fd;
static termios 	tio;
//--------------------------------------------------------------------
//BOOL aOpenCom (SHORT blok, UINT dev);
//UINT aPollDevice(INT device, INT devn, SHORT blok);
//INT  aScanBus (SHORT adr, UINT dev, UINT speed);
//DOUBLE aReadData (CHAR* com);
BOOL dOpenCom (SHORT blok, UINT speed);
VOID ULOGW (CHAR* string, ...);
UINT baudrate (UINT baud);
//--------------------------------------------------------------------
static MYSQL *mysql; 
static MYSQL_ROW row;
static MYSQL_RES *res;

static CHAR 	query[200];

extern  "C" DeviceR device[20];
extern	"C" chData  chd[200];
extern	"C" Channels  chan[200];
//-----------------------------------------------------------------------------
INT	devNum=0,bytes=0;
SHORT dErrorConnect=0;					// число ошибок связи
BOOL Outs (SHORT blok,UCHAR x,UCHAR y,CHAR* string,UCHAR flag);	// вывод на экран сообщения
UCHAR Ins(SHORT blok);						// считывание буфера клавиатуры
VOID SendMessage (UINT ePos,UINT hPos,UINT type);		// 
BOOL FormError(UINT code,CHAR& MessageText);			// формирование комментария к событию
VOID EvOut(VOID);						// меню вывода на дисплей событий
VOID Options(VOID);						// меню вывода на дисплей текущих настроек
VOID ZnOut(VOID);						// меню вывода на дисплей текущих значений
VOID TkOut(VOID);						// меню вывода на дисплей текущих значений
VOID ReportForm(VOID);					// меню вывода на печать
VOID OutValue (VOID);					// функция вывода значений
VOID OutName (UINT page);				// функция вывода имен значений
VOID OutEvents (UINT str);				// функция вывода событий
VOID OutCursor (SHORT x);				// перевод курсора в нужное место экрана
VOID InitUMenu (SHORT nMenu);				// вывод подменю вывода на печать
VOID Menu3(SHORT typ);					// меню вывода на печать
VOID OutArrows (UINT pos);				// стрелочки скроллинга
SHORT scr=0;						// переключатель события/комментарии
CHAR Keys[32];						// буфер нажатых клавиш
UINT eTotal=0,mTotal=0,rTotal=3;			// общее количество записей
UINT start=0,tPos=0,spd=1200,rPos=0,nParam=200;
UINT pTotal=0;
CHAR EventsBuf[4][21];					// буфер сообщений
CHAR ParamBuf[40];					// буфер параметров
CHAR ParamVal[20];					// буфер параметров
CHAR EventsBuf2[2][80];					// буфер комментариев к сообщениям
CHAR DataBuf[4][20];					// буфер вычисленных данных
CHAR ValueBuf[4][15];					// буфер значений
CHAR ValueName[4][13];					// буфер имен значений
BOOL ex=FALSE;						// флажок выхода из меню
BOOL grad=FALSE;					// флажок градусника
BOOL evrefresh=FALSE;					// флажок рефреша
VOID OutProgress ();					// вывод градусника
//-----------------------------------------------------------------------------
BOOL dInit(SHORT blok)					// инициализация работы
{
 Outs (blok,0,0,"",6);
 Outs (blok,0,0,"",1);
 Outs (blok,0,1,"текущие значения",0);
 Outs (blok,1,1,"просмотр данных",0);
 Outs (blok,2,1,"печать отчетов",0);
 Outs (blok,3,1,"настройки",0);
 Ins  (blok);				
 Outs (blok,3,1,"081C2A0808080808",11);	
 Outs (blok,3,1,"08080808082A1C08",10);
 return TRUE;
}
//-----------------------------------------------------------------------------
VOID ScreenSaver(VOID)					// хранитель экрана
{
 CHAR buf[25];
 struct tm *ttime;
 time_t tim;
 tim=time(&tim);
 ttime=localtime(&tim);
 sprintf (buf,"Теплоприбор-ЭКО"); 
 Outs (fd,1,2,buf,0);
 sprintf (buf,"%02d/%02d/%04d  %02d:%02d:%02d",ttime->tm_mday,ttime->tm_mon,ttime->tm_year,ttime->tm_hour,ttime->tm_mday,ttime->tm_min);
 Outs (fd,2,0,buf,0);
}
//-----------------------------------------------------------------------------
BOOL dDeInit(VOID)
{
 Outs (fd,0,0,"",1);
 close(fd);
 return TRUE;
}
//-----------------------------------------------------------------------------
VOID * dRead (VOID * devr)
{
 UINT blok=fd;
 DWORD pause = 500;
 time_t stime=0,vtime=0,ntime=0,etime=0;
 if (devNum)
	{
	 Outs (blok,0,0,"",1);
	 dInit(fd);
	 while (!ex)
		{
		 UCHAR bytes = Ins (blok);
		 if (bytes)
			{
			 for (UINT i=1;i<bytes;i++)	//ULOGW ("bytes=%d",bytes);
				{
				 if (debug>2) ULOGW ("[dk8072] Keys[%d]=%c",i,Keys[i]);
				 if (Keys[i]=='H' && scr==0) if (tPos<3) { Outs (blok,tPos,0," ",0); tPos++;}
				 if (Keys[i]=='F' && scr==0) if (tPos>0) { Outs (blok,tPos,0," ",0); tPos--;}
				 if (Keys[i]=='K' && scr==0)
					{
					 if (tPos==3) { stime=time(&stime); vtime=stime; Options(); break; }
					 if (tPos==2) { stime=time(&stime); vtime=stime; ReportForm(); break;}
					 if (tPos==1) { stime=time(&stime); vtime=stime; TkOut(); break; }
					 if (tPos==0) { stime=time(&stime); vtime=stime; ZnOut(); break; }
					}
				}
			}
		 etime = time(&ntime) - vtime;
		 vtime = time(&ntime);
		 //printf ("[display] proc load <%0.3f%%>\n",(DOUBLE)((DOUBLE)(etime*100)/(DOUBLE)(pause+etime)));
		 if (vtime-stime>30)
			{
			 if (!scr) Outs (fd,0,0,"",1);
			 //printf ("ScreenSaver ()");
			 ScreenSaver ();
			 Outs (blok,0,0,"",7);
			 scr=1; usleep (pause);
			}
		 else { Outs (blok,tPos,0,">",0);  Outs (blok,4,21,"",0); if (scr) { scr=0;  dInit(fd); } }
		}
	 Outs (blok,0,0,"",1);
	}
 else   if (debug>2) ULOGW ("[dk8072] read without scan or scan return 0");
}
//-----------------------------------------------------------------------------
VOID ZnOut(VOID)
{
 UINT ePos=0,hPos=0;
 BOOL kys=FALSE;
 DWORD pause = 500;
 time_t stime=0,vtime=0,ntime=0,etime=0;
 Outs (fd,0,0,"",1);
 SendMessage (ePos,hPos,1);
 usleep (500);
 OutName (0);
 if (eTotal>4) Outs (fd,3,0,"\\xB0",0);
 //----------------------------------------
 while (!ex)
	{
	 UCHAR bytes = Ins (fd);
	 if (bytes)
		{
		 for (UINT i=1;i<bytes;i++)
			{
			 ULOGW ("Keys[%d]=%c",i,Keys[i]);
			 if (Keys[i]=='H') if (ePos+4<eTotal) 
				{
				 ePos++; kys=TRUE;
				 OutArrows (ePos);
				}
			 if (Keys[i]=='F') if (ePos>0)
				{
				 ePos--; kys=TRUE;
				 OutArrows (ePos);
				}
			 if (Keys[i]=='I') { ePos=8; ex=TRUE;}
			 if (Keys[i]=='G') if (hPos<3) { hPos++; ePos=0; kys=TRUE; usleep(1800);}
			 if (Keys[i]=='E') if (hPos>0) { hPos--; ePos=0; kys=TRUE; usleep(1800);}
			}
		}
	 if (kys)
		{
		 //SendMessage(hWnd1,MSG_DISPLAY,hPos,(LPARAM)ePos); 
		 SendMessage(ePos,hPos,1); usleep (1000); OutValue (); OutName (hPos); kys=FALSE;
		}
	 else
		{
		 if (vtime-stime>2) { SendMessage(hPos,ePos,1);  OutValue (); stime=vtime;}
		 if (vtime-ntime>10) { SendMessage(hPos,ePos,1); OutName (hPos); ntime=vtime;}
		}
	 etime = time(&ntime)-vtime;
	 vtime = time(&ntime);
	 //printf ("[display] proc load <%0.3f%%>\n",(DOUBLE)((DOUBLE)(etime*100)/(DOUBLE)(pause+etime)));
	 usleep (pause);
	}
 if (ePos==8) ex=FALSE;		// esli ex ustanovili izvne - vihod globalniy
 dInit(fd);
}
//-----------------------------------------------------------------------------
VOID TkOut(VOID)
{
 time_t ctime=0,ntime=0;
 UINT ePos=0;
 Outs (fd,0,0,"",1);
 SendMessage(0,0,1); usleep (500);
 //----------------------------------------
 while (!ex)
	{
	 UCHAR bytes = Ins (fd);
	 if (bytes)
		{
		 for (UINT i=1;i<bytes;i++)
			{
			 ULOGW ("[dk8072] Keys[%d]=%c",i,Keys[i]);
			 if (Keys[i]=='I') { ePos=8; ex=TRUE;}
			}
		}
 	 ctime = time(&ctime);
	 if (ctime-ntime>2) 
		{
		 SendMessage(0,0,1);
		 Outs (fd,0,0,DataBuf[0],0); Outs (fd,1,0,DataBuf[1],0);
		 Outs (fd,2,0,DataBuf[2],0); Outs (fd,3,0,DataBuf[3],0); ntime=ctime;
		}
	}
 if (ePos==8) ex=FALSE;		// esli ex ustanovili izvne - vihod globalniy
 dInit(fd);
}
//-----------------------------------------------------------------------------
VOID EvOut(VOID)
{
 UINT ePos=0,str=0;
 Outs (fd,0,0,"",1);
 SendMessage(1,ePos,2); usleep (500);
 if (eTotal>0) Outs (fd,0,0,EventsBuf[0],0);
 if (eTotal>1) Outs (fd,1,0,EventsBuf[1],0);
 if (eTotal>2) Outs (fd,2,0,EventsBuf[2],0);
 if (eTotal>3) Outs (fd,3,0,EventsBuf[3],0);
 //----------------------------------------
 while (!ex)
    {
     UCHAR bytes = Ins (fd);
     if (bytes)
    	{
	 for (UINT i=1;i<bytes;i++)
	    {
	     ULOGW ("[dk8072] Keys[%d]=%c",i,Keys[i]);
 	     if (Keys[i]=='H') if (ePos+2<mTotal) { ePos++; SendMessage(1,ePos,2);}
	     if (Keys[i]=='F') if (ePos>0) { ePos--; SendMessage(1,ePos,2); }
	     if (Keys[i]=='I') { ePos=8; ex=TRUE;}
	     if (Keys[i]=='G') if (!str) { str=1; evrefresh=TRUE;}
	     if (Keys[i]=='E') if (str) { str=0; evrefresh=TRUE;}
	    }
	}
     if (evrefresh) { OutEvents (str); evrefresh=FALSE; }
    }
 if (ePos==8) ex=FALSE;		// esli ex ustanovili izvne - vihod globalniy
 dInit(fd); 
}
//-----------------------------------------------------------------------------
VOID Options(VOID)
{
 UINT ePos=0;
 Outs (fd,0,0,"",1);
 time_t ctime=0,ntime=0;
 SendMessage(ePos,0,3);
 //----------------------------------------
 while (!ex)
	{
	 UCHAR bytes = Ins (fd);
	 if (bytes)
		{
		 for (UINT i=1;i<bytes;i++)
			{
			 if (Keys[i]=='I') { ePos=8; ex=TRUE;}
			 if (Keys[i]=='G') if (ePos<pTotal) { Outs (fd,ePos,0," ",0); ePos++; }
			 if (Keys[i]=='E') if (ePos>0) { Outs (fd,ePos,0," ",0); ePos--;}
			}
		}
	 ctime = time(&ctime);
	 if (ctime-ntime>2000)
		{
		 SendMessage(ePos,0,3);
		 Outs (fd,0,0,"",1); Outs (fd,0,0,ParamBuf,0); Outs (fd,2,0,ParamVal,0);
		 ntime=ctime;
		}
	}
 if (ePos==8) ex=FALSE;
 dInit(fd);
}
//-----------------------------------------------------------------------------
VOID ReportForm(VOID)
{
 rPos=0;
 InitUMenu (1);
 while (!ex)
	{
	 Outs (fd,rPos,0,">",0);
	 OutProgress ();
	 UCHAR bytes = Ins (fd);
	 if (bytes)
		{
		 for (UINT i=1;i<bytes;i++)
			{
			 if (debug>2) ULOGW ("[dk8072] Keys[%d]=%c",i,Keys[i]);
			 if (Keys[i]=='H') if (rPos<3) { Outs (fd,rPos,0," ",0); rPos++;}
			 if (Keys[i]=='F') if (rPos>0) { Outs (fd,rPos,0," ",0); rPos--;}
			 if (Keys[i]=='K') { Menu3(rPos+1); rPos=0;	}
			 if (Keys[i]=='I') { rPos=8; ex=TRUE;}
			}
		}
	}
 if (rPos==8) ex=FALSE;		// esli ex ustanovili izvne - vihod globalniy
 dInit(fd); 
}
//-----------------------------------------------------------------------------
VOID Menu3(SHORT typ)
{
 CHAR buf[20];
 SHORT x=6,qnsh=30;
 UINT nRash=0;
 if (typ==4) qnsh=1;
 struct tm *ttime;
 time_t tim;
 tim=time(&tim);
 ttime=localtime(&tim);
 sprintf (buf,"%02d/%02d/%04d %02d:%02d",ttime->tm_mday,ttime->tm_mon,ttime->tm_year,ttime->tm_hour,ttime->tm_min);
 Outs (fd,0,0,"",1);
 if (typ==NS_MESSAGE)	Outs (fd,0,0,"нешт. ситуации т.0 ",0);
 if (typ==LOGIKA)	Outs (fd,0,0,"Логика пр.200  т.0 ",0);
 if (typ==SOURGE_ERROR) Outs (fd,0,0,"пер. электропитания",0);
 if (typ==ARCHIVE_DATA) Outs (fd,0,0,"архив значений т.0 ",0);
 Outs (fd,1,0,"расчетное время:   ",0);
 Outs (fd,2,0,buf,0);
 if (typ!=ARCHIVE_DATA && typ!=ARCHIVE_DATA)  Outs (fd,3,0,"вывести 30 записей",0);
 if (typ==ARCHIVE_DATA) Outs (fd,3,0,"  отчет за час   >",0);
 OutCursor (x);
 while (!ex)
    {
     UCHAR bytes = Ins (fd);
     if (bytes)
    	{
	 for (UINT i=1;i<bytes;i++)
    	    {
	     if (debug>2) ULOGW ("Keys[%d]=%c",i,Keys[i]);
	     if (Keys[i]=='H')
		{
		 switch (x)
		    {
		     case 0: if (ttime->tm_mday>1) ttime->tm_mday--;
		    	     else
				{
				 ttime->tm_mday = 31;
				}
			      break;
		     case 1: if (ttime->tm_mon>1) ttime->tm_mon--; else ttime->tm_mon=12; break;
    		     case 2: if (ttime->tm_year>2000) ttime->tm_year--; else ttime->tm_year=2099; break;
		     case 3: if (ttime->tm_hour>0) ttime->tm_hour--; else ttime->tm_hour=23; break;
		     case 4: if (ttime->tm_min>0) ttime->tm_min--; else ttime->tm_min=59; break;
		     case 5: if (qnsh>1) qnsh--; else if (typ!=4) qnsh=99; break;
		     case 6: if (nRash>1) nRash--; else nRash=rTotal; break;
		     case 7: if (nParam>MIN_ARCHIVE_NUM) nParam--;
    		    }
		 if (x==5) if (typ!=ARCHIVE_DATA) { sprintf (buf,"%02d",qnsh); Outs (fd,3,8,buf,0);}
		 if (x==5) if (typ==ARCHIVE_DATA)
				{
				 if (qnsh==1) Outs (fd,3,0,"  отчет за час   >",0);
				 if (qnsh==2) Outs (fd,3,0,"< отчет за сутки >",0);
				 if (qnsh==3) Outs (fd,3,0,"< отчет за месяц >",0);
				 if (qnsh==4) Outs (fd,3,0,"< отчет за год    ",0);
				}
		 if (x==6 && typ!=SOURGE_ERROR) 
			{ 
			 sprintf (buf,"%d",nRash); Outs (fd,0,17,buf,0); 
			}
		 if (x<5)
			{ 
			 sprintf (buf,"%02d/%02d/%04d %02d:%02d",ttime->tm_mday,ttime->tm_mon,ttime->tm_year,ttime->tm_hour,ttime->tm_min);
			 Outs (fd,2,0,buf,0); 
			}
		 if (x==7) 
			{ sprintf (buf,"%d",nParam); Outs (fd,0,10,buf,0); }
		 OutCursor (x);
		}
	     if (Keys[i]=='F')
		{
		 switch (x)
		    {
		     case 0: if (ttime->tm_mday<31) ttime->tm_mday++; else ttime->tm_mday=1; break;
    		     case 1: if (ttime->tm_mon<12) ttime->tm_mon++; else ttime->tm_mon=1; break;
		     case 2: if (ttime->tm_year<2099) ttime->tm_year++; else ttime->tm_year=2000; break;
		     case 3: if (ttime->tm_hour<23) ttime->tm_hour++; else ttime->tm_hour=0; break;
		     case 4: if (ttime->tm_min<59) ttime->tm_min++; else ttime->tm_min=0; break;
		     case 5: if (typ!=4) { if (qnsh<99) qnsh++; else qnsh=0;}
			     else { if (qnsh<4) qnsh++; else qnsh=0;}
			     break;
		     case 6: if (nRash<3) nRash++; break;
		     case 7: if (nParam<MAX_ARCHIVE_NUM) nParam++;
		    }
	     if (x==6 && typ!=SOURGE_ERROR) { sprintf (buf,"%d",nRash); Outs (fd,0,17,buf,0); }
	     if (x==5) if (typ!=ARCHIVE_DATA) { sprintf (buf,"%02d",qnsh); Outs (fd,3,8,buf,0);}
    	     if (x==5) if (typ==ARCHIVE_DATA)
		    {
		     if (qnsh==1) Outs (fd,3,0,"  отчет за час   >",0);
		     if (qnsh==2) Outs (fd,3,0,"< отчет за сутки >",0);
		     if (qnsh==3) Outs (fd,3,0,"< отчет за месяц >",0);
		     if (qnsh==4) Outs (fd,3,0,"< отчет за год    ",0);}
    		     if (x<5)
			{ 
			 sprintf (buf,"%02d/%02d/%04d %02d:%02d",ttime->tm_mday,ttime->tm_mon,ttime->tm_year,ttime->tm_hour,ttime->tm_min);
			 Outs (fd,2,0,buf,0); 
			}
		     if (x==7) { sprintf (buf,"%d",nParam); Outs (fd,0,10,buf,0); }
		     OutCursor (x);
		    }
	     if (Keys[i]=='E')
		{
		 if (x>0) x--;
		 else  if (typ==SOURGE_ERROR) x=5;
    		 if (typ==LOGIKA) x=7;
		 if (typ!=SOURGE_ERROR && typ!=LOGIKA) x=6;
		 OutCursor (x);
		}
	     if (Keys[i]=='G')
		{
		 if (typ!=SOURGE_ERROR && typ!=LOGIKA) if (x<6) x++; else x=0;
		 if (typ==SOURGE_ERROR) if (x<5) x++; else x=0;
		 if (typ==LOGIKA) if (x<7) x++; else x=0;
		 OutCursor (x);
		}
	     if (Keys[i]=='K')
		{
		 Outs (fd,0,0,"",1);
		 PrinterOut (ttime,typ,qnsh,nRash);
		 rPos=8; ex=TRUE;
		}
	     if (Keys[i]=='I') { rPos=8; ex=TRUE;}
	    }
	}
    }
 if (rPos==8) ex=FALSE;		// esli ex ustanovili izvne - vihod globalniy
 InitUMenu (1);
}
//-----------------------------------------------------------------------------
INT dScanBus (SHORT blok, UINT speed)
{
 devNum=0;
 UCHAR Out[] = {'$','0','1','M',0xd};
 UCHAR sBuf1[20],sBuf2[20];
 if (!dOpenCom (blok,speed))  return -1;
 read (fd, &sBuf1, 10);
 for (UINT nump=0;nump<3;nump++)
    {
     if (debug>2) ULOGW ("[dk8072] out=[$01M\r]");
     tcsetattr (fd,TCSAFLUSH,&tio);
     write (fd,&Out,5);
     usleep(300000);
     //ioctl (fd,FIONREAD,&bytes);
     //if (debug>2) ULOGW ("[dk8072] b.read = %d",bytes);
     //if (bytes) 
     bytes = read (fd, &sBuf1, 15);
     if (debug>2) ULOGW ("[dk8072] b.read = %d",bytes);
     for (INT i=0;i<bytes;i++) 
	{
         if (sBuf1[i]==0xd) { sBuf1[i]=0; break; }
	 if (debug>1) ULOGW ("[dk8072] [%d] 0x%x (%c)",i,sBuf1[i],sBuf1[i]);
	}
     BOOL bcFF_06 = FALSE;
     for (UINT cnt=0;cnt<10;cnt++)
	{
	 if (sBuf1[cnt]=='!')
	    if (sBuf1[cnt+1]=='0')	if (sBuf1[cnt+2]=='1')
    		{ bcFF_06 = TRUE; start=cnt+3;}
	}
     if (bcFF_06)
	{
	 devNum++;
	 if (debug>0) ULOGW("[dk8072] device found on address %d",0x1);
	 nump=10;
	}
    }
return devNum;
}
//---------------------------------------------------------------------------------------------------
VOID PrinterOut (tm *st, SHORT type, SHORT Period, SHORT pipes)
{
 struct tm *ttime;
 time_t tim;
 tim=time(&tim);
 ttime=localtime(&tim);
 //ULOGW ("[%d] (%d) <%d> %d:%d %d/%d/%d - %d:%d %d/%d/%d",sp.pipe,sp.type,sp.quant,stt.wHour,stt.wMinute,stt.wDay,stt.wMonth,stt.wYear,st.wHour,st.wMinute,st.wDay,st.wMonth,st.wYear);
}
//---------------------------------------------------------------------------------------------------
BOOL Outs(SHORT blok,UCHAR x,UCHAR y,CHAR* string,UCHAR flag)
{
 UINT dwbr1=0,i=0;
 UCHAR Out[] = {'$','0','1','T','0','0','0','0','0','0','\r'};
 UCHAR sBuf1[20];
 SHORT success=0;
 time_t ctime=0;
 BOOL bcFF_06 = FALSE;
 if (flag==0)
	{
	 Out[4]=x+48; Out[5]=y/16+48; Out[6]=y%16+48; if (y>9 && y<16) Out[6]=Out[6]+7;
	 for (i=7;i<strlen (string)+7;i++) { Out[i]=string[i-7]; /*ULOGW("0x%x",Out[i]);*/}
	 if (Out[i-1]==0xa) i--;
	 Out[i]=0xd;
	}
 if (flag==1)  { Out[3]='C'; Out[4]=0xd; i=4; }
 if (flag==2)  { Out[3]='S'; Out[4]=0xd; i=4; }
 if (flag==3)  { Out[3]='S'; Out[4]='D'; Out[5]=0xd; i=5;}
 //$AA0SSTT[chk](cr) // $730482035
 if (flag==4)  { Out[3]='0'; Out[4]='3'; Out[5]='0'; Out[6]='0'; Out[7]='0'; Out[8]=0xd; i=8;}	// lights off
 if (flag==5)  { Out[3]='0'; Out[4]='3'; Out[5]='0'; Out[6]='3'; Out[7]='0'; Out[8]=0xd; i=8;}	// lights on
 if (flag==6)  { Out[3]='0'; Out[4]='4'; Out[5]='8'; Out[6]='2'; Out[7]='0'; Out[8]='3'; Out[9]='5'; Out[10]=0xd; i=10;}	// lights on
 if (flag==7)  { Out[3]='0'; Out[4]='4'; Out[5]='8'; Out[6]='2'; Out[7]='0'; Out[8]='0'; Out[9]='0'; Out[10]=0xd; i=10;}	// lights on
 //if (flag==4)  { Out[3]='0'; Out[4]='\\'; Out[5]='4'; Out[6]='0'; Out[7]='2'; Out[8]=0xd; i=8;}	// lights off
 //if (flag==4)  { Out[3]='0'; Out[4]='4'; Out[5]='0'; Out[6]='2'; Out[7]='0'; Out[8]='3'; Out[9]='5'; Out[10]=0xd; i=10;}	// lights on
 //if (flag==5)  { Out[3]='0'; Out[4]='4'; Out[5]='8'; Out[6]='2'; Out[7]='0'; Out[8]='3'; Out[9]='5'; Out[10]=0xd; i=10;}	// lights on
 if (flag>=10 && flag<20) { Out[3]='Z'; Out[4]=flag+38; for (UINT oi=0;oi<16;oi++) Out[oi+5]=string[oi]; Out[21]=0xd; i=21;} 
 //if (flag==20) { Out[3]='Z'; Out[4]=flag-38; for (UINT oi=0;oi<16;oi++) Out[oi+5]=string[oi]; Out[21]=0xd; i=21;}
 //for (UINT po=0;po<=i;po++) ULOGW("Output %c",Out[po]);
 ctime = time(&ctime);
 if (flag==4 || flag==5) write (fd,&Out,i+1);
 else write (fd,&Out,i+1);

 sBuf1[0]=0; usleep(20);
 if (bytes) read (fd, &sBuf1, 10);
 for (UINT cnt=0;cnt<10;cnt++)
    {
     if (sBuf1[cnt]=='!')
	if (sBuf1[cnt+1]=='0')	if (sBuf1[cnt+2]=='1')
    	    {
	     bcFF_06 = TRUE; success=1;
	    }
    }
 if (success==0)
    {
     dErrorConnect++;
     if (dErrorConnect>10)
	{
	 dErrorConnect=0;
	}
     return FALSE;
    }
 else dErrorConnect=0;
 return TRUE;
}
//---------------------------------------------------------------------------------------------------
UCHAR Ins(SHORT blok)
{
 UINT start=0,dwbr1=0;
 UCHAR Out[] = {'$','0','1','K',0xD};
 UCHAR sBuf1[20]; UCHAR sBuf2[20];
 SHORT success=0;
// for (UINT i=0;i<=10;i++) 	Out[i] = (CHAR) sBuf[i];
 if (debug>1) ULOGW ("[dk8072] out=%s",Out);
 write (fd,&Out,5);
 ioctl (fd,FIONREAD,&bytes);
// if (bytes) read (fd, &sBuf1, 5);
 if (debug>1) ULOGW ("[dk8072] bytes read = %d",bytes);
 if (bytes) read (fd, &sBuf1, 10);
 for (INT i=0;i<bytes;i++) 
    {
     if (sBuf1[i]==0xd) { sBuf1[i]=0; break; }
     if (debug>1) ULOGW ("[dk8072] [%d] 0x%x (%c)",i,sBuf1[i],sBuf1[i]);
    }
 if (bytes)
 for (UINT cnt=0;cnt<10;cnt++)
    {
     if (sBuf1[cnt]=='!')
    	if (sBuf1[cnt+1]=='0')	if (sBuf1[cnt+2]=='1')
	    { 
	     success=1;
    	     start=cnt+3; //ULOGW ("success");
	    }
    }
 if (success==0)
    {
     dErrorConnect++;
     if (dErrorConnect>10)
	 return 0;
    }
 else dErrorConnect=0;
 for (INT i=start; i<bytes+dwbr1-1;i++)	 Keys[i-start]=sBuf1[i];
 return (UCHAR)bytes+dwbr1-1-start;
}
//---------------------------------------------------------------------------------------------------
BOOL dOpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 if (blok<10)  sprintf (devp,"/dev/ttyS%d",blok);
 if (blok>9)  sprintf (devp,"/dev/ttyr0%d",blok-10);
 if (debug>0) ULOGW("[dk8072] attempt open com-port %s",devp);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[dk8072] error open com-port %s [%s]",devp,strerror(errno)); 
     return FALSE;
    }
 else if (debug>1) ULOGW("[dk8072] open com-port success"); 
 //bzero (&tio,sizeof(tio));
 tcflush(fd,TCIOFLUSH);
 tcgetattr(fd, &tio);
 cfsetospeed(&tio, baudrate(speed));
 tio.c_cflag |= baudrate(speed)|CREAD|CLOCAL|CS8|CRTSCTS|PARODD;
// tio.c_cflag &= ~CSIZE;
 tio.c_cflag &=~(CSTOPB|PARENB|IXON|ECHO); //
// tio.c_lflag &= ~ICANON;
// tio.c_lflag &= ICANON;
// tio.c_cc[VMIN] = 0;
// tio.c_cc[VTIME] = 10; //Time out in 10e-1 sec
 cfsetispeed(&tio, baudrate(speed));
 fcntl(fd, F_SETFL, 0); // 
 tio.c_iflag = 0;
 tcsetattr(fd,TCSANOW,&tio);  
 return TRUE;
}
//---------------------------------------------------------------------------------------------------
VOID InitUMenu (SHORT nMenu)
{
 Outs (fd,0,0,"",1);
 if (nMenu==1)
	{
	 Outs (fd,0,1,"нештатные ситуации",0);
	 Outs (fd,1,1,"отчеты с логики   ",0);
	 Outs (fd,2,1,"пер.электропитания",0);
	 Outs (fd,3,1,"архивные значения",0);
	}
}
//-----------------------------------------------------------------------------
VOID OutName (UINT page)
{
// CHAR bf[4]; _itoa(page,bf,10);
// Outs (fd,1,0,bf,0);
// Outs (fd,2,0,bf,0);
 Outs (fd,0,18,"   ",0);
 Outs (fd,0,1,ValueName[0],0);
 Outs (fd,1,1,ValueName[1],0);
 Outs (fd,2,1,ValueName[2],0);
 Outs (fd,3,1,ValueName[3],0);
 Outs (fd,0,8,ValueBuf[0],0);
 Outs (fd,1,8,ValueBuf[1],0);
 Outs (fd,2,8,ValueBuf[2],0);
 Outs (fd,3,8,ValueBuf[3],0);
}
//-----------------------------------------------------------------------------
VOID OutValue (VOID)
{
 Outs (fd,0,8,ValueBuf[0],0);
 Outs (fd,1,8,ValueBuf[1],0);
 Outs (fd,2,8,ValueBuf[2],0);
 Outs (fd,3,8,ValueBuf[3],0);
}
//-----------------------------------------------------------------------------
VOID OutEvents (UINT str)
{
if (str)
    {
     Outs (fd,0,0,"",1);
     Outs (fd,0,0,EventsBuf2[0],0);
     Outs (fd,2,0,EventsBuf2[1],0);
    }
else
    {
     Outs (fd,0,0,"",1);
     Outs (fd,0,0,EventsBuf[0],0);
     Outs (fd,1,0,EventsBuf[1],0);
     Outs (fd,2,0,EventsBuf[2],0);
     Outs (fd,3,0,EventsBuf[3],0);
    }
}
//-----------------------------------------------------------------------------
VOID OutCursor (SHORT x)
{
switch (x)
	{
	 case 0: Outs (fd,2,1,"",0); break;
	 case 1: Outs (fd,2,4,"",0); break;
	 case 2: Outs (fd,2,9,"",0);  break;
	 case 3: Outs (fd,2,12,"",0); break;
	 case 4: Outs (fd,2,15,"",0); break;
	 case 5: Outs (fd,3,9,"",0);  break;
	 case 6: Outs (fd,0,17,"",0); break;
	 case 7: Outs (fd,0,12,"",0);
	}
}
//-----------------------------------------------------------------------------
VOID OutArrows (UINT pos)
{
 //if (pos+4<eTotal) Outs (fd,3,0,"\\xB0",0);
 if (pos+4<eTotal) Outs (fd,3,0,"^",0);
 else Outs (fd,3,0," ",0);
 if (pos>0) Outs (fd,0,0,"v",0);
 else Outs (fd,0,0," ",0);
}
//----------------------------------------------------------------------------------------------------------
VOID ReportForming (BOOL doit)
{
grad = doit;
}
//----------------------------------------------------------------------------------------------------------
VOID OutProgress ()
{
 if (grad)
	{
	 Outs (fd,0,0,"",1);
	 Outs (fd,1,0,"Производится печать",0);
	 UINT i=0; BOOL j=0;
	 while (grad)
		{
		 if (i<19)
			{
			 if (j) Outs (fd,2,i," ",0); 
			 else Outs (fd,2,i,"#",0);
			 usleep(500);
			 i++;
			}
		 else { i=0; j=!j;}
		}
	 Outs (fd,0,0,"",1);
	 InitUMenu (1);
	}
}
//--------
VOID SendMessage (UINT ePos,UINT hPos,UINT type)
{
 INT cnt=0;
 strcpy (ValueName[0],chan[ePos].name);
 strcpy (ValueName[1],chan[ePos+1].name);
 strcpy (ValueName[2],chan[ePos+2].name);
 strcpy (ValueName[3],chan[ePos+3].name);
 sprintf (ValueBuf[0],"%f",chd[ePos].current);
 sprintf (ValueBuf[1],"%f",chd[ePos+1].current);
 sprintf (ValueBuf[2],"%f",chd[ePos+2].current);
 sprintf (ValueBuf[3],"%f",chd[ePos+3].current);

 sprintf (query,"SELECT * FROM events ORDER BY id DESC");
 if (mysql_query(mysql, query))
 while ((row=mysql_fetch_row(res)))
     	{
	 if (cnt>=ePos && cnt<(ePos+5))
	     sprintf (EventsBuf[cnt-ePos],"[0x%x] %s",atoi(row[4]),row[1]);
	 cnt++;
	}
 //EventsBuf[0]
 //ValueName[0]
 //ValueBuf[0]
}
