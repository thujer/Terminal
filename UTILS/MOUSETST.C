#include <stdio.h>
#include <conio.h>


void main()
{
   asm{
         MOV  AX,0x24
         INT  0x33
      }
      //ÚÄÄÄÄÄÄÄÄÄÒÄÄÄÄÄÄÄÒÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
      //³ Vstup   º AX    º 0x0024  SubFn 24H - zjisti verzi software, typ my¨i
      //ÀÄÄÄÄÄÄÄÄÄ¶       º                     a ‡¡slo p©eru¨en¡              Ver 6.26+
      //ÚÄÄÄÄÄÄÄÄÄ×ÄÄÄÄÄÄÄ×ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
      //³ V˜stup  º AX    º FFFFH chyba, jinak
      //ÀÄÄÄÄÄÄÄÄÄ¶ BH    º hlavn¡ ‡¡slo verze
      //          º BL    º vedlej¨¡ ‡¡slo verze
      //          º CH    º typ (1=bus, 2=serial, 3=InPort, 4=PS/2, 5=HP)
      //          º CL    º p©eru¨en¡ (0=PS/2, 2=IRQ2, 3=IRQ3,...,7=IRQ7)
      //          ÓÄÄÄÄÄÄÄÐÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

   clrscr();
   printf("AX=%X, BH=%X, BL=%X,   ",_AX,_BH,_BL);

   while (!kbhit());
}