#include <stdio.h>

unsigned char I,BufIndex;
unsigned char Buf[256];

void main()
{
  for(I=0; I < 255; I++) Buf[I]=I;
  Buf[255]=255;

  BufIndex=232; I=25;
  printf("%2d\n\r",Buf[(unsigned char) (I+BufIndex)]);
}
