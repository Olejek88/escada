#define CURRENT_ARCHIVE		9
#define HOUR_ARCHIVE		26
#define DAY_ARCHIVE		27
#define MONTH_ARCHIVE		28
#define INT_ARCHIVE		25

#define ADR_NUMBER		834
#define ADR_NUMBER_LEN		4

#define ADR_ARCHTYPE		3
#define ADR_YEAR		0
#define ADR_DAY			1
#define ADR_STATUS		6
#define ADR_TIME		0
#define ADR_DATA		256

#define ReadHoldingRegisters	0x3
#define ReadInputRegisters	0x4
#define PresetSingleRegister  	0x6
#define PresetMultiRegister  	0x6

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
    bool ReadTime ();    
    int ReadData (UINT	type, UINT deep);
    
//    bool send_elf (UINT op, UINT reg, UINT nreg, UCHAR* dat);
//    int read_elf (BYTE* dat);
};

