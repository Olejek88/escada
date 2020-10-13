#include "mdm.h"
#include <signal.h>

int Mb_device;				/* device tu use */
int Mbm_Pid_Child;		/* PID child used to read the slave answer */
int Mbm_Pid_Sleep;		/* PID use to wait the end of the timeout */
byte *Mbm_result;			/* byte readed on the serial port : answer of the slave */


#include <fcntl.h>

/************************************************************************************
		Mb_test_crc : check the crc of a packet
*************************************************************************************
input :
-------
trame  : packet with is crc
n      : lenght of the packet without tht crc
                              ^^^^^^^
answer :
--------
1 = crc fealure
0 = crc ok
************************************************************************************/
int Mb_test_crc(byte trame[],int n)
{
	unsigned int crc,i,j,carry_flag,a;
	crc=0xffff;
	for (i=0;i<n;i++)
	{
		crc=crc^trame[i];
		for (j=0;j<8;j++)
		{
			a=crc;
			carry_flag=a&0x0001;
			crc=crc>>1;
			if (carry_flag==1)
				crc=crc^0xa001;
		}
	}
   if (Mb_verbose)
      printf("test crc %0x %0x\n",(crc&255),(crc>>8));
	if ((trame[n+1]!=(crc>>8)) || (trame[n]!=(crc&255)))
      return 1;
   else
      return 0;
}

/************************************************************************************
		Mb_calcul_crc : compute the crc of a packet and put it at the end
*************************************************************************************
input :
-------
trame  : packet with is crc
n      : lenght of the packet without tht crc
                              ^^^^^^^
answer :
--------
crc
************************************************************************************/
int Mb_calcul_crc(byte trame[],int n)
{
	unsigned int crc,i,j,carry_flag,a;
	crc=0xffff;
	for (i=0;i<n;i++)
	{
		crc=crc^trame[i];
		for (j=0;j<8;j++)
		{
			a=crc;
			carry_flag=a&0x0001;
			crc=crc>>1;
			if (carry_flag==1)
				crc=crc^0xa001;
		}
	}
	trame[n+1]=crc>>8;
	trame[n]=crc&255;
	return crc;
}
/************************************************************************************
		Mb_close_device : Close the device
*************************************************************************************
input :
-------
Mb_device : device descriptor

no output
************************************************************************************/
void Mb_close_device(int Mb_device)
{
  if (tcsetattr (Mb_device,TCSANOW,&saved_tty_parameters) < 0)
    perror("Can't restore terminal parameters ");
  close(Mb_device);
}

/************************************************************************************
		Mb_open_device : open the device
*************************************************************************************
input :
-------
Mbc_port   : string with the device to open (/dev/ttyS0, /dev/ttyS1,...)
Mbc_speed  : speed (baudrate)
Mbc_parity : 0=don't use parity, 1=use parity EVEN, -1 use parity ODD
Mbc_bit_l  : number of data bits : 7 or 8 	USE EVERY TIME 8 DATA BITS
Mbc_bit_s  : number of stop bits : 1 or 2    ^^^^^^^^^^^^^^^^^^^^^^^^^^

answer  :
---------
device descriptor
************************************************************************************/
int Mb_open_device(char Mbc_port[20], int Mbc_speed, int Mbc_parity, int Mbc_bit_l, int Mbc_bit_s)
{
  int fd;

  /* open port */
  fd = open(Mbc_port,O_RDWR | O_NOCTTY | O_NONBLOCK | O_NDELAY) ;
  if(fd<0)
  {
    perror("Open device failure\n") ;
    exit(-1) ;
  }

  /* save olds settings port */
  if (tcgetattr (fd,&saved_tty_parameters) < 0)
  {
    perror("Can't get terminal parameters ");
    return -1 ;
  }

  /* settings port */
  bzero(&Mb_tio,sizeof(&Mb_tio));

  switch (Mbc_speed)
  {
     case 0:
        Mb_tio.c_cflag = B0;
        break;
     case 50:
        Mb_tio.c_cflag = B50;
        break;
     case 75:
        Mb_tio.c_cflag = B75;
        break;
     case 110:
        Mb_tio.c_cflag = B110;
        break;
     case 134:
        Mb_tio.c_cflag = B134;
        break;
     case 150:
        Mb_tio.c_cflag = B150;
        break;
     case 200:
        Mb_tio.c_cflag = B200;
        break;
     case 300:
        Mb_tio.c_cflag = B300;
        break;
     case 600:
        Mb_tio.c_cflag = B600;
        break;
     case 1200:
        Mb_tio.c_cflag = B1200;
        break;
     case 1800:
        Mb_tio.c_cflag = B1800;
        break;
     case 2400:
        Mb_tio.c_cflag = B2400;
        break;
     case 4800:
        Mb_tio.c_cflag = B4800;
        break;
     case 9600:
        Mb_tio.c_cflag = B9600;
        break;
     case 19200:
        Mb_tio.c_cflag = B19200;
        break;
     case 38400:
        Mb_tio.c_cflag = B38400;
        break;
     case 57600:
        Mb_tio.c_cflag = B57600;
        break;
     case 115200:
        Mb_tio.c_cflag = B115200;
        break;
     case 230400:
        Mb_tio.c_cflag = B230400;
        break;
     default:
        Mb_tio.c_cflag = B9600;
  }
  switch (Mbc_bit_l)
  {
     case 7:
        Mb_tio.c_cflag = Mb_tio.c_cflag | CS7;
        break;
     case 8:
     default:
        Mb_tio.c_cflag = Mb_tio.c_cflag | CS8;
        break;
  }
  switch (Mbc_parity)
  {
     case 1:
        Mb_tio.c_cflag = Mb_tio.c_cflag | PARENB;
        Mb_tio.c_iflag = ICRNL;
        break;
     case -1:
        Mb_tio.c_cflag = Mb_tio.c_cflag | PARENB | PARODD;
        Mb_tio.c_iflag = ICRNL;
        break;
     case 0:
     default:
        Mb_tio.c_iflag = IGNPAR | ICRNL;
        break;
  }

  if (Mbc_bit_s==2)
     Mb_tio.c_cflag = Mb_tio.c_cflag | CSTOPB;
     
  Mb_tio.c_cflag = Mb_tio.c_cflag | CLOCAL | CREAD;
  Mb_tio.c_oflag = 0;
  Mb_tio.c_lflag = 0; /*ICANON;*/
  Mb_tio.c_cc[VMIN]=1;
  Mb_tio.c_cc[VTIME]=0;

  /* clean port */
  tcflush(fd, TCIFLUSH);

  fcntl(fd, F_SETFL, FASYNC);
  /* activate the settings port */
  if (tcsetattr(fd,TCSANOW,&Mb_tio) <0)
  {
    perror("Can't set terminal parameters ");
    return -1 ;
  }
  
  /* clean I & O device */
  tcflush(fd,TCIOFLUSH);
  
   if (Mb_verbose)
   {
      printf("setting ok:\n");
      printf("device        %s\n",Mbc_port);
      printf("speed         %d\n",Mbc_speed);
      printf("data bits     %d\n",Mbc_bit_l);
      printf("stop bits     %d\n",Mbc_bit_s);
      printf("parity        %d\n",Mbc_parity);
   }
   return fd ;
}

/************************************************************************************
		Mb_rcv_print : print a character
This function can be use with slave or master to print a character when it receive one
*************************************************************************************
input :
-------
c : character

no output
************************************************************************************/
void Mb_rcv_print(unsigned char c)
{
   printf("-> receiving byte :0x%x %d \n",c,c);
}

/************************************************************************************
		Mb_snd_print : print a character
This function can be use with slave or master to print a character when it send one
*************************************************************************************
input :
-------
c : character

no output
************************************************************************************/
void Mb_snd_print(unsigned char c)
{
   printf("<- sending byte :0x%x %d \n",c,c);
}

char *Mb_version(void)
{
   return VERSION;
}

/************************************************************************************
		Mbm_get_data : thread reading data on the serial port
*************************************************************************************
input :
-------
len	:	number of data to read;

no output
************************************************************************************/
void Mbm_get_data(int *len )
{
   int i;
   byte read_data;

   Mbm_Pid_Child=getpid();


      if (Mb_verbose)
         fprintf(stderr,"starting receiving data, total length : %d \n",*len);
   for(i=0;i<(*len);i++)
   {
      /* read data */
      read(Mb_device,&read_data,1);

      /* store data to the slave answer packet */
      Mbm_result[i]=read_data;
      
      /* call the pointer function if exist */
      if(Mb_ptr_rcv_data!=NULL)
         (*Mb_ptr_rcv_data)(read_data);
	 //(*Mb_ptr_rcv_data)();
      if (Mb_verbose)
         fprintf(stderr,"receiving byte :0x%x %d (%d)\n",read_data,read_data,Mbm_result[i]);
   }
   if (Mb_verbose)
      fprintf(stderr,"receiving data done\n");
}

/************************************************************************************
		Mbm_sleep : thread wait timeout
*************************************************************************************
input :
-------
timeout : duduration of the timeout in ms

no output
************************************************************************************/
void Mbm_sleep(int *timeout)
{
   Mbm_Pid_Sleep=getpid();
	if (Mb_verbose)
      fprintf(stderr,"sleeping %d ms\n",*timeout);

   usleep(*timeout*1000);
}

/************************************************************************************
		Mbm_send_and_get_result : send data, and wait the answer of the slave
*************************************************************************************
input :
-------
trame     : packet to send
timeout   : duduration of the timeout in ms
long_emit : length of the packet to send
longueur  : length of the packet to read

answer :
--------
0         : timeout failure
1         : answer ok
************************************************************************************/
int Mbm_send_and_get_result(byte trame[], int timeout, int long_emit, int longueur)
{
   int i,stat1=-1,stat2=-1;

	pthread_t thread1,thread2;
   Mbm_result = (unsigned char *) malloc(longueur*sizeof(unsigned char));

   /* clean port */
   tcflush(Mb_device, TCIFLUSH);

   /* create 2 threads for read data and to wait end of timeout*/
   pthread_create(&thread2, NULL,(void *(*)(void *))&Mbm_sleep,&timeout);
   pthread_create(&thread1, NULL,(void *(*)(void *))&Mbm_get_data,&longueur);

	if (Mb_verbose)
      fprintf(stderr,"start writing \n");
   for(i=0;i<long_emit;i++)
   {
      /* send data */
      write(Mb_device,&trame[i],1);
      /* call pointer function if exist */
      if(Mb_ptr_snd_data!=NULL)
         (*Mb_ptr_snd_data)(trame[i]);
   }

  if (Mb_verbose)
      fprintf(stderr,"write ok\n");

   do {
      if (Mbm_Pid_Child!=0)
         /* kill return 0 if the pid is running or -1 if the pid don't exist */
         stat1=kill(Mbm_Pid_Child,0);
      else
         stat1=0;

      if (Mbm_Pid_Sleep!=0)
         stat2=kill(Mbm_Pid_Sleep,0);
      else
         stat2=0;

      /* answer of the slave terminate or and of the timeout */
      if (stat1==-1 || stat2==-1) 
         break;

   } while(1); 
   if (Mb_verbose)
   {
      fprintf(stderr,"pid reading %d return %d\n",Mbm_Pid_Child,stat1);
      fprintf(stderr,"pid timeout %d return %d\n",Mbm_Pid_Sleep,stat2);
   }

   /* stop both childs */
   Mbm_Pid_Child=0;
   Mbm_Pid_Sleep=0;
   pthread_cancel(thread1);
   pthread_cancel(thread2);
   /* error : timeout fealure */
   if (stat1==0)
   {
      return 0;
   }
   /* ok : store the answer packet in the data trame */
   for (i=0;i<=longueur;i++)
      trame[i]=Mbm_result[i];

   return 1;
}
      


/************************************************************************************
   				Mbm_master : comput and send a master packet
*************************************************************************************
input :
-------
Mb_trame	  : struct describing the packet to comput
   					device		: device descriptor
   					slave 		: slave number to call
                  function 	: modbus function
                  address		: address of the slave to read or write
                  length		: lenght of data to send
data_in	  : data to send to the slave
data_out	  : data to read from the slave
timeout	  : timeout duration in ms
ptrfoncsnd : function to call when master send data (can be NULL if you don't whant to use it)
ptrfoncrcv : function to call when master receive data (can be NULL if you don't whant to use it)
*************************************************************************************
answer :
--------
0 : OK
-1 : unknow modbus function
-2 : CRC error in the slave answer
-3 : timeout error
-4 : answer come from an other slave
*************************************************************************************/
int Mb_master(Mbm_trame Mbtrame,int data_in[], int data_out[],void *ptrfoncsnd, void *ptrfoncrcv)
{
   int i,longueur,long_emit;
   int slave, function, adresse, nbre;
   byte trame[256];

   Mb_device=Mbtrame.device;
   slave=Mbtrame.slave;
   function=Mbtrame.function;
   adresse=Mbtrame.address;
   nbre=Mbtrame.length;
   Mb_ptr_snd_data=(void(*)())ptrfoncsnd;
   Mb_ptr_rcv_data=(void(*)())ptrfoncrcv;
      
   switch (function)
   {
      case 0x03:
      case 0x04:
         /* read n byte */
         trame[0]=slave;
         trame[1]=function;
         trame[2]=adresse>>8;
         trame[3]=adresse&0xFF;
         trame[4]=nbre>>8;
         trame[5]=nbre&0xFF;
         /* comput crc */
         Mb_calcul_crc(trame,6);
         /* comput length of the packet to send */
         long_emit=8;
         break;
         
      case 0x06:
         /* write one byte */
         trame[0]=slave;
         trame[1]=function;
         trame[2]=adresse>>8;
         trame[3]=adresse&0xFF;
         trame[4]=data_in[0]>>8;
         trame[5]=data_in[0]&0xFF;
         /* comput crc */
         Mb_calcul_crc(trame,6);
         /* comput length of the packet to send */
         long_emit=8;
         break;

      case 0x07:
         /* read status */
         trame[0]=slave;
         trame[1]=function;
         /* comput crc */
         Mb_calcul_crc(trame,2);
         /* comput length of the packet to send */
         long_emit=4;
         break;
         
      case 0x08:
         /* line test */
         trame[0]=slave;
         trame[1]=0x08;
         trame[2]=0;
         trame[3]=0;
         trame[4]=0;
         trame[5]=0;
         Mb_calcul_crc(trame,6);
         /* comput length of the packet to send */
         long_emit=8;
         break;
         
      case 0x10:
         /* write n byte  */
         trame[0]=slave;
         trame[1]=0x10;
         trame[2]=adresse>>8;
         trame[3]=adresse&0xFF;
         trame[4]=nbre>>8;
         trame[5]=nbre&0xFF;
         trame[6]=nbre*2;
         for (i=0;i<nbre;i++)
         {
            trame[7+i*2]=data_in[i]>>8;
            trame[8+i*2]=data_in[i]&0xFF;
         }
         /* comput crc */
         Mb_calcul_crc(trame,7+nbre*2);
         /* comput length of the packet to send */
         long_emit=(nbre*2)+9;
         break;
      default:
         return -1;
   }
	if (Mb_verbose) 
   {
   	fprintf(stderr,"send packet length %d\n",long_emit);
      for(i=0;i<long_emit;i++)
         fprintf(stderr,"send packet[%d] = %0x\n",i,trame[i]);
   }
   
   /* comput length of the slave answer */
   switch (function)
   {
      case 0x03:
      case 0x04:
         longueur=5+(nbre*2);
         break;
      
      case 0x06:
      case 0x08:
      case 0x10:
      longueur=8;
         break;

      case 0x07:
      longueur=5;
         break;

      default:
         return -1;
         break;
   }

   /* send packet & read answer of the slave
      answer is stored in trame[] */
	if(!Mbm_send_and_get_result(trame,Mbtrame.timeout,long_emit,longueur))
      return -3;	/* timeout error */
  	if (Mb_verbose)
   {
      fprintf(stderr,"answer :\n");
      for(i=0;i<longueur;i++)
         fprintf(stderr,"answer packet[%d] = %0x\n",i,trame[i]);
   }
   
   if (trame[0]!=slave)
      return -4;	/* this is not the right slave */

   switch (function)
   {
      case 0x03:
      case 0x04:
         /* test received data */
         if (trame[1]!=0x03 && trame[1]!=0x04)
            return -2;
         if (Mb_test_crc(trame,3+(nbre*2)))
            return -2;
         /* data are ok */
         if (Mb_verbose)
            fprintf(stderr,"Reader data \n");
         for (i=0;i<nbre;i++)
         {
            data_out[i]=(trame[3+(i*2)]<<8)+trame[4+i*2];
            if (Mb_verbose)
               fprintf(stderr,"data %d = %0x\n",i,data_out[i]);
         }
         break;
      
      case 0x06:
         /* test received data */
         if (trame[1]!=0x06)
            return -2;
         if (Mb_test_crc(trame,6))
            return -2;
         /* data are ok */
         if (Mb_verbose)
            fprintf(stderr,"data stored succesfull !\n");
         break;

      case 0x07:
         /* test received data */
         if (trame[1]!=0x07)
            return -2;
         if (Mb_test_crc(trame,3))
            return -2;
         /* data are ok */
         data_out[0]=trame[2];	/* store status in data_out[0] */
         if (Mb_verbose)
            fprintf(stderr,"data  = %0x\n",data_out[0]);
         break;

      case 0x08:
         /* test received data */
         if (trame[1]!=0x08)
            return -2;
         if (Mb_test_crc(trame,6))
            return -2;
         /* data are ok */
         if (Mb_verbose)
            fprintf(stderr,"Loopback test ok \n");
         break;

      case 0x10:
         /* test received data */
         if (trame[1]!=0x10)
            return -2;
         if (Mb_test_crc(trame,6))
            return -2;
         /* data are ok */
         if (Mb_verbose)
            fprintf(stderr,"%d setpoint stored succesfull\n",(trame[4]<<8)+trame[5]);
         break;

      default:
         return -1;
         break;
   }
   return 0;
}

