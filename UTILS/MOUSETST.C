#include <stdio.h>
#include <conio.h>


void main()
{
   asm{
         MOV  AX,0x24
         INT  0x33
      }
      //旼컴컴컴컴爐컴컴컴爐컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
      //� Vstup   � AX    � 0x0024  SubFn 24H - zjisti verzi software, typ my쮑
      //읕컴컴컴컴�       �                     a 눀slo p쯥ru쮍n�              Ver 6.26+
      //旼컴컴컴컴崙컴컴컴崙컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
      //� V쁲tup  � AX    � FFFFH chyba, jinak
      //읕컴컴컴컴� BH    � hlavn� 눀slo verze
      //          � BL    � vedlejÆ 눀slo verze
      //          � CH    � typ (1=bus, 2=serial, 3=InPort, 4=PS/2, 5=HP)
      //          � CL    � p쯥ru쮍n� (0=PS/2, 2=IRQ2, 3=IRQ3,...,7=IRQ7)
      //          聃컴컴컴懃컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

   clrscr();
   printf("AX=%X, BH=%X, BL=%X,   ",_AX,_BH,_BL);

   while (!kbhit());
}