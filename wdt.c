#include <stdio.h>
#include <sys/io.h>
#include <termios.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#define outp(a, b) outb(b, a)
#define inp(a) inb(a)

int kbhit(void)
{
struct timeval tv;
struct termios old_termios, new_termios;
int error;
int count = 0;
tcgetattr(0, &old_termios);
new_termios = old_termios;
new_termios.c_lflag &= ~ICANON;
new_termios.c_lflag &= ~ECHO;
new_termios.c_cc[VMIN] = 1;
new_termios.c_cc[VTIME] = 1;
error = tcsetattr(0, TCSANOW, &new_termios);
tv.tv_sec = 0;
tv.tv_usec = 100;
select(1, NULL, NULL, NULL, &tv);
error += ioctl(0, FIONREAD, &count);
error += tcsetattr(0, TCSANOW, &old_termios);
return error == 0 ? count : -1;
}

int main(void)
{
iopl(3);
unsigned char c;
unsigned int lTime;
outp(0x22, 0x13); // Lock register
outp(0x23, 0xc5); // Unlock config. register
// 500 mini-second
lTime = 0x20L * 500L;
outp(0x22, 0x3b);
outp(0x23, (lTime >> 16) & 0xff);
outp(0x22, 0x3a);
outp(0x23, (lTime >> 8) & 0xff);
outp(0x22, 0x39);
outp(0x23, (lTime >> 0) & 0xff);
// Reset system
outp(0x22, 0x38);
c = inp(0x23);
c &= 0x0f;
c |= 0xd0; // Reset system. For example, 0x50 to trigger IRQ7
outp(0x22, 0x38);
outp(0x23, c);
// Enable watchdog timer
outp(0x22, 0x37);
c = inp(0x23);
c |= 0x40;
outp(0x22, 0x37);
outp(0x23, c);
outp(0x22, 0x13); // Lock register
outp(0x23, 0x00); // Lock config. register
printf("Press any key to stop trigger timer.\n");

while(!kbhit())
{
outp(0x22, 0x13); // Unlock register
outp(0x23, 0xc5);
outp(0x22, 0x3c);
unsigned char c = inp(0x23);
outp(0x22, 0x3c);
outp(0x23, c | 0x40);
outp(0x22, 0x13); // Lock register
outp(0x23, 0x00);
}

printf("System will reboot after 500 milli-seconds.\n");
return 0;
}