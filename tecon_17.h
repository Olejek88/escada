#ifndef _TECON_17_H_
#define _TECON_17_H_

#ifdef  WIN32
#include <windows.h>
#endif//WIN32

//#include "../../../log/log.h"
#include "cmds.h"
#include "ft1_2.h"

#define T_UNDEFINED		0x00

#define T_BYTE				0x01 << 24
#define T_WORD				0x02 << 24
#define T_FLOAT				0x03 << 24
#define T_TMASK				0x0f << 24

#define T_COMMON			0x10 << 24
#define T_SENS				0x20 << 24
#define T_TUBE				0x30 << 24
#define T_ARC				0x40 << 24
#define T_STMASK			0x70 << 24

#define MAX_CHANNELS	20

void * tekonDeviceThread (void * devr);
class DeviceTekon;

class DeviceTekon : public DeviceD {
public:    
    UINT	idt;
    UINT	device;    
    UINT	n_ar[MAX_CHANNELS];
    UINT	n_sen[MAX_CHANNELS];
    UINT	prm[MAX_CHANNELS];
    UINT	n_pip[MAX_CHANNELS];
    UINT	n_chn[MAX_CHANNELS];
public:    
    // constructor
    DeviceTekon () {};
    ~DeviceTekon () {};
    int ReadDataCurrent (UINT sens_num);
    int ReadDataArchive (UINT sens_num);
};

//Type ID returerd by tecon
enum tecon_17_return_type {
	_UNDEFINED	= T_UNDEFINED,
	_BYTE		= T_BYTE,
	_WORD		= T_WORD,
	_FLOAT		= T_FLOAT
};

//#define tecon_17_return_type T17
//Parameter returned by tecon
union tecon_17_return{
	BYTE	b;
	WORD	w;
	float	f;
};

struct tecon_17{
	void* port;
	BYTE addr;
};

#define T17_DAYS_DEPTH		4
#define T17_MONTHS_DEPTH	1
#define T17_YEARS_DEPTH		1

#define T17_3_DAY_AGO		0x00
#define T17_2_DAY_AGO		0x20
#define T17_1_DAY_AGO		0x40
#define T17_TODAY		0x60

enum tecon_17_arhive_type{
	_HOURS,
	_DAYS,
	_MONTHS
};

enum tecon_17_errors{
	_OK,
	_NO_DATA
};

// One elemnt of the tecon archive
struct tecon_17_arhive{
	time_t		from;
	time_t		to;
	tecon_17_return	val;
	tecon_17_errors	flag;
};

// Buffer of tecon parameters
template < class T>
struct tecon_17_buff{
	tecon_17_return_type type;
	
	size_t us;
	size_t sz;
	T* ptr;
};

template < class T>
bool tecon_17_CreateBuff(tecon_17_buff<T>* buff, size_t sz);

template < class T>
bool tecon_17_DestroyBuff(tecon_17_buff<T>* buff);

// Sensor parameters
#define TECON_17_SENSOR_MIN	0x00
#define TECON_17_SENSOR_MAX	0x3F
//#define TECON_17_SENSOR_FLAG	0x00
// Tube parameters
#define TECON_17_TUBE_MIN	0x00
#define TECON_17_TUBE_MAX	0x0F
//#define TECON_17_TUBE_MASK	0x80

enum tecon_17_parameter {

	// Sensor parameters
	T17_SENS_MOMENTARY	= 0x000f | T_FLOAT | T_SENS,
	T17_SENS_AVERAGE_5_MIN	= 0x0014 | T_FLOAT | T_SENS,

	// Tube parameters
	T17_TUBE_TYPE			= 0x8000 | T_WORD  | T_TUBE,
	T17_TUBE_AVERAGE_5_MIN		= 0x8017 | T_FLOAT | T_TUBE,
	T17_TUBE_HEAT_POWER_5_MIN	= 0x802b | T_FLOAT | T_TUBE,

	// Archive
	T17_HOURS			= 0xc000 | T_FLOAT | T17_TODAY	   | T_ARC,
	T17_HOURS_1			= 0xc000 | T_FLOAT | T17_2_DAY_AGO | T_ARC,
	T17_HOURS_2			= 0xc000 | T_FLOAT | T17_2_DAY_AGO | T_ARC,
	T17_HOURS_3			= 0xc000 | T_FLOAT | T17_3_DAY_AGO | T_ARC,

	T17_DAYS			= 0xc0c0 | T_FLOAT | T_ARC,
	T17_MONTHS			= 0xc080 | T_FLOAT | T_ARC,

	// Plant settings
	T17_SERIAL= 0x403c | T_WORD,		//Serial number of device
	
	// Common
	T17_TIME= 0x4015 | T_WORD,
	T17_DATE= 0x4016 | T_WORD,
	T17_YEAR= 0x4017 | T_WORD,
};

//Read parameter from tecon
bool Read(tecon_17 device,		tecon_17_parameter param, BYTE sens_or_tube,
					tecon_17_buff<tecon_17_return> values);

//Read archive from tecon
bool Read(tecon_17 device,
			tecon_17_arhive_type type, BYTE arc_num,
			time_t from, time_t to,
			tecon_17_buff<tecon_17_arhive> values);

// Get current time of device
time_t Time(tecon_17 device);

#endif// _TECON_17_H_
