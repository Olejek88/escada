//------------------------------------------------------------------------------
pthread_t thr,thr2,thrd;

static	db	dbase;
static	 MYSQL_RES *res;
static	 MYSQL_ROW row;
//static	 CHAR   	query[500];

static	MYSQL_RES *res_;
static	MYSQL_RES *res2;
static	MYSQL_RES *res3;
static 	MYSQL_ROW row_;
static 	MYSQL_ROW row2;
static 	MYSQL_ROW row3;
static	CHAR querys[2500];

UINT 	ret;
UINT 	devnum=0,channum=0,rashnum=0,thrdnum=0;
CHAR	kernellog[300];
CHAR	version2[50];
timeval disptm;

struct 	utsname ubuf;
//glibtop_cpu cpu1;
//glibtop_cpu cpu2;

struct 	tm *currenttime;		// current system time

BOOL LoadDKConfig();
BOOL isConfigurated=false;
BOOL isInitialised=false;
BOOL WorkRegim=false;

DeviceDK	dk;
DeviceR 	dev[MAX_DEVICE];
DeviceLK 	lk[MAX_DEVICE_LK];
DeviceBIT 	bit[MAX_DEVICE_BIT];
Device2IP 	ip2[MAX_DEVICE_2IP];
DeviceIRP 	irp[MAX_DEVICE_IRP];
DeviceMEE 	mee[MAX_DEVICE_MEE];
DeviceTekon 	tekon[MAX_DEVICE_TEKON];
DeviceKM5  	km[MAX_DEVICE_KM];
DeviceCE  	ce[MAX_DEVICE_CE];
DeviceCEM  	cem[MAX_DEVICE_CE];
DeviceVKT  	vkt[MAX_DEVICE_VKT];
DeviceELF  	elf[MAX_DEVICE_NSP];
//DeviceHART  	hart[MAX_DEVICE_HART];
DeviceMER  	mer[MAX_DEVICE_M230];
DeviceTSRV  	tsrv[MAX_DEVICE_NSP];
//DeviceSET  	set[MAX_DEVICE_M230];
DeviceVIST  	vist[MAX_DEVICE_NSP];
DeviceSPG741  	spg741[MAX_DEVICE_NSP];
DeviceSPG742  	spg742[MAX_DEVICE_NSP];
//DeviceSPG761	*spg761;

Flats		flat[MAX_FLATS];
Fields		field[MAX_DEVICE_BIT];
Datas		data[10000];

UINT		device_num=0;
uint16_t	flat_num=0;
uint16_t	dev_num[30]={0};
UINT		lk_num=0;
UINT		bit_num=0;
UINT		irp_num=0;
UINT		ip2_num=0;
UINT		mee_num=0;
UINT		tekon_num=0;
uint16_t	field_num=0;
uint16_t	data_num=0;
UINT		km_num=0;
UINT		ce_num=0;
UINT		cem_num=0;
UINT		vkt_num=0;
UINT		elf_num=0;
UINT		hart_num=0;
UINT		mer_num=0;
UINT		tsrv_num=0;
UINT		spg741_num=0;
UINT		spg742_num=0;
//UINT		set_num=0;

UINT 	debug=4;
UINT 	restart=0;

BOOL	threads[30];
BOOL	crq_thread=false;
BOOL	lk_thread=false;
BOOL	ce_thread=false;
BOOL	cem_thread=false;
BOOL	tek_thread=false;
BOOL	km5_thread=false;
BOOL	irp_thread=false;
BOOL	vkt_thread=false;
BOOL	srv_thread=false;
BOOL	elf_thread=false;
BOOL	panel_thread=false;
BOOL	hart_thread=false;
BOOL	mer_thread=false;
//------------------------------------------------------------------------------
static void disp_tick (int signo);
int initkernel (void);
void * dispatcher (void * thread_arg);
extern 	void * lkDeviceThread (void * devr);
extern 	void * oDeviceThread (void * devr);
extern	void * irpDeviceThread (void * devr);
extern	void * ceDeviceThread (void * devr);
extern	void * cemDeviceThread (void * devr);
extern	void * tekonGSMDeviceThread (void * devr);
extern	void * vktDeviceThread (void * devr);
extern	void * vistDeviceThread (void * devr);
extern	void * spg761DeviceThread (void * devr);

//extern	bool	CalculateFlats	(UINT	type,  CHAR* date);
void StartThreads (void);
//------------------------------------------------------------------------------
