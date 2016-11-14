#define	CUR_REQUEST	44
#define	NSTR_REQUEST	52
#define	STR_REQUEST	65
#define MAX_CHANNELS	50
#define	ZERO_REQUEST	0

#define	VERSION_REQUEST	9

void * kmDeviceThread (void * devr);

class DeviceKM5;

class DeviceKM5 : public DeviceD {
public:    
    UINT	idt;
    //uint16_t	device;    		// device id
    UINT	pipe[MAX_CHANNELS];	// identificator
    UINT	cur[MAX_CHANNELS];	// nparametr for read values
    UINT	prm[MAX_CHANNELS];	// prm identificator    
    UINT	n_hour[MAX_CHANNELS];	// sm
    UINT	n_day[MAX_CHANNELS];	// sm
    UINT	addr[MAX_CHANNELS];	// sm
public:    
    // constructor
    DeviceKM5 () {};
    ~DeviceKM5 () {};
    int ReadInfo ();
    int ReadDataCurrent (UINT sens_num);
    int ReadAllArchive (UINT  sens_num, UINT tp, UINT secd);
    BOOL send_km (UINT op, UINT prm, UINT frame, UINT type, UINT index);
    UINT read_km (BYTE* dat, BYTE type);    
};
