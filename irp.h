#define	READ_REGISTRY		0x05
#define	READ_DATA		0x06
#define	WRITE_REGISTRY		0x07

#define	SV_NUMBER		0x00
#define	DEVICE_TYPE		0x01
#define	TIME_PRODUCT		0x02
#define	SERIAL_NUMBER		0x04
#define	SPEED			0x06
#define	ADR			0x07
#define	STRUT			0x09
#define	STYPE			0x0D
#define	TCP1			0x0E
#define	TCP2			0x0F
#define	TCP_P1			0x10
#define	TCP_P2			0x14
#define	IMP1			0x18
#define	IMP2			0x19
#define	PABS1			0x1A
#define	PABS2			0x1E
#define	TSOUR			0x22
#define	PATM			0x26
#define	QIMP			0x2B
#define	DHOUR			0x2C
#define	NHOUR			0x2D
#define	ERR			0x4B
#define	CURRENT_TIME		0x4C

#define HOUR_ADR		0x60
#define DAY_ADR			0x19E0
#define MONTH_ADR		0x30C0
#define EVENT_ADR		0x31CE
#define TECH_ADR		0x3B2E

void * irpDeviceThread (void * devr);
class DeviceIRP;

class DeviceIRP : public DeviceD {
public:    
    UINT	idirp;
    UINT	adr;
    UINT	strut;
    UINT	stype;
    UINT	tcp1;
    UINT	tcp2;
    FLOAT	tcp_p1;
    FLOAT	tcp_p2;
    UINT	imp1;
    UINT	imp2;
    FLOAT	pabs1;
    FLOAT	pabs2;
    FLOAT	tsour;
    FLOAT	patm;
    UINT	qimp;
    UINT	dhour;
    UINT	nhour;
public:    
    // constructor
    DeviceIRP () {};
    ~DeviceIRP () {};
    int ReadLastArchive (UINT	gl);
    int ReadDataCurrent ();
    
    bool send_irp (UINT op, UINT reg, UINT nreg, UCHAR* dat);
    int read_irp (BYTE* dat);
};
