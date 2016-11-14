#define VIS_SOFTWARE_VERSION	0x04 // (4) 21 Версия программного обеспечения прибора
#define VIS_DEVICE_TYPE		0x19 // (25) 21 Тип (наименование) прибора
#define VIS_SERIAL		0x2E // (46) 21 Заводской серийный номер прибора ASCIIZ-строка произвольной длины
#define VIS_DATE		0x51 // (81) 2 Отчетное число Беззнаковое 16-бит. число (1 .. 28)
#define VIS_TIME		0x53 // (83) 2 Отчетный час Беззнаковое 16-бит. число (0 .. 23)
#define VIS_PAPERFORMAT		0x55 // (85) 2 Формат бумаги (A4/A3+)
#define VIS_PAPERTYPE		0x57 // (87) 2 Тип бумаги (Рулон/Лист) Беззнаковое 16-бит. число (0 / 1)
#define VIS_INTERFACE		0x59 // (89) 2 Интерфейс удаленного доступа (RS-232/RS-485/Опция) Беззнаковое 16-бит. число (0 .. 2)
#define VIS_SPEED_COM		0x5B // (91) 2 Скорость модемного последовательного порта (9600/19200)
#define VIS_MODEM		0x5D // (93) 2 Наличие модема (Нет/Есть) Беззнаковое 16-бит. число (0 / 1) 
#define VIS_ADDRESS		0x5F // (95) 2 Сетевой номер прибора Беззнаковое 16-бит. число (1 .. 247)
#define VIS1			0x69 // (105) 18 Структура изменяемых параметров т/с №1 прибора
#define VIS2			0x83 // (131) 18 Структура изменяемых параметров т/с №2 прибора
#define VIS3			0x9D // (157) 18 Структура изменяемых параметров т/с №3 прибора
#define VIS_SYSTEM_NUM		0xC5 // (197) 1 Число теплосистем в приборе Беззнаковое 8-бит. число (1 .. 3)

#define	DATETIME		0x0000 	// (0) 3 Текущие Дата и Время прибора
#define	PARAMETRS1		0x0200	// Набор измеряемых текущих параметров т/с по т1
#define	PARAMETRS2		0x0600	// Набор измеряемых текущих параметров т/с по т2
#define	PARAMETRS3		0x0A00	// Набор измеряемых текущих параметров т/с по т3


void * visDeviceThread (void * devr);
class DeviceVIST;

class DeviceVIST : public DeviceD {
public:    
    UINT	idvist;
    UINT	addr;
    UINT	device;
    CHAR	dev_name[25];
    CHAR	version[25];
    CHAR	serial[25];
    CHAR	software[25];
    tm 		*devtime;
public:    
    // constructor
    DeviceVIST () {};
    ~DeviceVIST () {};
    bool ReadVersion ();
    bool ReadTime ();    
    int ReadData (UINT	type);

    bool send_vist (UINT op, UINT reg, UINT nreg, UCHAR* dat);
    int read_vist (BYTE* dat);
};

