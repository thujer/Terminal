#include <stdio.h>

unsigned char Ascii,Scan;
unsigned char Index;
unsigned char IndexRing,IndexConnect,IndexOk;

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


void main()
{
  IndexRing=0;
  IndexConnect=0;

  while(1)
  {
    GetKey();

    switch(IndexRing)
    {
      case 0: if(Ascii=='R') IndexRing++; else IndexRing=0; break;
      case 1: if(Ascii=='I') IndexRing++;
              else if(Ascii!='R') IndexRing=0; break;
      case 2: if(Ascii=='N') IndexRing++; else IndexRing=0; break;
      case 3: if(Ascii=='G') IndexRing++; else IndexRing=0; break;
      case 4: if(Ascii==13) IndexRing++; else { IndexRing=0; break; }
              printf("Ring\n\r");
              IndexRing=0;
              break;
      default: IndexRing=0;
    }

    switch(IndexConnect)
    {
      case 0: if(Ascii=='C') IndexConnect++; else IndexConnect=0; break;
      case 1: if(Ascii=='O') IndexConnect++;
              else if(Ascii!='C')
	      IndexConnect=0;
	      break;
      case 2: if(Ascii=='N') IndexConnect++;
              else //if((Ascii!='C') & (Ascii!='O'))
	      IndexConnect=0;
	      break;
      case 3: if(Ascii=='N') IndexConnect++;
              else //if((Ascii!='C') & (Ascii!='O') & (Ascii!='N'))
	      IndexConnect=0;
	      break;
      case 4: if(Ascii=='E') IndexConnect++;
      case 5: if(Ascii=='C') IndexConnect++;
      case 6: if(Ascii=='T') IndexConnect++;
      case 7: if(Ascii==13)  IndexConnect++; else { IndexConnect=0; break; }
              printf("Connect\n\r");
              IndexConnect=0;
              break;
      default: IndexConnect=0;
    }

    printf("%d   %d\n",IndexRing,IndexConnect);
    if(Scan==1) break;
  }
}