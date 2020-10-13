#ifndef _FT1_2_H_
#define _FT1_2_H_

#ifdef WIN32
#include <windows.h>
#endif //WIN32

//#include "../../log/log.h"
#include "buffer/buffer.h"
#include "types.h"

// Frame markers
#define FT1_2_SF_FL		0x10 // Start frame with fixed length
#define FT1_2_EF_FL		0x16 // End frame with fixed length

#define FT1_2_SF_VL		0x68 // Start frame with variable length
#define FT1_2_EF_VL		0x16 // End frame with variable length

#define FT1_2_ACK		0xa2 // Confirmation byte (short answer yes)
#define FT1_2_ERR		0xe5 // Error byte? sended when some error take a place(i.e. transmission errors)

#define FT1_2_PRM		0x40
#define FT1_2_FCB		0x04
#define FT1_2_ACD		0x04
#define FT1_2_FCV		0x10
#define FT1_2_DFC		0x10
#define FT1_2_FC_MASK		0x0f

#define FT1_2_DEFAULT_CTRL FT1_2_PRM

#define FT1_2_ADDR_MIN			0x00
#define FT1_2_ADDR_MAX			0x7f
//#define FT1_2_ADDR_BROADCAST	0xff //Not used

#define FT1_2_MAX_TX_DATA_LENGTH_VF		0xf9-3
#define FT1_2_MAX_RX_DATA_LENGTH_VF		0xbf-3//0x7f-3 //Depended  from version ov firmware see documentattion
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Generate frame & store it to Buff
size_t FT1_2_FrameFL(const BYTE Ctrl, // Control byte
				   const BYTE Addr, // Byte of address
				   const BYTE Cmd, // Byte of command
				   const BYTE* const Data, const BYTE DataSize, // Data of the frame & size of data in BYTEs
				   BYTE* const Buff, const size_t BuffSize); //Buffer for creating frame// Function return size of generated frame 
// Function return 0 if used incorrect parameters

// Generate frame & store it to Buff with defalt Ctrl
size_t FT1_2_FrameFL(const BYTE Addr, // Byte of address
				   const BYTE Cmd, // Byte of command
				   const BYTE* const Data, const BYTE DataSize, // Data of the frame & size of data in BYTEs
				   BYTE* const Buff, const size_t BuffSize); //Buffer for creating frame
size_t FT1_2_FrameFL(const BYTE Addr, // Byte of address
				   const BYTE Cmd, // Byte of command
				   const buffer* const Data, // Data of the frame & size of data in BYTEs
				   buffer* const Buff); //Buffer for creating frame

// Function return size of generated frame 
// Function return 0 if used incorrect parameters

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Generate frame & store it to Buff
size_t FT1_2_FrameVL(const BYTE Ctrl, // Control byte
				   const BYTE Addr, // Byte of address
				   const BYTE Cmd, // Byte of command
				   const BYTE* const Data, const BYTE DataSize, // Data of the frame & size of data in BYTEs
				   BYTE* const Buff, const size_t BuffSize); //Buffer for creating frame// Function return size of generated frame 
// Function return 0 if used incorrect parameters

// Generate frame & store it to Buff with defalt Ctrl
size_t FT1_2_FrameVL(const BYTE Addr, // Byte of address
				   const BYTE Cmd, // Byte of command
				   const BYTE* const Data, const BYTE DataSize, // Data of the frame & size of data in BYTEs
				   BYTE* const Buff, const size_t BuffSize); //Buffer for creating frame
// Function return size of generated frame
// Function return 0 if used incorrect parameters

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parse frame
size_t FT1_2_Frame(const BYTE* const Buff, const size_t BuffSize, //Buffer for reading frame
				   BYTE& Ctrl, // Control byte
				   BYTE& Addr, // Byte of address
				   BYTE* Data, BYTE& DataReaded, const BYTE DataSize); // Data of the frame & size of data in BYTEs

size_t FT1_2_Frame(const BYTE* const Buff, const size_t BuffSize, //Buffer for reading frame
				   BYTE& Ctrl, // Control byte
				   BYTE& Addr, // Byte of address
				   BYTE& Cmd, // Byte of command
				   BYTE* Data, BYTE& DataReaded, const BYTE DataSize); // Data of the frame & size of data in BYTEs
				  
// Function return size of parsed frame
// Function return 0 if used incorrect parameters

WORD I_HOUR(const unsigned short Hour, const unsigned short Day, const unsigned short Month, const unsigned short Year,
			const unsigned short D);

#endif //_FT1_2_H_
