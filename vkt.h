#define OPC_QUALITY_GOOD 0xC0
#define OPC_QUALITY_BAD 0x00
#define OPC_QUALITY_CONFIG_ERROR 0x04
#define OPC_QUALITY_DEVICE_FAILURE 0x0C
#define OPC_QUALITY_UNCERTAIN 0x40
#define OPC_QUALITY_SENSOR_CAL 0x50

#ifndef TIME_ADR
#define TIME_ADR		0x3FFB
#endif

#define ID			0x3EA6
#define ELEMENT_LIST		0x3FFF
#define ELEMENT_TYPE		0x3FFD
#define ELEMENT_DATE		0x3FFB
#define DATA			0x3FFE

#define HOUR_ARCHIVE		0
#define DAY_ARCHIVE		1
#define MONTH_ARCHIVE		2
#define TOTAL_ARCHIVE		3
#define TOTAL_VALUES		4
#define TOTALIZER		5

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

void * vktDeviceThread (void * devr);
class DeviceVKT;

class DeviceVKT : public DeviceD {
public:    
    UINT	idvkt;
    UINT	adr;
    CHAR	version[10];
    CHAR	software[5];
    tm 		*devtime;
public:    
    // constructor
    DeviceVKT () {};
    ~DeviceVKT () {};
    bool ReadVersion ();
    bool ReadTime ();    
    int  ReadData (UINT	type, UINT deep);
    bool send_vkt (UINT op, UINT reg, UINT nreg, UCHAR* dat);
    int  read_vkt (BYTE* dat);

};

