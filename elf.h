#define FUNC_VERSION		0x12
#define FUNC_TIME		0xA
#define FUNC_DATA		0x2

#define	READ_VERSION		0x10
#define	READ_TIME		0x11
#define	READ_CURRENTS		0x0
#define	READ_HOURS		0x1
#define	READ_DAYS		0x2

#define	VERSION_ELF		0x03454F00
#define	CURR_TIME		0x03150900

void * elfDeviceThread (void * devr);
class DeviceELF;

class DeviceELF : public DeviceD {
public:    
    UINT	idelf;
    UINT	adr;
    CHAR	version[10];
    CHAR	software[5];
    tm 		*devtime;
public:    
    // constructor
    DeviceELF () {};
    ~DeviceELF () {};
    bool ReadVersion ();
    //bool ReadTime ();    
    int ReadData (UINT	type);
    
    bool send_elf (UINT op, UINT reg, UINT nreg, UCHAR* dat);
    int read_elf (BYTE* dat);
};

