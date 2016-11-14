//--------------------------------------------------------------------------------
#include "types.h"
#include "main.h"
#include "errors.h"
#include "db.h"

VOID 	ULOGW (const CHAR* string, ...);		// log function
//VOID 	Events (DWORD evnt, DWORD device);	// events 
extern	"C"	DeviceDK	dk;
extern  "C" 	UINT		debug;
//--------------------------------------------------------------------------------
// Constructor initializes the string chr_ds_name with the data source name.
db::db()
{
}
//--------------------------------------------------------------------------------
// Allocate environment and connection handle, connect to data source, allocate statement handle
//--------------------------------------------------------------------------------
BOOL db::sqlconn(const CHAR * tbname, const CHAR * login, const CHAR * pass)
{
 mysql=mysql_init(NULL);	// init mysql connection
 if (!mysql)
    {
     if (debug>0) ULOGW ("[db] init mysql database........failed [[%d] %s]",mysql_errno(mysql),mysql_error(mysql));
     //Events ((4<<28)&TYPE_DK<<24&DATABASE_INIT_ERROR,dk.iddk);
     mysql_close(mysql);
     return FALSE;    
    }
 if (!mysql_real_connect(mysql, "localhost", login, pass,"",3306,NULL,0))
    { 
     if (debug>0) ULOGW ("[db] connecting to database........failed [[%d] %s]",mysql_errno(mysql),mysql_error(mysql));
     //Events ((4<<28)&TYPE_DK<<24&DATABASE_CONNECT_ERROR,dk.iddk);
     mysql_close(mysql);
     return FALSE;
    }
 else if (debug>1)ULOGW ("[db] connecting to database........success");
 mysql_real_query(mysql, "USE dk",strlen("USE dk"));
 return TRUE;
}
//--------------------------------------------------------------------------------
// Execute SQL command
//UINT db::sqlexec(CHAR* query,MYSQL_RES *rs)
MYSQL_RES* db::sqlexec(const CHAR* query)
{
 //if (debug>2) ULOGW ("[db] mysql query (%s)",query); 
 if (!mysql_real_query(mysql, query, strlen(query)))
    {
     if (strstr((CHAR*)query,"SELECT"))
	{
	 if (!(res=mysql_store_result(mysql)))
	    {
	     if (debug>1) ULOGW ("[db] error in function store_result");
	     /*if (!restart)
		{
	         restart=1;
        	 system ("service mysqld restart"); sleep (15);
		 system ("killall mysqld"); sleep (10);
		 system ("service mysqld start");
	         //int r=execl ("/home/user/dk/rest.sh","");	 
	         ULOGW ("[db] restart mysql");
	         sleep (10);
	         restart=0;
		}
	     else  sleep (45);
	     this->sqlconn("dk","root","");			// connect to database	     	     
	     res=NULL;
	     return res;*/
	     return (0);
	    }
	 //UINT nr=mysql_num_rows(res);
	 //if (debug>1) ULOGW ("[db] res=0x%x",res);
	 return res;
	}
    }
 else
    {
     if (debug>2) ULOGW ("[db] mysql query (%s) error [[%d] %s]",query,mysql_errno(mysql),mysql_error(mysql));     
     //restart=1;     
     //if (mysql_errno(mysql)>2005) 
     /*if (1) 
        {
	 ULOGW ("[db] 2008!!! going to restart mysql");
 	 sleep (5);
	 if (!restart)
	    {
	     restart=1;
    	     system ("service mysqld restart"); sleep (15);
	     system ("killall mysqld"); sleep (10);
    	     system ("service mysqld start");
	     
	     //int r=execl ("/home/user/dk/rest.sh","");	 
	     //\ULOGW ("[db] restart mysql");
	     sleep (10); 
	     restart=0;
	    }
	 else  sleep (40);
	 this->sqlconn("dk","root","");			// connect to database
	 res=NULL; restart=0;
	 return res;
	}     */
     return (0);
    }
// if (debug>2) ULOGW ("[db] mysql end");
// if (res) mysql_free_result(res);
 return (0);
}
//--------------------------------------------------------------------------------
// Free the statement handle, disconnect, free the connection handle
void db::sqldisconn(void)
{
  if (res) mysql_free_result(res);
  mysql_close (mysql);
}

