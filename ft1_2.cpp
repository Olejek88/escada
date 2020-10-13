#include "ft1_2.h"
#include "main.h"

VOID 	ULOGW (CHAR* string, ...);		// log function


BYTE CRC(const BYTE* const Data, const BYTE DataSize)
    {
     BYTE _CRC = 0;
     BYTE* _Data = (BYTE*)Data;	
	for(unsigned int i = 0; i < DataSize; i++) _CRC += *_Data++;	
	return _CRC;
    };

bool IS_LEAP_YEAR(const unsigned short Year)
    {
     return (!(Year%4) && ((Year%100) || !(Year%400)))?true:false;
    };
// Generate index for month
// Comments: Month in [1..12]
BYTE I_MON_12(const unsigned short Month){
	if(Month == 0 || Month > 12){
		if (debug>1) ULOGW ("[tekon] Incorrect month num [%d]", Month);
		return 0;
	};
	return (BYTE)Month-1;
};



// Generate index for month
// Comments: Month in [1..12]
// Comments: Year in [0..99]
BYTE I_MON_48(const unsigned short Month, const unsigned short Year){
	if(Year > 99){
		if (debug>1) ULOGW ("[tekon] Incorrect year num [%d]", Year);
		return 0;
	};
	
	return (BYTE)((Year%4)*12)+I_MON_12(Month);
};

#define D_16 16
#define D_32 32
#define D_64 64
// Generate index of day
WORD RELATIVE_NUM[]=		{0,31,59,90,120,151,181,212,243,273,304,334};
WORD RELATIVE_NUM_LEAP[]=	{0,31,60,91,121,152,182,213,244,274,305,335};

WORD I_DAY(const unsigned short Day, const unsigned short Month, const unsigned short Year){
	if(Day > 31){
    		if (debug>1) ULOGW ("[tekon] Incorrect day num [%d] or ", Day);
		return 0;
	};
	if(Month == 0 || Month > 12){
		if (debug>1) ULOGW ("[tekon] Incorrect month num [%d]", Month);
		return 0;
	};

	if(IS_LEAP_YEAR(Year)){
		return RELATIVE_NUM_LEAP[Month-1] + Day - 1;
	};

	return RELATIVE_NUM[Month-1] + Day - 1;
};

BYTE I_YEAR(const unsigned short Year){
	return Year%100;
};

WORD I_HOUR(const unsigned short Hour, const unsigned short Day, const unsigned short Month, const unsigned short Year,
			const unsigned short D){
WORD _Year;
	if(Hour > 24){
		if (debug>1) ULOGW ("[tekon] Incorrect month hour [%d]", Hour);
		return 0;
	};
	_Year = I_YEAR(Year);
	return ((365*_Year+_Year/4+I_DAY(Day, Month, Year)+ (IS_LEAP_YEAR(Year)?0:1))%D)*24 + Hour;
};

//[SATRT_BYTE;CONTROL_BYTE;ADDRESS_BYTE;COMMAND_BYTE;DATA;CRC_BYTE;STOP_BYTE]
size_t FT1_2_FrameFL(const BYTE Ctrl, // Control byte
				   const BYTE Addr, // Byte of address
				   const BYTE Cmd, // Byte of command
				   const BYTE* const Data, const BYTE DataSize, // Data of the frame & size of data in BYTEs
				   BYTE* const Buff, const size_t BuffSize){ //Buffer fo creating frame
size_t _iPrc = 0; //Stored bytes
BYTE* _Buff = Buff;
size_t _BuffSize = BuffSize;

	if(!(DataSize + 3 == 6 || DataSize + 3 == 8)){
		if (debug>1) ULOGW ("[tekon] Incorrect frame size out of range [0x%x]", DataSize);
		return 0;
	};
	
	*_Buff++ = FT1_2_SF_FL;
	*_Buff++ = Ctrl;
	*_Buff++ = Addr;
	*_Buff++ = Cmd;

	_BuffSize -= 4;

	//if(memcpy_s(_Buff, _BuffSize,  Data, DataSize)) !!!
	if(!memcpy(_Buff, Data, DataSize))
	{
		if (debug>1) ULOGW ("[tekon] Data not copyed");
	};

	_Buff += DataSize;

	*_Buff++ = CRC(Buff + 1, DataSize+3);
	*_Buff++ = FT1_2_EF_VL;

	_iPrc = 4 + DataSize + 2;

return _iPrc;
};

size_t FT1_2_FrameFL(const BYTE Addr, // Byte of address
				   const BYTE Cmd, // Byte of command
				   const BYTE* const Data, const BYTE DataSize, // Data of the frame & size of data in BYTEs
				   BYTE* const Buff, const size_t BuffSize){ //Buffer fo creating frame

	return FT1_2_FrameFL(FT1_2_DEFAULT_CTRL, Addr, Cmd, Data, DataSize, Buff, BuffSize);
};

size_t FT1_2_FrameFL(const BYTE Addr, // Byte of address
				   const BYTE Cmd, // Byte of command
				   const buffer* const Data, // Data of the frame & size of data in BYTEs
				   buffer* const Buff){ //Buffer for creating frame
	if(Data && Buff){
		return (Buff->us = FT1_2_FrameFL(Addr, Cmd, (BYTE*)Data->ptr, (BYTE)Data->us, (BYTE*)Buff->ptr, (BYTE)Buff->sz));
	}else{
	    if (debug>1) ULOGW ("[tekon] Pointer is NULL Data[%p], Buff[%p]", Data, Buff);
	};
	return 0;
};

//[SATRT_BYTE;LENGTH_BYTE;LENGTH_BYTE;SATRT_BYTE;CONTROL_BYTE;ADDRESS_BYTE;COMMAND_BYTE;DATA;CRC_BYTE;STOP_BYTE]
size_t FT1_2_FrameVL(const BYTE Ctrl, // Control byte
				   const BYTE Addr, // Byte of address
				   const BYTE Cmd, // Byte of command
				   const BYTE* const Data, const BYTE DataSize, // Data of the frame & size of data in BYTEs
				   BYTE* const Buff, const size_t BuffSize){ //Buffer fo creating frame
size_t _iPrc = 0; //Stored bytes
BYTE* _Buff = Buff;
size_t _BuffSize = BuffSize;

	if(DataSize > FT1_2_MAX_TX_DATA_LENGTH_VF){
		if (debug>1) ULOGW ("[tekon] Incorrect frame size out of range [0x%x]", DataSize);
		return 0;
	};

	*_Buff++ = FT1_2_SF_VL;
	*_Buff++ = DataSize + 3;
	*_Buff++ = DataSize + 3;
	*_Buff++ = FT1_2_SF_VL;

	*_Buff++ = Ctrl;
	*_Buff++ = Addr;
	*_Buff++ = Cmd;

	_BuffSize -= 7;

	//if(memcpy_s(_Buff, _BuffSize,  Data, DataSize)) !!!
	if(!memcpy(_Buff, Data, DataSize))
	{
		if (debug>1) ULOGW ("[tekon] Data not copyed");
	};

	_Buff += DataSize;
	
	*_Buff++ = CRC(Buff + 4, DataSize+3);
	*_Buff++ = FT1_2_EF_VL;

	_iPrc = 4+3+ DataSize + 2;

return _iPrc;
};

size_t FT1_2_FrameVL(const BYTE Addr, // Byte of address
				   const BYTE Cmd, // Byte of command
				   const BYTE* const Data, const BYTE DataSize, // Data of the frame & size of data in BYTEs
				   BYTE* const Buff, const size_t BuffSize){ //Buffer fo creating frame

	return FT1_2_FrameVL(FT1_2_DEFAULT_CTRL, Addr, Cmd, Data, DataSize, Buff, BuffSize);
};

size_t FT1_2_Frame(const BYTE* const Buff, const size_t BuffSize, //Buffer for reading frame
				   BYTE& Ctrl, // Control byte
				   BYTE& Addr, // Byte of address
				   BYTE* Data, BYTE& DataReaded, const BYTE DataSize){ // Data of the frame & size of data in BYTEs

size_t _iPrc = 0; //Stored bytes
BYTE* _Buff = (BYTE*)Buff;
size_t _BuffSize = BuffSize;

	if(!_Buff){
		if (debug>1) ULOGW ("[tekon] Pointer to buffer is NULL");
		return 0;
	};

	if(*_Buff == FT1_2_ACK){
		return 1;
	};

	if(*_Buff == FT1_2_ERR){
		if (debug>1) ULOGW ("[tekon] Resived signal [FT1_2_ERR]");
		return 1;
	};

	switch(*_Buff++){
		case FT1_2_SF_FL:

			DataReaded = 6;

			if(CRC(_Buff, DataReaded) != *(_Buff+DataReaded)){
				if (debug>1) ULOGW ("[tekon] CRC error");
			};

			if(*(_Buff+DataReaded+1) != FT1_2_EF_FL){
				if (debug>1) ULOGW ("[tekon] No stop byte in frame");
				return 0;
			};

			Ctrl = *_Buff++;
			Addr = *_Buff++;

			DataReaded -= 2;

			//if(memcpy_s(Data, DataSize,  _Buff, DataReaded)) !!!
			if(!memcpy(Data, _Buff, DataReaded))
			{
				if (debug>1) ULOGW ("[tekon] Data not copyed");
			};
			_iPrc = 1+2+DataReaded+2;
			break;
		case FT1_2_SF_VL:
			if(*_Buff != *(_Buff+1)){
				if (debug>1) ULOGW ("[tekon] Size byte error sizes [%x] [%x]", *_Buff, *(_Buff+1));
				return -1;
			}else{
				DataReaded = *_Buff;
			};
			_Buff +=2;

			if(*_Buff != FT1_2_SF_VL){
				if (debug>1) ULOGW ("[tekon] Repit start byte error [%x]", *_Buff);
				return -1;
			};
			_Buff++;
			
			if(CRC(_Buff, DataReaded) != *(_Buff+DataReaded)){
				if (debug>1) ULOGW ("[tekon] CRC error");			
			};

			if(*(_Buff+DataReaded+1) != FT1_2_EF_VL){
				if (debug>1) ULOGW ("[tekon] No stop byte in frame");
				return 0;
			};

			Ctrl = *_Buff++;
			Addr = *_Buff++;
			DataReaded -= 2;
			/*
			Cmd	= *_Buff++;
			*/

			//DataReaded = DataSize;

			//if(memcpy_s(Data, DataSize,  _Buff, DataReaded))
			if(!memcpy(Data, _Buff, DataReaded))
			{
				if (debug>1) ULOGW ("[tekon] Data not copyed");
			};
			
			_iPrc = 1+2+1+DataReaded+2;
			break;
		default:
			if (debug>1) ULOGW ("[tekon] No start byte in frame");
			return 0;
	};

return _iPrc;
};

size_t FT1_2_Frame(const BYTE* const Buff, const size_t BuffSize, //Buffer for reading frame
				   BYTE& Ctrl, // Control byte
				   BYTE& Addr, // Byte of address
				   BYTE& Cmd, // Byte of command
				   BYTE* Data, BYTE& DataReaded, const BYTE DataSize){ // Data of the frame & size of data in BYTEs
size_t _iPrc = 0; //Stored bytes
unsigned  int _iCnt;
BYTE* _Data;

	_iPrc = FT1_2_Frame(Buff, BuffSize, Ctrl, Addr, Data, DataReaded, DataSize);
	Cmd = *Data;
	_Data = Data;
	for(_iCnt = 0; _iCnt < (unsigned int)(DataReaded-1); _iCnt++){
		*_Data++ = *(_Data+1);
	};
	DataReaded--;

return _iPrc;
};
