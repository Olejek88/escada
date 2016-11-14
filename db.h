
class db
{
 MYSQL 	*mysql; 
 MYSQL_ROW row;
 MYSQL_RES *res;
public:
   db();           // Constructor
   BOOL sqlconn(const CHAR * tbname, const CHAR * login, const CHAR * pass);
   //UINT sqlexec(CHAR* query, MYSQL_RES *res);
   MYSQL_RES* sqlexec(const CHAR* query);
   VOID sqldisconn(void);
};
