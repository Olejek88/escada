#define	VERSION		0x100
#define	SN		0x10A
#define	TIME		0x120
#define SUM_W		0x131
#define	SUM_W1		0x130
#define	SUM_W2		0x130
#define	CURRENT_W	0x12E
#define	HOUR_W		0x134
#define	NAK_W		0x12F
#define	NAK_W1		0x133
#define	NAK_W2		0x133

void * ceDeviceThread (void * devr);

class DeviceCE;

class DeviceCE : public DeviceD {
public:    
    UINT	idce;

    // config    
    DeviceCE () {}
    // store it to class members and form sequence to send in device
    BYTE* ReadConfig ();    
    // source data will take from db or class member fields
    bool  send_ce (UINT op);
    // function read data from 2IP
    // reading data will be store to DB and temp data class member fields
    int  read_ce (BYTE* dat);
    // ReadDataCurrent - read single device connected to concentrator
    int ReadDataCurrent ();
    int ReadAllArchive  (UINT tp);
    BOOL send_ce (UINT op, UINT prm, UINT frame, UINT index);
    UINT read_ce (BYTE* dat, BYTE type);
};
