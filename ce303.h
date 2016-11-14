//-----------------------------------------------------------------------------
#define START		0x2f
#define STOP		0x21
#define CR		0xD
#define LF		0xA
#define ACK		0x6
#define SOH		0x1
#define STX		0x2
#define ETX		0x3
#define EOT		0x4
#define REQUEST		0x3F

#define	SN		0x5002


#define	CURRENT_U	0x4001
#define	CURRENT_W	0x4003
#define	CURRENT_F	0x400D
#define	CURRENT_I	0x400A

#define	ARCH_MONTH	0x1201
#define	ARCH_DAYS	0x1401
#define	TOTAL_MONTH	0x1101
#define	TOTAL_DAYS	0x1301

#define	READ_DATE	0x10
#define	READ_TIME	0x11

#define	READ_PARAMETERS	0x22
#define	READ_ARCHIVES	0x33
#define	OPEN_CHANNEL_CE	0x50
#define	OPEN_PREV	0x51

void * cemDeviceThread (void * devr);

class DeviceCEM;

class DeviceCEM : public DeviceD {
public:    
    UINT	idce;
    CHAR	pass[10];
    UINT	adrr[8];

    // config    
    DeviceCEM () {}
    // store it to class members and form sequence to send in device
    BYTE* ReadConfig ();    
    // 
    int ReadInfo ();
    // source data will take from db or class member fields
    bool  send_ce (UINT op);
    // function read data from 2IP
    // reading data will be store to DB and temp data class member fields
    int  read_ce (BYTE* dat);
    // ReadDataCurrent - read single device connected to concentrator
    int ReadDataCurrent ();
    int ReadAllArchive  (UINT tp);
    BOOL send_ce (UINT op, UINT prm, CHAR* request, UINT frame);
    UINT read_ce (BYTE* dat, BYTE type);
};
