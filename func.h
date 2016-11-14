#include <net/if.h>
//---------------------------------------------------------------------------
static termios 	tios;
//---------------------------------------------------------------------------
void ULOGW (const char* string, ...);
UINT baudrate (UINT baud);
VOID Events (db dbase, DWORD evnt, DWORD device);
VOID Events (db dbase, DWORD evnt, DWORD device, FLOAT value);
VOID Events (db dbase, DWORD evnt, DWORD device, DWORD code, FLOAT value, DWORD type, DWORD chan);

unsigned char Crc8 (BYTE* data, UINT lent);
BYTE crc8_compute_tabel ( BYTE* str, BYTE col);
BOOL OpenTTY (UINT fd, SHORT blok, UINT speed);

char* getip(char *ip);
BOOL SetDeviceStatus(db dbase, UINT devtype, UINT devnum, UINT devadr, UINT type, CHAR* datatime);
BOOL UpdateThreads (db dbase, uint16_t thread_id, uint8_t global, uint8_t start, uint8_t curr, uint16_t curr_adr, uint8_t status, uint8_t type, CHAR* datatime);
// function store current data to database
BOOL StoreData (db dbase, UINT dv, UINT prm, UINT pipe, UINT status, FLOAT value, uint8_t dest);
// function store archive data to database
BOOL StoreData (db dbase, UINT dv, UINT prm, UINT pipe, UINT type, UINT status, FLOAT value, CHAR* data, uint8_t dest);
BOOL StoreData (db dbase, UINT dv, UINT prm, UINT pipe, UINT type, UINT status, FLOAT value, CHAR* data, uint8_t dest, uint16_t channel);
BOOL StoreData (db dbase, UINT dv, UINT prm, UINT pipe, UINT status, FLOAT value, uint8_t dest, uint16_t chan);
UINT  GetChannelNum (db dbase, uint16_t prm, uint16_t pipe, UINT device);
uint8_t BCD (uint8_t dat);

