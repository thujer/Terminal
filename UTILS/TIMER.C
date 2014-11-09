#include <conio.h>
#include <stdlib.h>
#include <dos.h>

unsigned char T,CT0,CT1;

void main()
{
  while(1)
  {
    CT0=peekb(0,0x46C);

    delay(random(20)+2000);

    CT1=peekb(0,0x46C);

    if(CT0<peekb(0,0x46C)) T=CT1 - CT0;
    else T=(0xFF-CT0) + CT1;

    cprintf("%X\n\r",T);
    if(inp(0x60)==1) break;
  }
}