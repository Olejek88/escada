#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include <pthread.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <iostream>
#include <termios.h>
#include <math.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <ctype.h>
//#include <linux/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct DeviceR {
  UINT	id;		//
  UINT	idd;		//
  SHORT SV;		// 
  SHORT interface;	//   
  SHORT	protocol;	// 
  UINT  port;		//
  UINT	speed;		//
  UINT	adr;  		//
  SHORT type;		//
  CHAR  number[30];	//
  UINT	flat;		//
  SHORT	akt;		//
  CHAR  lastdate[15];	//
  UINT  qatt;		//   
  UINT	qerrors;	//
  SHORT	conn;		//
  CHAR  devtim[25];	//
  SHORT	chng;		//
  SHORT	req;		//
  SHORT	source;  	//
  CHAR	name[50];	//   
  SHORT	ust;
  float	rasknt;
  float	pottrt;
  float	potlep;
  float knt;
};


#define TYPE_NOTDEFINED		0x0	// not defined type
#define TYPE_BIT		0x1	// BIT type
#define TYPE_2IP		0x2	// 2IP type
#define TYPE_KM			0x3	// Flat view type
#define TYPE_MEE		0x4	// MEE type
#define TYPE_IRP		0x5	// IRP type
#define TYPE_LK			0x6	// LK type
#define TYPE_DK			0x7	// DK type
#define TYPE_SERVER		0x8	// Server type
#define TYPE_INPUTTSRV		0xA	// operator panel type
#define TYPE_INPUTTEKON		0xB	// Tekon type
#define TYPE_INPUTLOGIKA	0xC	// Logika type
#define TYPE_INPUTKM		0xD	// KM type
#define TYPE_INPUTCE		0xE	// CE102 type
#define TYPE_INPUTVKT		0xF	// CE102 type
#define TYPE_SPG741		0x15	// SPG741 type
#define TYPE_HART		0x14	// Hart type
#define TYPE_MERCURY230		0x10	// Mercury 230 type
#define TYPE_INPUTELF		0x16	// Elf type
#define TYPE_SET_4TM		0x11	// Set 4TM type
#define TYPE_VIST		0x12	// VIS-T type
//#define TYPE_CE102		0x13	// CE102 type
#define TYPE_CE303		0x17	// CE303 type
#define TYPE_SPG742		0x18	// SPG742 type

#define MAX_DEVICE		200	// max device quantity
#define MAX_DEVICE_NSP		5	// max non-specific device
#define MAX_DEVICE_LK		20	// max lk quantity
#define MAX_DEVICE_BIT		150	// max bit quantity
#define MAX_DEVICE_2IP		20	// max 2ip quantity
#define MAX_DEVICE_IRP		20	// max irp quantity
#define MAX_DEVICE_MEE		5	// max mee quantity
#define MAX_DEVICE_TEKON	15	// max tekon quantity
#define MAX_DEVICE_KM		9	// max KM
#define MAX_DEVICE_CE		10	// max CE
#define MAX_DEVICE_VKT		5	// 
#define MAX_DEVICE_ELF		5	// 
#define MAX_DEVICE_742		10	// max spg quantity
#define MAX_FLATS		100	// max flats quantity
#define MAX_STRUTS		20	// max struts quantity
#define MAX_LEVEL		10	// max levels quantity
#define	MAX_DEVICE_M230		80

#define	TYPE_CURRENTS		0
#define	TYPE_HOURS		1
#define	TYPE_DAYS		2
#define	TYPE_MONTH		4
#define	TYPE_INCREMENTS		7
#define	TYPE_EVENTS		9

class DeviceD
{
public:
    UINT	iddev;			// rec id
    UINT	device;			// device identificator
    short	SV;
    short	interface;
    short	protocol;
    short	port;
    UINT	speed;
    UINT	adr;
    short	type;
    char	number[30];
    UINT	flat;
    short	akt;
    char	lastdate[15];
    UINT	qatt;
    UINT	qerrors;
    short	conn;
    char	devtim[15];
    short	chng;
    short	req;
    short	source;
    char 	name[50];
    short	ust;

    float	rasknt;
    float	pottrt;
    float	potlep;
    float	knt;
//    DeviceD (DeviceR &devices)    {     UINT id=devices.id;    };
    DeviceD () {};
    ~DeviceD () {};
    UINT GetDevice ()
	{
	 return iddev;
	};
    //virtual BYTE* ReadConfig ();
    //virtual int	SendConfig ();
    //int RecieveConfig ();
    //int ReadDataCurrent ();
    //int ReadDataArch ();
    //int RWCommand ();	
};

union fnm
{
    float 	f;
    char	c[4];
};

class DeviceDK : public DeviceD {
public:
    UINT	iddk;
    UINT	adr;
    CHAR	ip[30];
    UINT	build;
    short	regim;
    short	qzapr;
    UINT	deep;
    UINT	tmdt;
    UINT	intlk;
    UINT	inttek;
    UINT	interp;
    CHAR	last_date[20];
    BYTE	chng;	

    CHAR	address[50];
    BYTE	log;
    FLOAT	knt_ent;
    FLOAT	knt_wat;
    BYTE	emul;
    BYTE	irp_approx;
    UINT	irp_addr_1;
    UINT	irp_addr_2;
    UINT	irp_addr_3;
    UINT	irp_addr_4;
    UINT	irp_addr_5;
    UINT	nstrut;
    UINT	nlevels;
    UINT	nentr;
    UINT	nflats;
    FLOAT	maxent;
    FLOAT	maxtemp;
    FLOAT	datamin;
    FLOAT	datamax;    
    BYTE	main_ele;
    BYTE	main_heat;
    BYTE	main_wat;
    BYTE	heat;
    BYTE	formxml;
    BYTE	crq_enabl;
    UINT	maxcrqlen;
    UINT	crqport;

    BYTE	pth_ce;
    BYTE	pth_irp;
    BYTE	pth_km5;
    BYTE	pth_lk;
    BYTE	pth_tek;
    BYTE	pth_spg742;
    BYTE	pth[40];
    // config    
    DeviceDK () {}
    // function read DK config from DB
    // store it to class members and form sequence for send to server
    BYTE* ReadConfig ();    
};

class Flats {
public:
    UINT	id;
    UINT	flatd;
    UINT	level;
    UINT	rooms;
    UINT	nstrut;
    CHAR	name[100];

    CHAR	lasthour[18];
    CHAR	lastday[18];

    FLOAT	square;
    UINT	ent;
    Flats () {}
};

struct _AnsLog {
  uint8_t  checksym;	// checksum status (true - ok, false - bad)
  uint8_t  from;	// source address (SPG)
  uint8_t  to;		// reciever address (controller)
  uint8_t  func;	// answer function
  char 	head[100];	// answer header
  uint8_t  pipe;	// channels or pipe number
  uint8_t  nadr;	// parametr or array number
  uint8_t  crc;		// checksum

  uint8_t  from_param;	// [index] from which parametr read
  uint8_t  quant_param;	// [index] parametrs quant

  char  data [20][20][120];	// [для всех запросов на чтение] значения параметров
  char  time [20][20][30];	// [для всех запросов на чтение] метка времени
};
typedef struct _AnsLog AnsLog;

typedef struct _Archive arch;

struct _Archive {
    uint8_t	id;		// номер порядковый
    uint16_t	no;		// номер параметра 
    uint16_t	adr;		// адрес в пространстве Логики 
    char	name[250];	// текстовое название параметра
    uint16_t	pipe;		// коэффициент пересчета величины
    uint16_t	type;		// тип элемента (0-часовой, 1-суточный, 2-декадный, 3-по месяцам, 4-сменный)
    char	meas[15];	// текстовое название еденицы измерения
};

struct _NScode {
    uint8_t	id;		// номер порядковый
    char	shrt[20];	// текстовое название параметра - короткое
    char	name[200];	// текстовое название параметра
};


#define	 bright		"\e[1;37m"
#define	 kernel_color	"\e[0;31m"
#define	 success_color	"\e[0;32m"
#define	 module_color	"\e[0;34m"
#define	 sql_color	"\e[0;35m"
#define	 nc		"\e[0m"
