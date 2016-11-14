#define	TEST			0x01
#define	LOAD_TO_RAM		0x02
#define	START_INITIALIZE	0x03
#define	READ_INITIALIZE_STATUS	0x04

#define READ_STRUCTURE			0x5
#define WRITE_STRUCTURE_BY_FACTORY 	0x6
#define READ_STRUCTURE_BY_FACTORY	0x7
#define	SYNC_RTC		0x15

#define	SET_UART_BAUD_RATE	0x0b
#define	READ_FROM_RAM		0x0c
#define	START_DEINITIALIZE	0x11
#define	READ_DEINITIALIZE_STATUS	0x12
#define	READ_IDENTIFICATION	0x13
#define	GET_NUM_VISIBLE		0x14
#define	READ_VISIBILITY_STAT	0x15
#define	CLEAR_INIT_DEINIT	0x18
#define	UPDATE_RTC		0x19
#define	GET_FLAT_INFO_COUNT 	0x21
#define SET_FLAT_CONFIG	 	0x1C

enum ErrorCode
{
 ERROR_LK_FRAME_NO_ERROR		= 0x00,	// Нет ошибки
 ERROR_NO_ERROR				= 0x00,	// Нет ошибки
 ERROR_NUM_EXCEED,				// Номер превышен
 ERROR_WRITE_VIOLATION,				// Ошибка при записи
 ERROR_READ_VIOLATION,				// Ошибка при чтении
 ERROR_NULL_PTR,				// Нулевой указатель
 ERROR_SUBSCRIPT_OUT_OF_RANGE,			// Выход за границы массива
 ERROR_UNDEFINED_FACTORY,			// Неопределенный уникальный идентификатор
 ERROR_MODULE_ALREADY_LINKED,			// Устройство с таким уникальным идентификатором уже связано с ЛК
 ERROR_MODULE_NOT_BINDED,			// Устройство с таким уникальным идентификатором не связано с ЛК
 ERROR_AREA_IS_FULL,				// Область заполнена. Отсутствует возможность добавления нового элемента
 ERROR_UNSUPPORTED,				// Не поддерживается
 ERROR_HANDLER,					// Ошибка обработчика команды
 ERROR_RETRY_NEEDED,				// Необходим повтор команды
 ERROR_FS,					// Ошибка, связанная с файловой системой
 ERROR_FS_END = ERROR_FS + 16,			// Последняя ошибка, связанная с файловой системой
 ERROR_OUT_OF_MEMORY,				// Закончилась память
 ERROR_INCORRECT_PARAMETER,			// Входные параметры не поддерживаются
 ERROR_READ,					// Ошибка при чтении
 ERROR_WRITE,					// Ошибка при записи
 ERROR_UNKNOWN,					// Неизвестная ошибка
 ERROR_ITEM_NOT_FOUND,				// Элемент не найден
 ERROR_END_OF_ARCHIVE,				// Достигнут конец архива
 ERROR_ITEM_ALREADY_EXIST,			// Элемент уже существует
 ERROR_OAFFS,					// Ошибка, связанная с OAFFS
 ERROR_OAFFS_END = ERROR_OAFFS + 32,		// Последняя ошибка, связанная с OAFFS
}; 

enum NumOp
{
 OPERATION_NOP						= 0x00,
 OPERATION_LK_TEST					= 0x01,
 OPERATION_LK_CHECK_SYSTEM				= 0x02,
 OPERATION_WRITE_STRUCTURE				= 0x04, // запись структуры
 OPERATION_READ_STRUCTURE				= 0x05, // чтение структуры
 OPERATION_WRITE_STRUCTURE_BY_FACTORY			= 0x06, // запись структуры устройства по его Factory
 OPERATION_READ_STRUCTURE_BY_FACTORY			= 0x07, // чтение структуры устройства по его Factory
 OPERATION_LK_LINK_FACTORY				= 0x08, // связывание устройства по его Factory с ЛК
 OPERATION_LK_UNLINK_FACTORY				= 0x09, // отвязывание устройства по его Factory от ЛК
 OPERATION_LK_GET_LINKED_FACTORY			= 0x0A, // получение списка связанных с ЛК устройств
 OPERATION_LK_READ_LAST_FACTORY_WHO_ASKED_CONFIG	= 0x0B, // идентификация устройства, последним запросившего конфигурацию
 OPERATION_LK_SYNC_RTC					= 0x15,
};						

#define Factory_Structure	 		1
#define Features_Structure 			2
#define Net_Frame 				3
#define RF_Frame 				4
#define LK_Config_Structure 			5
#define LK_Data_Structure 			6
#define Flat_Config_Structure 			7
#define Flat_Imitation_Structure 		8
#define Flat_Raw_Data_Structure 		9
#define Flat_Month_Consumption	 		10
#define Flat_Interval_Consumption 		11
#define Strut_Params_Structure	 		12
#define Event 					13
#define Module_Config 				14
#define Module_DataTo 				15
#define Module_DataFrom 			16
#define Module_Structure 			17
#define BIT_Config_Structure 			18
#define BIT_DataFrom_Structure	 		19
#define BIT_DataTo_Structure 			20
#define I2CH_Config_Structure 			21
#define I2CH_DataFrom_Structure 		22
#define I2CH_DataTo_Structure 			23
#define MEE_Config_Structure 			24
#define MEE_DataFrom_Structure	 		25
#define MEE_DataTo_Structure 			26
#define FlatMon_Config_Structure 		27
#define FlatMon_DataFrom_Structure		28
#define FlatMon_DataTo_Structure 		29
#define VF_Config_Structure 			30
#define VF_DataFrom_Structure 			31
#define VF_DataTo_Structure 			32
#define Module_Visibility_Structure 		33
#define Module_Visibility_Info	 		34
#define Module_Status 				35
#define MWPC_Config_Structure 			36
#define MWPC_DataFrom_Structure 		37
#define MWPC_DataTo_Structure 			38
#define LS_Config_Structure 			39
#define LS_DataFrom_Structure 			40
#define LS_DataTo_Structure 			41
#define TerAnG_Config_Structure 		42
#define TerAnG_DataFrom_Structure 		43
#define TerAnG_DataTo_Structure 		44
#define LC1CH_Config_Structure	 		45
#define LC1CH_DataFrom_Structure 		46
#define LC1CH_DataTo_Structure	 		47
#define SmoSen_Config_Structure 		48
#define SmoSen_DataFrom_Structure 		49
#define SmoSen_DataTo_Structure 		50
#define MotSen_Config_Structure 		51
#define MotSen_DataFrom_Structure 		52
#define MotSen_DataTo_Structure 		53
#define LCMCH_Config_Structure	 		54
#define LCMCH_DataFrom_Structure 		55
#define LCMCH_DataTo_Structure			56
#define LeakSen_Config_Structure		57
#define LeakSen_DataFrom_Structure 		58
#define LeakSen_DataTo_Structure		59
#define ThermoReg_Config_Structure 		60
#define ThermoReg_DataFrom_Structure 		61
#define ThermoReg_DataTo_Structure 		62
#define ThermoMon_Config_Structure		63
#define ThermoMon_DataFrom_Structure 		64
#define ThermoMon_DataTo_Structure 		65
						
#define	ERROR			0xFF

#define	s115200		0
#define	s57600		1
#define	s38400		2
#define	s28800		3
#define	s19200		4
#define	s9600		5
#define	s4800		6
#define	s2400		7

void * lkDeviceThread (void * devr);

class DeviceLK;
class DeviceBIT;
class Device2IP;
class DeviceMEE;

class DeviceLK : public DeviceD {
private:    
    UINT	idlk;
//    UINT	device;
    UINT	adr;
    CHAR	sensors[400];
    short	level;
    CHAR	flats[200];
    CHAR	struts[200];
    short	napr;
public:    
    // constructor
    DeviceLK () {};
    ~DeviceLK () {};
    // function read LK config from DB
    // store it to class members and form sequence to send in device
    BYTE* ReadConfig ();    
    // function send to LK operation sequence
    // source data will take from db or class member fields
    int  read_lk (BYTE* dat);
    bool send_lk (UINT op);        
//    int RecieveConfig ();    
    // ReadDataCurrent - read all devices connected to current concentrator
    int ReadDataCurrent ();
    // ReadDataCurrent - read devices with type <type> connected to current concentrator
//    int ReadDataCurrent (int type);
    // ReadDataCurrent - read single device with identificator <idevice> connected to concentrator
//    int ReadDataCurrent (int type, int idevice);
    // send_lk - send to LK command
    bool send_lk (UINT adr, UINT op, UINT num_com, UINT adr_dt);
    // ReadLKConfig - function reads config from db and form string to send in LK (sensor)
    bool LoadLKConfig();
    //int		SendConfig ();
    //int ReadDataArch ();
    //int RWCommand ();    
};
//--------------------------------------------------------------------------------------
class DeviceBIT : public DeviceD {
public:
    UINT	idbit;
//    UINT	device;
    DWORD	rf_interact_interval;
    WORD	ids_lk;
    WORD	ids_module;
    WORD	measure_interval;
    WORD	integral_measure_count;
    float	pi;
    WORD	flat_number;
    BYTE	strut_number;
    BYTE	reserved0;
    WORD	low_error_temperature;
    WORD	high_error_temperature;
    WORD	low_warning_temperature;
    WORD	high_warning_temperature;
    WORD	imitate_temperature;
    BYTE	pa_table;    
    BYTE	reserved1;
    WORD	reserved2;    
    WORD	reserved3;
    WORD	reserved4;
    WORD	napr;    
    // config    
    DeviceBIT () {}
    // function read BIT config from DB
    // store it to class members and form sequence to send in device
    BYTE* ReadConfig ();    
    // function send to BIT operation sequence
    // source data will take from db or class member fields
    bool  send_lk (UINT op, UINT structure);
    // function read data from BIT
    // reading data will be store to DB and temp data class member fields
    int  read_lk (BYTE* dat);
    //int	SendConfig ();
    //int RecieveConfig ();    
    // ReadDataCurrent - read single device with identificator <idevice> connected to concentrator
    int ReadDataCurrent ();
};
//--------------------------------------------------------------------------------------
class Device2IP : public DeviceD {
public:
    UINT	id2ip;
    DWORD	rf_interact_interval;
    WORD	ids_lk;
    WORD	ids_module;
    WORD	measure_interval;
    WORD	flat_number;
    BYTE	reserved0;
    BYTE	reserved1;
    BYTE	imp_weight1;
    BYTE	imp_weight2;
    float	warn_exp1;
    float	warn_exp2;    
    float	imit_exp1;    
    float	imit_exp2;
    BYTE	pa_table;
    BYTE	reserved2;

    // config    
    Device2IP () {}
    // function read 2IP config from DB
    // store it to class members and form sequence to send in device
    BYTE* ReadConfig ();    
    // function send to 2IP operation sequence
    // source data will take from db or class member fields
    bool  send_lk (UINT op);
    // function read data from 2IP
    // reading data will be store to DB and temp data class member fields
    int  read_lk (BYTE* dat);
    // ReadDataCurrent - read single device connected to concentrator
    int ReadDataCurrent ();
};
//--------------------------------------------------------------------------------------
class DeviceMEE : public DeviceD {
public:
    UINT	idmee;
    WORD	ids_lk;
    WORD	ids_module;

    float	limit;
    float	uplimit;    
    float	lolimit;    
    float	calcoef;

    // config    
    DeviceMEE () {}
    // store it to class members and form sequence to send in device
    BYTE* ReadConfig ();    
    // function send to 2IP operation sequence
    // source data will take from db or class member fields
    bool  send_lk (UINT op);
    // function read data from 2IP
    // reading data will be store to DB and temp data class member fields
    int  read_lk (BYTE* dat);
    // ReadDataCurrent - read single device connected to concentrator
    int ReadDataCurrent ();
};
