#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include <STDLIB.H>

unsigned char S1,S2,B;


void main()
{
  clrscr();
  printf("Test prijmu parity: \n\n");
  asm{
       MOV  AH,0
       MOV  DX,1
       MOV  AL,3+8+16+32+64+128
       INT  0x14
     }

  cprintf("AH=%X \n\r",_AH);

  while(1)
  {
    if(inp(0x2FD) & 1)
    {
      asm{
           MOV  AH,2
           MOV  DX,1
           INT  0x14
           MOV  B,AL
           MOV  S1,AH
         }
      printf("Prijaty znak: %2X ,  Status: %2X  ",B,S1);

      asm{
           MOV  AH,3
           MOV  DX,1
           INT  0x14
           MOV  S1,AL
         }
      printf(" Status: %2X\n\r",S1);


    }

    //_AL=random(255);
    //printf("Odesilani znaku... %X",_AL);
    //asm{
    //     MOV  AH,1
    //     MOV  DX,1
    //     INT  0x14
    //   }
    //printf("  Status: %2X\n\r",_AH);

    if(inp(0x60)==1) break;
  }
}



