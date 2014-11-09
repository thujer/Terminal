#include <stdio.h>
#include <conio.h>
#include <dos.h>


unsigned char Scan,Ascii;


void GetKey()
{
  unsigned char S,A;
  asm{
        MOV AH,0x10
        INT 0x16
        MOV A,AL
        MOV S,AH
      }
  Ascii=A;
  Scan=S;
}


//-----------------------------------------------------------------------
// Vykresli na obrazovku ramecek s rozmery x0, y0, x1, y1
// s attributy Attr
// Pokud je Shadow = True je vykreslovan stin ramecku
//------------------------------------------------------------------------
void Frame(char X0,char Y0,char X1,char Y1,char Attr,char Shadow)
{
  unsigned char x,y;

  textattr(Attr);

  for(y=((Y1-Y0)/2+Y0); y>=Y0; y--)
  for(x=X0; x<X1; x++)
  {
    gotoxy(x+1,y+1); putch(' ');
    gotoxy(x+1,Y1-y+1); putch(' ');
    delay(10);
  }

  gotoxy(X0,Y0);putch('Ú');
  for(x=0; x<(X1-X0); x++) putch('Ä');
  putch('¿');
  for(y=0; y<(Y1-Y0); y++)
  {
    textattr(Attr);
    gotoxy(X0,Y0+y+1);putch('³');
    gotoxy(X1+1,Y0+y+1);putch('³');
  }

  textattr(Attr);
  gotoxy(X0,Y1);putch('À');
  for(x=0; x<(X1-X0); x++) putch('Ä');
  putch('Ù');


  if(Shadow)
  {
    for(y=0; y<(Y1-Y0+1); y++)
    {
      textcolor(7);textbackground(0);
      gotoxy(X1+2,Y0+y+1);cprintf("°°");
    }
    textcolor(7);textbackground(0);
    gotoxy(X0+2,Y1+1);
    for(x=0; x<(X1-X0); x++) putch('°');
  }
}








/*
void Frame(char x0,char y0,char x1,char y1,char Attr,char Shadow)
{
  unsigned char x,y;

  textattr(Attr);
  gotoxy(x0,y0);putch('Ú');
  for(x=0;x<(x1-x0);x++) putch('Ä');
  putch('¿');
  for(y=0;y<(y1-y0);y++)
  {
    textattr(Attr);
    gotoxy(x0,y0+y+1);putch('³');
    gotoxy(x1+1,y0+y+1);putch('³');
  }

  textattr(Attr);
  gotoxy(x0,y1);putch('À');
  for(x=0;x<(x1-x0);x++) putch('Ä');
  putch('Ù');

  for(x=x0;x<x1;x++)
  for(y=y0;y<(y1-1);y++)
  {
    gotoxy(x+1,y+1);putch(' ');
  }

  if(Shadow)
  {
    for(y=0;y<(y1-y0+1);y++)
    {
      textcolor(7);textbackground(0);
      gotoxy(x1+2,y0+y+1);cprintf("°°");
    }
    textcolor(7);textbackground(0);
    gotoxy(x0+2,y1+1);
    for(x=0;x<(x1-x0);x++) putch('°');
  }
}

*/


void main()
{
 textattr(15);
 clrscr();
 delay(300);
 Frame(10, 10, 50, 20, 16+32+64,0);

 while(1)
 {
   GetKey();
   if(Scan==1) break;
 }
}