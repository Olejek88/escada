//#define CURRENT_ARCHIVE		0
//#define HOUR_ARCHIVE		0
//#define DAY_ARCHIVE		1
//#define MONTH_ARCHIVE		2

#define ADR_NUMBER		834
#define ADR_NUMBER_LEN		4

#define ADR_ARCHTYPE		3
#define ADR_YEAR		0
#define ADR_DAY			1
#define ADR_STATUS		6
#define ADR_TIME		32784
#define INPUT_DATA		49152
#define INPUT_DATA2		32780
//#define INPUT_DATA		0

#define ReadHoldingRegisters	0x3
#define ReadInputRegisters	0x4
#define PresetSingleRegister  	0x6
#define PresetMultiRegister  	0x6
#define ReadRecordsByTime  	65

void * tsrvDeviceThread (void * devr);
class DeviceTSRV;

class DeviceTSRV : public DeviceD {
public:    
    UINT	id;
    UINT	adr;
    CHAR	version[10];
    CHAR	software[5];
    tm 		*devtime;
public:    
    // constructor
    DeviceTSRV () {};
    ~DeviceTSRV () {};
    bool ReadVersion ();
    bool ReadTime ();    
    int  ReadCurrent ();
    int  ReadData (UINT	type, UINT deep, UINT secd);
};

