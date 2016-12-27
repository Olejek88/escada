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
 int ret,lsw,msw,ieee;
 int i;
 float	qp=0, t1, t2, v1, v2, v3, p1, p2,m1,m2,qn;

 dbase.sqlconn("","root","");	// connect to database
 if (debug>1) ULOGW ("[mods] modbus_init_tcp 192.168.255.1:502");
 modbus_init_tcp(&mb_param, "192.168.255.1", 502);
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
     t1=LoadData (5007, 4, 0, 0, (CHAR *)"");
     t2=LoadData (5007, 4, 1, 0, (CHAR *)"");
     p1=LoadData (5007, 16, 0, 0, (CHAR *)"")/10.1792;
     p2=LoadData (5007, 16, 1, 0, (CHAR *)"")/10.1792;
     v1=LoadData (5007, 11, 0, 0, (CHAR *)"");
     v2=LoadData (5007, 11, 1, 0, (CHAR *)"");
     v3=LoadData (5007, 11, 5, 0, (CHAR *)"");
     qp=LoadData (5007, 13, 2, 0, (CHAR *)"");
     m1=LoadData (5007, 11, 0, 7, (CHAR *)"");
     m2=LoadData (5007, 11, 1, 7, (CHAR *)"");
     qn=LoadData (5007, 13, 2, 7, (CHAR *)"");
     if (debug>1) ULOGW ("[mods] qp=%f, t1=%f, t2=%f, p1=%f, p2=%f, v1=%f, v2=%f, v3=%f",qp,t1,t2,p1,p2,v1,v2,v3);
     
     ieee=*((int *)&t1); lsw=ieee<<16; lsw=ieee>>16; msw=ieee>>16;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+0] = msw;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+1] = lsw;
     ieee=*((int *)&t2); lsw=ieee<<16; lsw=ieee>>16; msw=ieee>>16;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+2] = msw;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+3] = lsw;

     ieee=*((int *)&p1); lsw=ieee<<16; lsw=ieee>>16; msw=ieee>>16;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+10] = msw;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+11] = lsw;
     ieee=*((int *)&p2); lsw=ieee<<16; lsw=ieee>>16; msw=ieee>>16;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+12] = msw;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+13] = lsw;

     ieee=*((int *)&v1); lsw=ieee<<16; lsw=ieee>>16; msw=ieee>>16;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+4] = msw;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+5] = lsw;
     ieee=*((int *)&v2); lsw=ieee<<16; lsw=ieee>>16; msw=ieee>>16;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+6] = msw;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+7] = lsw;

     ieee=*((int *)&qp); lsw=ieee<<16; lsw=ieee>>16; msw=ieee>>16;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+14] = msw;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+15] = lsw;

     ieee=*((int *)&m1); lsw=ieee<<16; lsw=ieee>>16; msw=ieee>>16;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+16] = msw;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+17] = lsw;

     ieee=*((int *)&m2); lsw=ieee<<16; lsw=ieee>>16; msw=ieee>>16;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+18] = msw;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+19] = lsw;

     ieee=*((int *)&qn); lsw=ieee<<16; lsw=ieee>>16; msw=ieee>>16;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+20] = msw;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+21] = lsw;

     ieee=*((int *)&v3); lsw=ieee<<16; lsw=ieee>>16; msw=ieee>>16;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+24] = msw;
     mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+25] = lsw;

     //mb_mapping.tab_input_registers[UT_INPUT_REGISTERS_ADDRESS+20] = p1;
     //mb_mapping.tab_input_registers[UT_INPUT_REGISTERS_ADDRESS+24] = p2;
     //mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+8] = v1;
     //mb_mapping.tab_holding_registers[UT_HOLDING_REGISTERS_ADDRESS+12] = v2;
     //mb_mapping.tab_input_registers[UT_INPUT_REGISTERS_ADDRESS+8] = v1;
     //mb_mapping.tab_input_registers[UT_INPUT_REGISTERS_ADDRESS+12] = v2;
     //mb_mapping.tab_input_registers[UT_INPUT_REGISTERS_ADDRESS+28] = q1;
     //mb_mapping.tab_input_registers[UT_INPUT_REGISTERS_ADDRESS+48] = v3;
                
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
 if (type==0) sprintf (query,"SELECT * FROM prdata WHERE pipe=%d AND type=%d AND prm=%d AND device=%d",pipe,type,prm,dv);
 else sprintf (query,"SELECT SUM(value) FROM hours WHERE pipe=%d AND type=1 AND prm=%d AND device=%d",pipe,prm,dv);
 // if (debug>2)  ULOGW("[mods] %s",query);
 res=dbase.sqlexec(query);
 if (row=mysql_fetch_row(res))
    {
     if (type>0 && row[0]!=NULL) rt=atof(row[0]);
     if (type==0) rt=atof(row[5]);
     //ULOGW("[mt] %s [%f]",query,rt);
    }
 if (res) mysql_free_result(res);
 return rt;
}
