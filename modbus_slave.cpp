//-----------------------------------------------------------------------------
#include "types.h"
#include "errors.h"
#include "main.h"
#include "db.h"
//#include <stdio.h>
//#include <unistd.h>
//#include <string.h>
//#include <stdlib.h>
#include "modbus_slave.h"
#include "modbus/modbus.h"
//-----------------------------------------------------------------------------
static	MYSQL_RES 	*res;
static	db		dbase;
static 	MYSQL_ROW 	row;

static 	CHAR   	query[500];
static	UINT	data_adr;	// for test we remember what we send to device
static	UINT	data_len;	// data address and lenght
static	UINT	data_op;	// and last operation

extern 	"C" UINT device_num;	// total device quant
extern 	"C" BOOL WorkRegim;	// work regim flag
extern 	"C" tm *currenttime;	// current system time

extern	BOOL	modbus_slave_thread;
extern  "C" UINT	debug;

static  FLOAT LoadData (UINT dv, UINT prm, UINT pipe, UINT type, CHAR* data);
VOID    ULOGW (const CHAR* string, ...);              // log function
//-----------------------------------------------------------------------------
void * msServerThread (void * devr)
{
 int socket;
 modbus_param_t mb_param;
 modbus_mapping_t mb_mapping;
 int ret;
 int i;
 dbase.sqlconn("","root","");	// connect to database
 if (debug>1) ULOGW ("[mods] modbus_init_tcp 127.0.0.1:1502");
 modbus_init_tcp(&mb_param, "127.0.0.1", 1502);
 modbus_set_debug(&mb_param, TRUE);

 if (debug>1) ULOGW ("[mods] modbus_mapping_new(%d,%d)",UT_HOLDING_REGISTERS_ADDRESS,UT_HOLDING_REGISTERS_NB_POINTS);
 ret = modbus_mapping_new(&mb_mapping,
                 UT_COIL_STATUS_ADDRESS + UT_COIL_STATUS_NB_POINTS,
                 UT_INPUT_STATUS_ADDRESS + UT_INPUT_STATUS_NB_POINTS,
                 UT_HOLDING_REGISTERS_ADDRESS + UT_HOLDING_REGISTERS_NB_POINTS,
                 UT_INPUT_REGISTERS_ADDRESS + UT_INPUT_REGISTERS_NB_POINTS);
 if (ret == FALSE) {
	if (debug>1) ULOGW ("[mods] Memory allocation failed");
        return 0;
    }

 if (debug>1) ULOGW ("[mods] modbus_init_listen_tcp");
 socket = modbus_init_listen_tcp(&mb_param);
        
 while (WorkRegim)
    {
     uint8_t query[MAX_MESSAGE_LENGTH];
     int query_size;

     /** INPUT REGISTERS **/
     mb_mapping.tab_input_registers[UT_INPUT_REGISTERS_ADDRESS+0] =
                    LoadData (5000, 4, 0, 0, (CHAR *)"");
     mb_mapping.tab_input_registers[UT_INPUT_REGISTERS_ADDRESS+2] =
                    LoadData (5000, 4, 1, 0, (CHAR *)"");
                
     ret = modbus_listen(&mb_param, query, &query_size);
     if (ret == 0) {
            if (((query[HEADER_LENGTH_TCP + 4] << 8) + query[HEADER_LENGTH_TCP + 5])
                == UT_HOLDING_REGISTERS_NB_POINTS_SPECIAL) {
                    /* Change the number of values (offset
                           TCP = 6) */
                    query[HEADER_LENGTH_TCP + 4] = 0;
                    query[HEADER_LENGTH_TCP + 5] = UT_HOLDING_REGISTERS_NB_POINTS;
                }
            modbus_manage_query(&mb_param, query, query_size, &mb_mapping);
        } else if (ret == CONNECTION_CLOSED) {
            /* Connection closed by the client, end of server */
            break;
        } else {
	    if (debug>1) ULOGW ("[mods] eror in modbus_listen (%d)\n", ret);
	}
    }
 close(socket);
 modbus_mapping_free(&mb_mapping);
 modbus_close(&mb_param);
 return 0;
}
        
//---------------------------------------------------------------------------------------------------
FLOAT LoadData (UINT dv, UINT prm, UINT pipe, UINT type, CHAR* data)
{
 float	rt=-1;
 if (type>0) sprintf (query,"SELECT * FROM data WHERE source=%d AND type=%d AND prm=%d AND flat=0 AND date=%s",pipe,type,prm,data);
 else sprintf (query,"SELECT * FROM prdata WHERE pipe=%d AND type=%d AND prm=%d AND device=%d",pipe,type,prm,dv);
 if (debug>2)  ULOGW("[mods] %s",query);
 res=dbase.sqlexec(query);
 if (row=mysql_fetch_row(res))
    {
     if (type>0) rt=atof(row[3]);
     if (type==0) rt=atof(row[5]);
     //ULOGW("[mt] %s [%f]",query,rt);
    }
 if (res) mysql_free_result(res);
 return rt;
}
