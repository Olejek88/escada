//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "tecon_17.h"
#include "db.h"
//-----------------------------------------------------------------------------
static 	MYSQL_RES *res;
static	db	dbase;
static 	MYSQL_ROW row;

static 	CHAR   	query[500];	
static 	INT	fd;
static 	termios tio;

extern 	"C" UINT device_num;	// total device quant
extern 	"C" BOOL WorkRegim;	// work regim flag
extern 	"C" tm *currenttime;	// current system time

extern 	"C" DeviceR 	dev[MAX_DEVICE];	// device class
extern 	"C" DeviceTekon tekon;		// tekon class

static	union fnm fnum[5];
UINT	chan_num=0;

 // Buffers
 tecon_17_buff<tecon_17_return> Value;
 tecon_17_buff<tecon_17_arhive> Archive;
 tecon_17	tec;
 
VOID 	ULOGW (CHAR* string, ...);		// log function
UINT 	baudrate (UINT baud);			// baudrate select
static	BOOL 	OpenCom (SHORT blok, UINT speed);	// open com function
VOID 	Events (DWORD evnt, DWORD device);	// events 
static	BOOL 	LoadTekonConfig();			// load tekon configuration
static	BOOL 	StoreData (UINT dv, UINT prm, UINT type, UINT status, FLOAT value);		// store data to DB
static	BOOL 	StoreData (UINT dv, UINT prm, UINT type, UINT status, FLOAT value, time_t data);
//-----------------------------------------------------------------------------
void * tekonDeviceThread (void * devr)
{
 int devdk=*((int*) devr);			// DK identificator
 dbase.sqlconn("dk","root","");			// connect to database

 // load from db tekon device
 LoadTekonConfig();
 // open port for work
 BOOL rs=OpenCom (tekon.port, tekon.speed);
 if (!rs) return (0);
 
 while (WorkRegim)
 for (UINT r=0;r<chan_num;r++)
    {
     if (debug>1) ULOGW ("[tekon] ReadDataCurrent (%d)",r);
     tekon.ReadDataCurrent (r);	
     if (debug>1) ULOGW ("[tekon] ReadDataArchive (%d)",r);
     tekon.ReadDataArchive (r);
     // Device address in the network 
     sleep (1);     
    }
 dbase.sqldisconn();
 if (debug>0) ULOGW ("[tekon] tekon thread end");
}
//-----------------------------------------------------------------------------
// ReadDataCurrent - read single device. Readed data will be stored in DB
int DeviceTekon::ReadDataCurrent (UINT	sens_num)
{
 bool 	rs;
 tec.addr = tekon.adr;
 tec.port = (void *)fd;
 
 this->qatt++;	// attempt
 
 // Read values from sensor
 rs=Read(tec, // device which to be read
	T17_SENS_MOMENTARY, // Parametr to be read (Sensor Momentory value)
	sens_num, 	// Sensor num
	Value);		
 if (!rs) 
    {
     // send problem
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>15) this->akt=0; // !!! 5 > normal quant from DB
     Events ((3<<28)&TYPE_INPUTTEKON<<24&SEND_PROBLEM,this->device);
    }
 else
    { 
     this->akt=1;
     this->qerrors=0;
     this->conn=1;
     sprintf (this->lastdate,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,1+currenttime->tm_mon,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);
     if (debug>2) ULOGW ("[tekon] [%d] [%f]",sens_num,Value.ptr[0].f);
     StoreData (this->device, this->prm[sens_num], 0, 0, Value.ptr[0].f);
    }

 return 0;
}
//-----------------------------------------------------------------------------
// ReadDataArchive - read single device. Readed data will be stored in DB
int DeviceTekon::ReadDataArchive (UINT	sens_num)
{
 bool 	rs;
 tec.addr = tekon.adr;
 tec.port = (void *)fd;
 
 this->qatt++;	// attempt

 rs = Read(tec, _HOURS, tekon.n_ar[sens_num], time(NULL) - 60*60*24, time(NULL), Archive);
 if (rs) for(UINT i = 0; i< Archive.us; i++)
    {
     if (debug>0) ULOGW ("[tekon] [A%d] [%s] (%d)",1, ctime(&Archive.ptr[i].to), Archive.ptr[i].val.f, Archive.ptr[i].flag);
     StoreData (this->device, this->prm[sens_num], 1, Archive.ptr[i].flag, Archive.ptr[i].val.f, Archive.ptr[i].to);
    }

 rs = Read(tec, _DAYS, tekon.n_ar[sens_num], time(NULL) - 60*60*24*7, time(NULL), Archive);
 if (rs) for(UINT i = 0; i< Archive.us; i++)
    {
     if (debug>0) ULOGW ("[tekon] [A%d] [%s] (%d)",2, ctime(&Archive.ptr[i].to), Archive.ptr[i].val.f, Archive.ptr[i].flag);
     StoreData (this->device, this->prm[sens_num], 2, Archive.ptr[i].flag, Archive.ptr[i].val.f, Archive.ptr[i].to);
    }

 rs = Read(tec, _MONTHS, tekon.n_ar[sens_num], time(NULL) - 60*60*24*63, time(NULL), Archive);
 if (rs) for(UINT i = 0; i< Archive.us; i++)
    {
     if (debug>0) ULOGW ("[tekon] [A%d] [%s] (%d)",3, ctime(&Archive.ptr[i].to), Archive.ptr[i].val.f, Archive.ptr[i].flag);
     StoreData (this->device, this->prm[sens_num], 3, Archive.ptr[i].flag, Archive.ptr[i].val.f, Archive.ptr[i].to);
    }
    
 if (!rs) 
    {
     this->qerrors++;
     this->conn=0;
     if (this->qerrors>15) this->akt=0; // !!! 5 > normal quant from DB
     Events ((3<<28)&TYPE_INPUTTEKON<<24&SEND_PROBLEM,this->device);
    }
 else
    { 
     this->akt=1;
     this->qerrors=0;
     this->conn=1;
     sprintf (this->lastdate,"%04d%02d%02d%02d%02d%02d",currenttime->tm_year+1900,1+currenttime->tm_mon,currenttime->tm_mday,currenttime->tm_hour,currenttime->tm_min,currenttime->tm_sec);
     if (debug>2) ULOGW ("[tekon] [%d] [%f]",sens_num,Value.ptr[0].f);
    }

 return 0;
}
//--------------------------------------------------------------------------------------
// load all tekon configuration from DB
BOOL LoadTekonConfig()
{
 sprintf (query,"SELECT * FROM dev_tekon");
 res=dbase.sqlexec(query); 
 UINT nr=mysql_num_rows(res);
 for (UINT r=0;r<nr;r++)
    {
     row=mysql_fetch_row(res);
     tekon.idt=atoi(row[0]);
     if (!r)
     for (UINT d=0;d<device_num;d++)
     if (dev[d].type==TYPE_INPUTTEKON)
        {
         tekon.iddev=dev[d].id;
         tekon.device=atoi(row[1]);	     
	 tekon.SV=dev[d].SV;
    	 tekon.interface=dev[d].interface;
	 tekon.protocol=dev[d].protocol;
	 tekon.port=dev[d].port;
	 tekon.speed=dev[d].speed;
	 tekon.adr=dev[d].adr;
	 tekon.type=dev[d].type;
	 strcpy(tekon.number,dev[d].number);
	 tekon.flat=dev[d].flat;
	 tekon.akt=dev[d].akt;
	 strcpy(tekon.lastdate,dev[d].lastdate);
	 tekon.qatt=dev[d].qatt;
	 tekon.qerrors=dev[d].qerrors;
	 tekon.conn=dev[d].conn;
	 strcpy(tekon.devtim,dev[d].devtim);
	 tekon.chng=dev[d].chng;
	 tekon.req=dev[d].req;
	 tekon.source=dev[d].source;
	 strcpy(tekon.name,dev[d].name);
	}    
     tekon.n_ar[r]=atoi(row[1]);
     tekon.n_sen[r]=atoi(row[2]);     
     tekon.prm[r]=atoi(row[3]);
     tekon.n_pip[r]=atoi(row[4]);
     tekon.n_chn[r]=atoi(row[5]);
     chan_num++;     
    } 
 if (res) mysql_free_result(res);
 if (debug>0) ULOGW ("[tekon] total %d channels",chan_num);
}
//---------------------------------------------------------------------------------------------------
BOOL StoreData (UINT dv, UINT prm, UINT type, UINT status, FLOAT value)
{
 sprintf (query,"SELECT * FROM data WHERE type=0 AND prm=%d AND flat=0",prm);
 res=dbase.sqlexec(query); 
 if (row=mysql_fetch_row(res))
     sprintf (query,"UPDATE data SET value=%f,status=%d WHERE type=0 AND prm=%d AND device=0",value,status,prm);
 else sprintf (query,"INSERT INTO data(flat,prm,type,value,status) VALUES('0','%d','0','%f','%d')",dv,prm,value,status);
// ULOGW ("%s [%d]",query,row);
 res=dbase.sqlexec(query); 
 if (res) mysql_free_result(res);
 return true;
}
//---------------------------------------------------------------------------------------------------
BOOL StoreData (UINT dv, UINT prm, UINT type, UINT status, FLOAT value, time_t data)
{
 CHAR date[20];
 struct tm *ct;
 ct=localtime(&data); // get current system time
 if (type==1) sprintf (date,"%04d%02d%02d%02d0000",ct->tm_year+1900,1+ct->tm_mon,ct->tm_mday,ct->tm_hour);
 if (type==2) sprintf (date,"%04d%02d%02d000000",ct->tm_year+1900,1+ct->tm_mon,ct->tm_mday); 
 sprintf (query,"SELECT * FROM data WHERE type=%d AND prm=%d AND flat=0 AND date=%s",type,prm,date);
 ULOGW ("[irp] %s",query);
 res=dbase.sqlexec(query); 
 if (row=mysql_fetch_row(res))
     sprintf (query,"UPDATE data SET value=%f,status=%d,date=%s WHERE type='%d' AND prm=%d AND flat='0' AND date='%s'",value,status,date,type,prm,date);
 else sprintf (query,"INSERT INTO data(flat,prm,type,value,status,date) VALUES('0','%d','%d','%f','%d','%s')",prm,type,value,status,date);
 ULOGW ("[irp] %s",query);
 res=dbase.sqlexec(query); 
 if (res) mysql_free_result(res);
 return true;
}
//---------------------------------------------------------------------------------------------------
BOOL OpenCom (SHORT blok, UINT speed)
{
 CHAR devp[50];
 sprintf (devp,"/dev/ttyS%d",blok);
 if (debug>0) ULOGW("[irp] attempt open com-port %s",devp);
 fd=open(devp, O_RDWR | O_NOCTTY | O_NDELAY);
 if (fd<0) 
    {
     if (debug>0) ULOGW("[irp] error open com-port %s [%s]",devp,strerror(errno)); 
     return FALSE;
    }
 else if (debug>1) ULOGW("[irp] open com-port success"); 
 
 tcflush(fd, TCIOFLUSH); //Clear send & recive buffers
 tcgetattr(fd, &tio);
 cfsetospeed(&tio, baudrate(speed));
 tio.c_cflag |= CREAD|CLOCAL;
 tio.c_cflag &= ~CSIZE;
 tio.c_cflag &= ~CSTOPB;
 tio.c_cflag |=CS8;
 tio.c_cflag &= ~CRTSCTS;
 tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
 tio.c_iflag &= ~(INLCR | IGNCR | ICRNL);
 tio.c_iflag &= ~(IXON | IXOFF | IXANY);
// tio.c_cc[VMIN] = 0;
// tio.c_cc[VTIME] = 1; //Time out in 10e-1 sec
 cfsetispeed(&tio, baudrate(speed));
 fcntl(fd, F_SETFL, /*FNDELAY*/0);
 tcsetattr(fd, TCSANOW, &tio);
 return TRUE;
}

//-----------------------------------------------------------------------------
bool Read(tecon_17 device,		tecon_17_parameter param, BYTE sens_or_tube,
					tecon_17_buff<tecon_17_return> values){

size_t iWrt, iRd, iPrc;

BYTE buff[255] = {0};
BYTE data[255] = {0};
BYTE ctrl, addr, cmd, ds;

// Make data subframe frame
switch(param & T_STMASK){
		case T_COMMON:
			data[0] = (param & 0xff00) >> 8;
			break;
		case T_SENS:
			data[0] |= sens_or_tube;
			/*if((sens_or_tube >= TECON_17_SENSOR_MIN) &&
					(sens_or_tube <=TECON_17_SENSOR_MAX)){
					data[0] |= sens_or_tube;
			}else{
			    Events ((3<<28)&TYPE_INPUTTEKON<<24&SENSOR_INCORRECT, tekon[0].device);
			    if (debug>1) ULOGW ("[tekon] Sensor num [%d] incorrect ", sens_or_tube);
			};*/
			break;
		case T_TUBE:
			data[0] |= sens_or_tube;
			/*if((sens_or_tube >= TECON_17_TUBE_MIN) &&
					(sens_or_tube <=TECON_17_TUBE_MAX)){
					data[0] |= sens_or_tube;
			}else{
			if (debug>1) ULOGW ("[tekon] Tube num [%d] incorrect ", sens_or_tube);
			};*/
			break;
		default:
			if (debug>1) ULOGW ("[tekon] Command subtype [%0x] incorrect ", param & T_STMASK);
			break;
	}

	data[1] = (param & 0xff);
	data[2] = 0x00;

	// Create coorect frame
	iPrc = FT1_2_FrameFL(device.addr, CMD_READ_PARAM, data, 3, buff, 255);

#ifdef WIN32		
	if (!WriteFile(device.port, buff, (DWORD)iPrc, (LPDWORD)&iWrt, NULL)){
		//_ftprintf(stderr, _T("Error writing to port with handle %0x"), 0);
		if (debug>1) ULOGW ("[tekon] Error writing to port with handle [%0x]", device.port);
	};
#endif //WIN32
#ifdef _UNIX_
	write (device.port,buff,iPrc);
#endif//_UNIX_

#ifdef WIN32		
	if (!ReadFile(device.port, buff, sizeof(buff), (LPDWORD)&iRd, NULL)){
		if (debug>1) ULOGW ("[tekon] Error reading from port with handle [%0x]", device.port);
		return 0;
	};
#endif //WIN32
#ifdef _UNIX_
	read (device.port, buff, iRd); 
#endif//_UNIX_
		
	// Parse farme
	FT1_2_Frame(buff,255, ctrl, addr,cmd, data, ds, 255);
	
	if(addr!= device.addr){
		if (debug>1) ULOGW ("[tekon] Asquered device with addr [%d] but answered with addr [%d]", device.addr, addr);
	}

	if(values.ptr == NULL || values.sz == 0){
		if (debug>1) ULOGW ("[tekon] Incorrect buffer with pointer [%p] and size [%d]", values.ptr, values.sz);
		return false;
	}
	
	switch(param & T_TMASK){
		case T_BYTE:
			values.us = ds > values.sz ? values.sz : ds;
			values.type = _BYTE;
			for(size_t i = 0; i < values.us; i++){
				values.ptr[i].b = *((BYTE*)data+i);
			};
			break;

		case T_WORD:
			values.us = (BYTE)(ds/2) > values.sz ? values.sz : ds/2;
			values.type = _WORD;
			for(size_t i = 0; i < values.us; i++){
				values.ptr[i].w = *((WORD*)data+2*i);
			};
			break;

		case T_FLOAT:
			values.us = (BYTE)(ds/4) > values.sz ? values.sz : ds/4;
			values.type = _FLOAT;
			for(size_t i = 0; i < values.us; i++){
				values.ptr[i].f = *((FLOAT*)data+4*i);
			};
			break;

		default:
			values.us = 0;
			if (debug>1) ULOGW ("[tekon] Data type undefined parameter is %x", param);
			return false;
	}
	//---------------------------------- sim
	values.ptr[0].f = 10+5*((DOUBLE)(random())/RAND_MAX);
	//--------------------------------------	
	return true;
};

bool Read(tecon_17 device,
					tecon_17_arhive_type type, BYTE arc_num,
					time_t from, time_t to,
					tecon_17_buff<tecon_17_arhive> values){

	time_t dev_time;

	tecon_17_buff<tecon_17_return> buff;

	if(to <= from){
		if (debug>1) ULOGW ("[tekon] Inverse date diaposon from: [%s] to: [%s]", ctime(&from), ctime(&to));
		return false;
	};

	dev_time = Time(device);
	values.us = 0;

	switch(type){
		case _HOURS:

			while(from < dev_time - 60*60*24*T17_DAYS_DEPTH){
				if(values.us < values.sz){
					values.ptr[values.us].flag = _NO_DATA;
					values.ptr[values.us].val.f = 0;
					values.ptr[values.us].from = from;
					values.ptr[values.us].to = from + 60*60;

					from = values.ptr[values.us++].to;
				}else{
					if (debug>1) ULOGW ("[tekon] Buffer to small");
					return false;
				};
			};
			
			while(from < dev_time){
					switch((dev_time - from)/(60*60*24))
					{
						case 0:
							Read(device, T17_HOURS, arc_num, buff);
							break;
						case 1:
							Read(device, T17_HOURS_1, arc_num, buff);
							break;
						case 2:
							Read(device, T17_HOURS_2, arc_num, buff);
							break;
						case 3:
							Read(device, T17_HOURS_2, arc_num, buff);
							break;
					};

					for(int i = 0; i < buff.us &&  from < to; i++){
						if(values.us < values.sz){
							values.ptr[values.us].flag = _OK;
							values.ptr[values.us].val = buff.ptr[i];
							values.ptr[values.us].from = from;
							values.ptr[values.us].to = from + 60*60;

							from = values.ptr[values.us++].to;
    						}else{
							if (debug>1) ULOGW ("[tekon] Buffer to small");
							return false;
						};
					};

					while(from < to){
						if(values.us < values.sz){
							values.ptr[values.us].flag = _NO_DATA;
							values.ptr[values.us].val.f = 0;
							values.ptr[values.us].from = from;
							values.ptr[values.us].to = from + 60*60;

							from = values.ptr[values.us++].to;
						}else{
							if (debug>1) ULOGW ("[tekon] Buffer to small");
							return false;
						};
					};
			};
			break; // _HOURS
		
		case _DAYS:
			while(from < dev_time - 60*60*24*31*T17_MONTHS_DEPTH){
				if(values.us < values.sz){
					values.ptr[values.us].flag = _NO_DATA;
					values.ptr[values.us].val.f = 0;
					values.ptr[values.us].from = from;
					values.ptr[values.us].to = from + 60*60*24;

					from = values.ptr[values.us++].to;
				}else{
					return false;
				};
			};
			
			if(from < dev_time){
					Read(device, T17_DAYS, arc_num, buff);

					for(int i = 0; i < buff.us &&  from < to; i++){
						if(values.us < values.sz){
							values.ptr[values.us].flag = _OK;
							values.ptr[values.us].val = buff.ptr[i];
							values.ptr[values.us].from = from;
							values.ptr[values.us].to = from + 60*60*24;

							from = values.ptr[values.us++].to;
						}else{
							if (debug>1) ULOGW ("[tekon] Buffer to small");
							return false;
						};
					};
					
					while(from < to){
						if(values.us < values.sz){
							values.ptr[values.us].flag = _NO_DATA;
							values.ptr[values.us].val.f = 0;
							values.ptr[values.us].from = from;
							values.ptr[values.us].to = from + 60*60*24;

							from = values.ptr[values.us++].to;
						}else{
						        if (debug>1) ULOGW ("[tekon] Buffer to small");
							return false;
						};
					};

			};
			break; // _DAYS

		case _MONTHS:
			while(from < dev_time - 60*60*24*31*12*T17_YEARS_DEPTH){
				if(values.us < values.sz){
					values.ptr[values.us].flag = _NO_DATA;
					values.ptr[values.us].val.f = 0;
					values.ptr[values.us].from = from;
					values.ptr[values.us].to = from + 60*60*24*31;

					from = values.ptr[values.us++].to;
				}else{
					if (debug>1) ULOGW ("[tekon] Buffer to small");
					return false;
				};
			};
			
			if(from < dev_time){

				for(BYTE mnth = gmtime(&from)->tm_mon && from < to; mnth < 31; mnth++)
					Read(device, (tecon_17_parameter)(T17_MONTHS |(mnth-1)), arc_num, buff);
					if(values.us < values.sz){
						values.ptr[values.us].flag = _OK;
						values.ptr[values.us].val = *buff.ptr;
						values.ptr[values.us].from = from;
						values.ptr[values.us].to = from + 60*60*24*31;

						from = values.ptr[values.us++].to;
					}else{
					    if (debug>1) ULOGW ("[tekon] Buffer to small");
						return false;
					};

					while(from < to){
						if(values.us < values.sz){
							values.ptr[values.us].flag = _NO_DATA;
							values.ptr[values.us].val.f = 0;
							values.ptr[values.us].from = from;
							values.ptr[values.us].to = from + 60*60*24*31;

							from = values.ptr[values.us++].to;
						}else{
						    if (debug>1) ULOGW ("[tekon] Buffer to small");
							return false;
						};
					};

			};
			break; // _MONTHS


	}
	
};

time_t Time(tecon_17 device){
	tm _time;
	tecon_17_buff<tecon_17_return> buff;

	if(!tecon_17_CreateBuff(&buff, 1)){
		if (debug>1) ULOGW ("[tekon] Buffer [%p] not created", &buff);
	}
	
	if(!Read(device, T17_TIME, 0, buff)) {
		if (debug>1) ULOGW ("[tekon] TIME reading error");
		return 0;
	}

	_time.tm_sec = buff.ptr->b;
	_time.tm_hour= (BYTE)buff.ptr->w>>1;
	
	if(!Read(device, T17_DATE, 0, buff)) {
		if (debug>1) ULOGW ("[tekon] DATE reading error");
		return 0;
	}

	_time.tm_mday = buff.ptr->b;
	_time.tm_mon = (BYTE)buff.ptr->w>>1;

	if(!Read(device, T17_YEAR, 0, buff)) {
		if (debug>1) ULOGW ("[tekon] YEAR reading error");
		return 0;
	};

	_time.tm_year = buff.ptr->w;

	tecon_17_DestroyBuff(&buff);
	return mktime(&_time);
};
