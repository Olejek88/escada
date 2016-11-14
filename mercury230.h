//-----------------------------------------------------------------------------
#define	CHECK		0x0
#define	OPEN_CHANNEL	0x1
#define	CLOSE_CHANNEL	0x2
#define	WRITE_DATA	0x3
#define	READ_TIME_230	0x4
#define	READ_EVENTS	0x4
#define	READ_DATA_230	0x5
#define	READ_POWER	0x6
#define	READ_PARAMETRS	0x8
#define	READ_DATA_230L	0xA
#define	READ_UI		0x11

#define ENERGY_SUM	0x8	// S
#define I1		0x21
#define I2		0x22
#define I3		0x23
#define U1		0x11
#define U2		0x12
#define U3		0x13
#define F		0x40
#define P_SUM		0x00
#define Q_SUM		0x04
#define S_SUM		0x08

void * merDeviceThread (void * devr);

class DeviceMER;

class DeviceMER : public DeviceD {
public:    
    UINT	idmer;
    UINT	adr_dest;
    UINT	adrr[8];
    CHAR	pass[10];

    FLOAT	Kn;
    FLOAT	Kt;
    FLOAT	A;

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
    BOOL send_mercury_htc (UINT op, UINT prm, UINT frame, UINT index);
    UINT read_mercury_htc (BYTE* dat, BYTE type);

    BOOL send_ce (UINT op, UINT prm, CHAR* request, UINT frame);
    UINT read_ce (BYTE* dat, BYTE type);
};
