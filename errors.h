//  Errors list.
//  Values are 32 bit values layed out as follows:
//   7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//  +---+-+-+-------+---------------+---------------+---------------+
//  |ErrType|Device |Reserved       |             Code              |
//  +---+-+-+-------+---------------+---------------+---------------+
//  where
//  Err - is the severity code
//  00 - Success
//  01 - Informational
//  10 - Warning
//  11 - Error
//  100 - Fault
//
//  Module - programm module code
//  0000 - system error
//  0001 - BIT error
//  0010 - 2IP error 
//  0011 - flat view error
//  0100 - MEE error
//  0101 - IRP error

#define SEND_PROBLEM		0x1
#define RECIEVE_PROBLEM		0x2

#define DATABASE_CONNECT_ERROR		0x100
#define DATABASE_INIT_ERROR		0x101

#define BUFFER_PROBLEM 		0x201	// Memory not allocated for buffer
#define SENSOR_INCORRECT	0x202	// Sensor num incorrect

#define THREAD_STOPPED		0x301
#define CRC_ERROR		0x305

#define H1_ZERO			0x401
#define H2_ZERO			0x402
#define T_BIG			0x403
#define VCC_WARNING		0x404
#define RSSI_WARNING		0x405
#define V_ERROR			0x406
#define Q_ERROR			0x407
#define W_ERROR			0x408
#define INCORRECT_DATE		0x409

#define	WRONG_MARKER		0x410

#define T1_UZERO		0x1001
#define T2_UZERO		0x1002
#define H1_UZERO		0x1004
#define H2_UZERO		0x1008
#define V1_UZERO		0x1010
#define V2_UZERO		0x1020
#define T1_B120			0x1040
#define T2_B120			0x1080
#define H1_B1000		0x1100
#define H2_B1000		0x1200
#define V1_B10000		0x1400
#define M1_B10000		0x1800
