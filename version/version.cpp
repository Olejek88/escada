//----------------------------------------------------------------------------
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
//----------------------------------------------------------------------------
int main(int numargs, char *argv[])
{
 FILE *target;
 struct stat ms;
 time_t tim;
 struct tm *ttime;
 struct tm *rtime;
 int  major_version,minor_version,build,cday=0;
 target = fopen("version/version.h","r+"); 
 printf ("version file name <version.h> pointer %p\n",target);
 if (target!=NULL)
    {
     stat ("version/version.h",&ms);
     tim=time(&tim);
     rtime=localtime(&tim);
     cday=rtime->tm_mday;

     ttime=gmtime(&ms.st_mtime);
     fscanf (target,"#define version \"%d.%d.%d\"",&major_version,&minor_version,&build);
     //printf ("%d %d",ttime->tm_mday,rtime->tm_mday);
     if (ttime->tm_mday!=cday)
        {
	 minor_version++;
	 if (minor_version>999) { minor_version=0; major_version++; }
	 build=1;
	}
     else build++;
     printf ("ver: %d.%d.%d\n",major_version,minor_version,build);
     fclose (target);
     target = fopen("version/version.h","w"); 
     fprintf (target,"#define version \"%d.%03d.%03d\"\n",major_version,minor_version,build);
     fprintf (target,"#define buildtime \"%d-%02d-%02d %02d:%02d:%02d\"\n",rtime->tm_year+1900,rtime->tm_mon+1,rtime->tm_mday,rtime->tm_hour,rtime->tm_min,rtime->tm_sec);
     fclose (target);
    }
 else 
    {
     target = fopen("version/version.h","w");
     if (target) 
     fclose (target);
    }
 return 0;
}
