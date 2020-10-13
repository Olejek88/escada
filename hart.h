#define DEVICE_NUM_MAX 	1
#define MODULE_NUM_MAX 	4
#define CHANNAL_NUM_MAX 8
#define TAGS_NUM_MAX 	50
//--------------------------------------------------------------------------------------
void * hReadDevice (void * devr);
BOOL hWrite (CHAR* command,CHAR* data);	
INT  hScanBus (SHORT adr, UINT dev, UINT speed);
//-------------------------------------------------------------------------------------
class DeviceHART;

class DeviceHART : public DeviceD {
public:    
    UINT	idhart;
    UINT	adr;
public:    
    // constructor
    DeviceHART () {};
    ~DeviceHART () {};
};


