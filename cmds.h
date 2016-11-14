#ifndef _CMDS_H_
#define _CMDS_H_

// Specification T.10.06.59RD //In russian to english transcription
#define CMD_CNTRL_FAILURE		0x00 //Control of failures
#define CMD_READ_PARAM			0x01 //Read parameter
#define CMD_READ_EXT_MEM		0x02 //Read external memory
#define CMD_READ_INT_MEM		0x03 //Read internal memory
#define CMD_READ_PRG_MEM		0x04 //Read program memory
#define CMD_WRITE_PARAM		0x05 //Write param
#define CMD_WRITE_EXT_MEM		0x06 //Write to external memeory
#define CMD_WRITW_INT_MEM		0x07 //Write to internal memory
#define CMD_START				0x08 //Start
#define CMD_STOP				0x09 //Stop
#define CMD_END_FULL_ACCESS	0x0b //End full access
#define CMD_WORK_BIT_PARAM		0x0c //Work with bit paramaeter
#define CMD_REPRG_DATA			0x0d //Reprogramming data
#define CMD_CLEAR_MEM			0x0e //Clear memory
#define CMD_CHANGE_PRG			0x0f //Change programm
#define CMD_EXCHANGE_SUPERFLO	0x10 //Make exchange with Superflo
#define CMD_READ_PARAM_SLAVE	0x11 //Redad parameter from slave device
#define CMD_READ_ARC_EVENTS	0x12 //Read archive of events
#define CMD_READ_PARAM_ARR		0x13 //Read package of parameters
#define CMD_WRITE_PARAM_SLAVE	0x14 //Write parameter to slave device



// Extended specification T.10.06.59RD-D1 //In russian to english transcription
#define CMD_READ_INDEX_PARAM	0x15 // Read index parameter
#define CMD_WRITE_INDEX_PARAM	0x16 // Write index parameter
#define CMD_SET_ACCESS_LEVEL	0x17 // Set access level
#define CMD_CLEAR_INDEX_PARAM	0x18 // Clear index parameter



#define CMD_ACCESS_LEVEL_USER		0x01
#define CMD_ACCESS_LEVEL_ADJUSTER	0x02
#define CMD_ACCESS_LEVEL_VENDOR		0x03
//#define CMD_ACCESS_LEVEL_MASK		0x03

#endif //_CMDS_H_
