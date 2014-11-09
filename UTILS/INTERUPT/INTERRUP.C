#include <stdio.h>
#include <dos.h>
#include <conio.h>

#ifdef __cplusplus
    #define __CPPARGS ...
#else
    #define __CPPARGS
#endif


unsigned int I;

void interrupt (*OldCOM1int)(__CPPARGS);
void interrupt (*OldCOM2int)(__CPPARGS);

void interrupt handler(__CPPARGS)
{
   //disable();

   I++;

   // Konec preruseni
   asm{
        MOV  AL,20h
        OUT  20h,AL
      }

   //enable();
   OldCOM1int();
}


#define IntCOM1 0x0C  // IRQ4
#define IntCOM2 0x0B  // IRQ3


void main()
{
   outp(0x3F9, 0xFF);
   outp(0x2F9, 0xFF);

   OldCOM1int = getvect(IntCOM1);
   OldCOM2int = getvect(IntCOM2);
   setvect(IntCOM2, handler);
   setvect(IntCOM1, handler);

   // Povoleni preruseni IRQ3 a IRQ4 ...
   asm{
        IN   AL,21h
        AND  AL,0FFh - (8 + 16)
        OUT  21h,AL
      }

   printf("Test preruseni... ESC=Exit\n\r");
   while(1)
   {
     printf("%2X, ",I);
     delay(500);
     if(inp(0x60)==1) break;
   }

   setvect(IntCOM1, OldCOM1int);
   setvect(IntCOM2, OldCOM2int);
}

