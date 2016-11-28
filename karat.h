#define CURRENT_ARCHIVE		0
#define HOUR_ARCHIVE		1
#define DAY_ARCHIVE		2
#define MONTH_ARCHIVE		4
#define INT_ARCHIVE		7

#define ADR_NUMBER		257
#define ADR_NUMBER_LEN		4

#define ADR_ARCHTYPE		3
#define ADR_HOUR		0x0
#define ADR_DAY			0x10
#define ADR_MONTH		0x20
#define ADR_INT			0x30
#define ADR_STATUS		6
#define ADR_TIME		0
#define ADR_DATA_CURRENT	0x2000
#define REQUEST_DATA		0x60

#define ReadHoldingRegisters	0x3
#define ReadInputRegisters	0x4
#define PresetSingleRegister  	0x6
#define PresetMultiRegister  	0x6

void * karatDeviceThread (void * devr);
class DeviceKarat;

class DeviceKarat : public DeviceD {
public:
    UINT	idkarat;
    UINT	adr;
    CHAR	version[10];
    CHAR	software[5];
    tm 		*devtime;
public:
    // constructor
    DeviceKarat () {};
    ~DeviceKarat () {};
    bool ReadVersion ();
    bool ReadTime ();
    int ReadData (UINT	type, UINT deep);
};
