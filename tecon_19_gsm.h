#include "cmds.h"

#define T_UNDEFINED			0x00

#define T_BYTE				0x01 << 24
#define T_WORD				0x02 << 24
#define T_FLOAT				0x03 << 24
#define T_TMASK				0x0f << 24

#define T_COMMON			0x10 << 24
#define T_SENS				0x20 << 24
#define T_TUBE				0x30 << 24
#define T_ARC				0x40 << 24
#define T_STMASK			0x70 << 24

#define MAX_CHANNELS			50
#define MAX_CLIENTS			50
#define UDP_PORT			"51952"

void * tekonGSMDeviceThread (void * devr);

class DeviceTekonGSM;

class DeviceTekonGSM : public DeviceD {
public:    
    UINT	idt;
    UINT	device;    		// device id
    UINT	pipe[MAX_CHANNELS];	// identificator
    UINT	cur[MAX_CHANNELS];	// nparametr for read values
    UINT	prm[MAX_CHANNELS];	// prm identificator    
    UINT	n_hour[MAX_CHANNELS];	// nparametr for read hour values archive
    UINT	n_day[MAX_CHANNELS];	// nparametr for read day values archive
    UINT	n_month[MAX_CHANNELS];	// nparametr for read month values archive
public:    
    // constructor
    DeviceTekonGSM () {};
    ~DeviceTekonGSM () {};
    int ReadDataCurrent (UINT sens_num);
    int ReadDataArchive (UINT sens_num);
    int ReadAllArchive (UINT  sens_num, UINT tp);
    BOOL send_tekon (UINT op, UINT prm, UINT frame, UINT index);
    UINT read_tekon (BYTE* dat);    
};
