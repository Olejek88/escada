#define	SET4_CHECK		0x0
#define	SET4_OPEN_CHANNEL	0x1
#define	SET4_CLOSE_CHANNEL	0x2
#define	SET4_WRITE_DATA		0x3
#define	SET4_READ_TIME		0x4
#define	SET4_READ_EVENTS		0x4
#define	SET4_READ_DATA		0x5
#define	SET4_READ_PARAMETRS	0x8
#define	SET4_READ_UI		0x11

#define ENERGY_SUM	0xF0
#define I1		0x21
#define I2		0x22
#define I3		0x23
#define U1		0x11
#define U2		0x12
#define U3		0x13
#define P_SUM		0x00
#define Q_SUM		0x04
#define S_SUM		0x08

void * merDeviceThread (void * devr);

class DeviceMER;

class DeviceMER : public DeviceD {
public:    
    UINT	idmer;

    // config    
    DeviceMER () {}
    // store it to class members and form sequence to send in device
    BYTE* ReadConfig ();    
    // source data will take from db or class member fields
    bool  send_mercury (UINT op);
    // function read data from 2IP
    // reading data will be store to DB and temp data class member fields
    int  read_mercury (BYTE* dat);
    int ReadInfo ();
    // ReadDataCurrent - read single device connected to concentrator
    int ReadDataCurrent ();
    int ReadAllArchive  (UINT tp);
    BOOL send_mercury (UINT op, UINT prm, UINT frame, UINT index);
    UINT read_mercury (BYTE* dat, BYTE type);
};
