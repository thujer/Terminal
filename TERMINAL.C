// ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
// ³            ASCII a HEX Terminal pro seriovou komunikaci              ³
// ³                    (c) 2001/2002  Tomas Hujer                        ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

#define NazevProgramu     "COM Terminal"
#define VerzeProgramu     "2.53b"
#define Creator           "Tomas Hujer"
#define PosledniKompilace "22. 02. 2002"
#define ConfigFileHeader  "THS Terminal Config File "
#define Copyright         "Copyright (c) 2002"

#include <STDIO.H>
#include <ALLOC.H>
#include <CONIO.H>
#include <DOS.H>
#include <IO.H>
#include <STRING.H>
#include <FCNTL.H>
#include <STDLIB.H>


#define ScrBuffers 6    // Pocet bufferu pro obrazove operace


struct
{
  unsigned int Y;
  unsigned char *Item;
} MenuItems[20];


struct
{
  // Hlavicka konfiguracniho souboru
  unsigned char Header[25];
  unsigned char Ver[5];

  // Hlavni konfigurace
  unsigned char PocBit;
  unsigned char StpBit;
  unsigned char Parita;
  unsigned char Port;
  unsigned int  AdresaPortu;
  unsigned char Speed;
  unsigned char DisplayChars[12];
  unsigned char BitMask;
  int           Delay_Ascii;
  int           Delay_HEX;
  unsigned int  CustomPortAdress;
  unsigned char ExitDialog;
  unsigned char SetFixAdress;

  // Konfigurace pro protokol Distel
  unsigned int  DistelSendByteDelay;
  unsigned int  DistelSendMsgDelay;
  unsigned long DistelMaxAutoSend;
  unsigned int  DistelTimeoutMsgDetect;
  unsigned char DistelMaxRecMsgs;
  unsigned char DistelBeepAfterRecMsg;
  unsigned char DistelBeepAfterSendMsg;
  unsigned char DistelBeepTimeout;
  unsigned char DistelBeepType;
  unsigned char DistelAdrMin;
  unsigned char DistelAdrMax;
  unsigned char DistelMSGDetectionType;
  unsigned char DistelMSGSendMode;
  unsigned char DistelBytesForDetect;
  unsigned char DistelMaxMSGLength;
  unsigned char DistelMinMSGLength;
  unsigned char DistelCmdMin;
  unsigned char DistelCmdMax;
  unsigned char DistelAktualizeStatIfRec;

  // Ukladani bufferu
  unsigned char CustomASCIIBuffer[12][61];    // [Buffer][Znak]
  unsigned char CustomHEXBuffer[12][61];      // [Buffer][Znak]
  unsigned char CustomDistelBuffer[12][100];  // [Buffer][Znak]
  unsigned char DistelBufferPopis[12][20];    // [Buffer][Znak]
  unsigned char DistelMsgNumber[12];          // [Buffer][Znak]

  // Nastaveni pro Terminal 2xCOM
  unsigned char T2C_PORT1;        // Port1 pro terminal 2xCOM
  unsigned char T2C_PORT2;        // Port2 pro terminal 2xCOM
  unsigned char T2C_ENABLE_SEND;  // Povoleni odesilani znaku pro 2xCOM port1
  unsigned char T2C_TYPP1;        // Typ vypisu portu1 (ASCII/HEX)
  unsigned char T2C_TYPP2;        // Typ vypisu portu2 (ASCII/HEX)

  // Ostatni
  unsigned char Pol;              // Pozice v hlavnim menu
  unsigned char ActivatePol;      // bit 7 ... aktivace polozky
  unsigned char AutoOpen;         // 1=Povoleni automaticke aktivace

} Config;


// Vyber komunikacni rychlosti portu
unsigned long PortSpeed[12]={ 110, 150, 300, 600, 1200, 2400, 4800, 9600, 19200,
                             38400, 57600, 115200 };

unsigned int  I,TimeOut;
unsigned char Byte;

// Povoleni ukonceni programu
unsigned char Exit;


// Ukladani pozice kurzoru
unsigned char Cursor_X_0, Cursor_Y_0;
unsigned char Cursor_X_1, Cursor_Y_1;
unsigned char Cursor_X_2, Cursor_Y_2;
unsigned char Cursor_X_3, Cursor_Y_3;
unsigned char Cursor_X_4, Cursor_Y_4;

// Promenne pro zjisteni stisknute klavesy
unsigned int  Ascii,Scan;

// Pozice v jednotlivych menu
unsigned char UPol, SPol, SNPol, NPol, DCPol, STPPol, DistelPol, DSPol,
              DBLPol, BMPol, MPol, NT2CPol;

// Obrazovkove buffery
unsigned char far *ScrBuf[ScrBuffers];

// Promenne pro konverzi mezi HEX a DEC
unsigned char HEX[3];
unsigned char HEXWORD[4];

// Buffer pro radkovy vstup z klavesnice
unsigned char Vstup[256];

// Parametry spustitelneho souboru
unsigned char *Param;

// Rukojet oteviranych souboru
int handle;

// Promenna pro kontrolu CRC EXE souboru (terminal.exe)
unsigned long ActCRCtest;
unsigned long CRCtest;

// Zjisteni aktualniho stavu portu (chyba parity, spatny stopbit...)
unsigned char PortStatus;


unsigned char Text[300];

// Prepinani mezi editaci a obnovenim vypisu hex. pole
unsigned char Input_TAB_ENABLED;

// Stav zobrazeni napovedy klavesovych zkratek
unsigned char _KeybHelpStatus;

// Povoleni puvodni hodnoty radkoveho vstupu pri stisku ESC
unsigned char _CancelModeEnabled;

// Povoleni automatickeho odesilani znaku (Protokol Distel)
unsigned char _DistelAutoSend;

// Docasne pozastaveni prijimani znaku
unsigned char _PauseReceive;

// Promenna pro automaticke otevreni okna po spusteni
unsigned char AutoOpenStatus;



//------------------------------------------------------------------------
// Zjisti stisknutou klavesu, pokud neni klavesa stisknuta, ceka.
//------------------------------------------------------------------------
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


//------------------------------------------------------------------------
// Filtruje znaky podle aktualni konfigurace
//------------------------------------------------------------------------
unsigned char FilterChar(unsigned char Ch)
{
  unsigned char _Ch;

  _Ch=Ch;

  switch(_Ch)
  {
    case 0x0D: if(Config.DisplayChars[1]) _Ch=0; break;
    case 0x0A: if(Config.DisplayChars[2]) _Ch=0; break;
    case 0x07: if(Config.DisplayChars[3]) _Ch=0; break;
    case 0x08: if(Config.DisplayChars[4]) _Ch=0; break;
    case 0x09: if(Config.DisplayChars[5]) _Ch=0; break;
  }
  return(_Ch);
}


//------------------------------------------------------------------------
// Prevede maly znak na velky
//------------------------------------------------------------------------
unsigned char UpChr(unsigned char Ch)
{
  if((Ch>=65) & (Ch<=122)) return(Ch & (0xFF-0x20));
  else return(Ch);
}



//------------------------------------------------------------------------
// Zjisti z tabulky BIOSu adresu serioveho portu
// Pokud je vracena hodnota 0, port neni pritomen nebo chyba portu
//------------------------------------------------------------------------
unsigned int AdrPort(char P)
{
  if(!Config.SetFixAdress)
  {
    switch(P)
    {
      case 0: return(peek(0,0x0400));   // default 0x3F8;
      case 1: return(peek(0,0x0402));   // default 0x2F8;
      case 2: return(peek(0,0x0404));   // default 0x3E8;
      case 3: return(peek(0,0x0406));   // default 0x2E8;
      case 4: return(Config.CustomPortAdress);
      default: return(0);
    }
  }
  else // Pokud toto zjisteni adresy portu nepracuje spravne
  {    // dosadi se adresy portu napevno:
    switch(P)
    {
      case 0: return(0x3F8);
      case 1: return(0x2F8);
      case 2: return(0x3E8);
      case 3: return(0x2E8);
      case 4: return(Config.CustomPortAdress);
      default: return(0);
    }
  }
}


//-----------------------------------------------------------------------
// Ulozi do bufferu Buf vyrez obrazovky dany rozmery x0, y0, x1, y1
//------------------------------------------------------------------------
void GetScr(unsigned char Buf,char x0,char y0,char x1,char y1)
{
  unsigned char x,y;

  if(ScrBuf[Buf]!=NULL)
  {
    for (y=y0;y<=y1;y++)
    for (x=x0;x<=x1;x++)
     { ScrBuf[Buf][x*2 + y*80*2]=peekb(0xB800,x*2 + y*80*2);
       ScrBuf[Buf][x*2 + y*80*2 + 1]=peekb(0xB800,x*2 + y*80*2 + 1); }
  }
  else
  {
    gotoxy(1,1); textattr(15);
    cprintf("\nChyba: Nedostatek pameti pro GetScr! Buffer: %2x",Buf);
  }
}



//-----------------------------------------------------------------------
// Obnovi z bufferu Buf vyrez obrazovky dany rozmery x0, y0, x1, y1
//------------------------------------------------------------------------
void SetScr(unsigned char Buf,char x0,char y0,char x1,char y1)
{
  unsigned char x,y;

  if(ScrBuf[Buf]!=NULL)
  {
    for (y=y0;y<=y1;y++)
    for (x=x0;x<=x1;x++)
     { pokeb(0xB800,x*2+y*80*2,ScrBuf[Buf][x*2 + y*80*2]);
       pokeb(0xB800,x*2+y*80*2 + 1,ScrBuf[Buf][x*2 + y*80*2 + 1]); }
  }
  else
  {
    gotoxy(1,1); textattr(15);
    cprintf("\nChyba: Nedostatek pameti pro SetScr! Buffer: %2x",Buf);
  }
}



//-----------------------------------------------------------------------
// Vykresli na obrazovku ramecek s rozmery x0, y0, x1, y1
// s attributy Attr
// Pokud je Shadow = True je vykreslovan stin ramecku
//------------------------------------------------------------------------
void Frame(char x0,char y0,char x1,char y1,char Attr,char Shadow)
{
  unsigned char x,y;

  textattr(Attr);

  for(x=x0;x<x1;x++)
  for(y=y0;y<(y1-1);y++)
  {
    gotoxy(x+1,y+1);putch(' ');
  }

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



//------------------------------------------------------------------------
// Zapise do promennych Cursor_X[B] a Cursor_Y[B] aktualni pozici kurzoru
//------------------------------------------------------------------------
void SaveCursorPos(unsigned char B)
{
  unsigned char _x,_y;
  asm{
       MOV  AH,3
       MOV  BH,0
       INT  0x10
       MOV  _x,DL
       MOV  _y,DH
                         }
  switch(B)
  {
    case 0: Cursor_X_0=_x; Cursor_Y_0=_y; break;
    case 1: Cursor_X_1=_x; Cursor_Y_1=_y; break;
    case 2: Cursor_X_2=_x; Cursor_Y_2=_y; break;
    case 3: Cursor_X_3=_x; Cursor_Y_3=_y; break;
    case 4: Cursor_X_4=_x; Cursor_Y_4=_y; break;
    default: printf("Chybne cislo bufferu pro ulozeni pozice kurzoru !");
  }
}



//------------------------------------------------------------------------
// Nastavi kurzor na pozici zapsanou v promennych Cursor_X_? a Cursor_Y_?
//------------------------------------------------------------------------
void LoadCursorPos(unsigned char B)
{
  unsigned char _x,_y;

  switch(B)
  {
    case 0: _x=Cursor_X_0; _y=Cursor_Y_0; break;
    case 1: _x=Cursor_X_1; _y=Cursor_Y_1; break;
    case 2: _x=Cursor_X_2; _y=Cursor_Y_2; break;
    case 3: _x=Cursor_X_3; _y=Cursor_Y_3; break;
    case 4: _x=Cursor_X_4; _y=Cursor_Y_4; break;
    default: printf("Chybne cislo bufferu pro obnoveni pozice kurzoru !");
  }

  asm{
       MOV  AH,2
       MOV  BH,0
       MOV  DL,_x
       MOV  DH,_y
       INT  0x10
                     }
}



//------------------------------------------------------------------------
// Roluje okno _X0, _Y0, _X1, _Y1 o jeden radek nahoru
// pradne misto ma atribut dany BH
//------------------------------------------------------------------------
void RolWindowUP(unsigned char _X0,unsigned char _Y0,unsigned char _X1,unsigned char _Y1)
{
   asm{
        MOV  AH,6
        MOV  CH,_Y0
        MOV  CL,_X0
        MOV  DH,_Y1
        MOV  DL,_X1
        MOV  AL,1
        MOV  BH,15
        INT  0x10
                  }
}



//-----------------------------------------------------------------------
// Nastavi neviditelny textovy kurzor
//------------------------------------------------------------------------
void HideTextCursor()
{
  asm{
  MOV AH,1
  MOV CH,20h
  MOV CL,20h
  INT 10h }
}


//-----------------------------------------------------------------------
// Nastavi viditelny textovy kurzor
//------------------------------------------------------------------------
void ShowTextCursor()
{
  asm{
  MOV AH,1
  MOV CH,15
  MOV CL,16
  INT 10h }
}



//------------------------------------------------------------------------
// Zobrazuje malou napovedu ( na radku 24 )
//------------------------------------------------------------------------
void ShowHint(unsigned char *Hint)
{
   textattr(10+16);
   window(1,24,80,24); clrscr(); window(1,1,80,25);
   gotoxy(1,24);
   if(Hint[0]!=0) cprintf(" %s",Hint);
}



//------------------------------------------------------------------------
//  Zobrazi text T uprostred radku Y
//------------------------------------------------------------------------
void CenterText(unsigned char Y, unsigned char *T)
{
  unsigned char X;

  X=80/2 - strlen(T)/2;

  gotoxy(X,Y);
  printf(T);
}



//------------------------------------------------------------------------
// Informace o programu
//------------------------------------------------------------------------
void About()
{
  HideTextCursor();
  Frame(25,9,55,17,64+32+16,0);
  CenterText(9," About ");
  CenterText(11,NazevProgramu" v"VerzeProgramu);
  CenterText(13,Copyright);
  CenterText(15,Creator);
  GetKey();
}



//------------------------------------------------------------------------
//  Ukonceni programu a vypis informaci o programu
//------------------------------------------------------------------------
void ExitAbout(unsigned char ExitCode)
{
  textattr(7);
  clrscr();
  CenterText(2,NazevProgramu" v"VerzeProgramu);
  CenterText(3,Copyright"  "Creator"\n\n");
  printf("Posledni upgrade: %s\n\n\r",PosledniKompilace);
  exit(ExitCode);
}




void SetDistelDefaultConfig()
{
  Config.DistelSendByteDelay=0;
  Config.DistelSendMsgDelay=0;
  Config.DistelMaxAutoSend=0;
  Config.DistelMaxRecMsgs=13;
  Config.DistelTimeoutMsgDetect=50;
  Config.DistelBeepAfterRecMsg=0;
  Config.DistelBeepAfterSendMsg=0;
  Config.DistelBeepTimeout=0;
  Config.DistelBeepType=1;
  Config.DistelAdrMin=0;
  Config.DistelAdrMax=255;
  Config.DistelMSGDetectionType=3;
  Config.DistelMSGSendMode=0;
  Config.DistelBytesForDetect=50;
  Config.DistelMaxMSGLength=255;
  Config.DistelMinMSGLength=0;
  Config.DistelCmdMin=0;
  Config.DistelCmdMax=255;
  Config.DistelAktualizeStatIfRec=0;
  Config.PocBit=8;             // Pocet prenasenych bitu
  Config.StpBit=0;             // Pocet stopbitu
  Config.Parita=0;             // None
  Config.Port=0;               // COM1
  Config.AdresaPortu=0x3F8;    // Adresa portu
  Config.Speed=8;              // 19200 Bd
  Config.DisplayChars[0]=0;    // Povoleni prichozim znakum zmenu kurzoru
  Config.DisplayChars[1]=0;    // 0x0D  CR
  Config.DisplayChars[2]=0;    // 0x0A  LF
  Config.DisplayChars[3]=0;    // 0x07  BELL
  Config.DisplayChars[4]=0;    // 0x08  BCK
  Config.DisplayChars[5]=0;    // 0x09  TAB
  Config.BitMask=255;          // Bitova maska (Zobrazovane bity)
  Config.SetFixAdress=0;       // Nastaveni default adres portu

  // Ukoncovaci dialog ( pri stisku Alt+X )
  Config.ExitDialog=1;

  // -------- Zpozdeni ----------
  Config.Delay_Ascii=1;        // Pro vysilani ASCII bufferu
  Config.Delay_HEX=1;          // Pro vysilani HEX bufferu

  // Nastaveni terminalu 2xCOM
  Config.T2C_PORT1=0;        // Port1 pro terminal 2xCOM
  Config.T2C_PORT2=1;        // Port2 pro terminal 2xCOM
  Config.T2C_ENABLE_SEND=0;  // Povoleni odesilani znaku pro 2xCOM port1
  Config.T2C_TYPP1=0;        // Typ vypisu portu1 (ASCII/HEX)
  Config.T2C_TYPP2=0;        // Typ vypisu portu2 (ASCII/HEX)

  // Nastaveni automaticke aktivace polozky
  Config.Pol=0;
  Config.ActivatePol=0;
  Config.AutoOpen=0;
}





//------------------------------------------------------------------------
// Nacteni konfigurace ze souboru

// Pokud soubor neexistuje, pokusi se ho zalozit, jestize neuspeje,
// DOS vyhlasi chybu

// Uklada se cela struktura Config, lze tedy jednoduse pridat dalsi
// promenne k ukladani do souboru
//------------------------------------------------------------------------
void ReadConfigFile()
{
  unsigned Attr;

  if ((handle = _open("TERMINAL.CFG", O_RDONLY )) == -1)
  {
     handle = _creat("TERMINAL.CFG", 0);

     for(I=0; I<12; I++) Config.DistelMsgNumber[I]=0;

     SetDistelDefaultConfig();

     clrscr();
     About();
  }
  else
  {
    printf("Nacita se konfigurace .... ");

    lseek(handle, 0L, SEEK_SET);
    if(_read(handle, &Config, sizeof(Config))==-1)
    {
      printf("Chyba pri cteni souboru s konfiguraci !");
      while(1)
      {
        GetKey();
        if(Scan==1) ExitAbout(0xFF);
        if((Ascii==13) | (Scan==57)) return;
      }
    }
    else printf("Ok\n");
    _dos_getfileattr("TERMINAL.CFG",&Attr);
    if(Attr & 1==1) About();

    if((Config.Ver[0]!=VerzeProgramu[0]) | (Config.Ver[1]!=VerzeProgramu[1])|
       (Config.Ver[2]!=VerzeProgramu[2]) | (Config.Ver[3]!=VerzeProgramu[3])|
       (Config.Ver[4]!=VerzeProgramu[4]))
    {
      printf("\n\n\n\rUpozorneni: Konfiguracni soubor byl ulozen jinou verzi programu !\n\r");
      printf("Nastaveni programu nemusi byt korektni... ");
      GetKey();
      printf("\n\n\r");
    }
  }
  _close(handle);

  if((Config.PocBit>8) | (Config.PocBit<6)) Config.PocBit=8;
  if(Config.StpBit>1) Config.StpBit=0;
  if(Config.Parita>4) Config.Parita=0;
  if(Config.Port>4) Config.Port=0;
  if(Config.Speed>12) Config.Speed=12;
  for(I=0; I<12; I++)
  {
    if(Config.DistelMsgNumber[I] > 12) Config.DistelMsgNumber[I]=0;
  }

  if((Config.Pol > 6) & (Config.Pol != 10)) Config.Pol=0;
}





//------------------------------------------------------------------------
// Zapis konfigurace do souboru
//------------------------------------------------------------------------
void WriteConfigFile()
{
  if ((handle = _open("TERMINAL.CFG", O_WRONLY)) == -1)
  {
    SaveCursorPos(3);
    HideTextCursor();
    window(1,1,80,25);
    ShowHint("Nelze vytvorit soubor TERMINAL.CFG pro zapis konfigurace !");
    GetScr(2,17,11,67,14);
    Frame(18,12,65,14,64+15,0);
    gotoxy(38,12); cprintf(" Chyba ");
    gotoxy(22,13); cprintf("Nelze zapsat konfiguraci do souboru !",Config.AdresaPortu);
    GetKey();
    SetScr(2,17,11,67,14);
    ShowTextCursor();
    LoadCursorPos(3);
    textattr(15);
  }
  else
  {
    memcpy(Config.Header, ConfigFileHeader, 30);
    Config.Ver[0]=VerzeProgramu[0];
    Config.Ver[1]=VerzeProgramu[1];
    Config.Ver[2]=VerzeProgramu[2];
    Config.Ver[3]=VerzeProgramu[3];
    Config.Ver[4]=VerzeProgramu[4];
    lseek(handle, 0L, SEEK_SET);
    _write(handle, &Config, sizeof(Config));
  }
  _close(handle);
}




//------------------------------------------------------------------------
// Prerusitelne zpozdeni
//------------------------------------------------------------------------
unsigned char Delay(unsigned int D)
{
  unsigned int I;
  for(I=0; I<D; I++)
  {
    delay(1);
    if(inp(0x60)==1)
    {
      getch();
      return(1);
    }
  }
  return(0);
}


//------------------------------------------------------------------------
// Nastavi prenosovou rychlost aktualniho portu
//------------------------------------------------------------------------
void NastavRychlostPortu(unsigned long Baud)
{
  unsigned char D0,D1;

  D0=(unsigned char) ((115200 / Baud) & 0xFF);        // Dolni byte
  D1=(unsigned char) (((115200 / Baud) >> 8) & 0xFF); // Horni byte

  outp(Config.AdresaPortu+3,128);   // Nastavi Zapis do reg. Adr+1
  outp(Config.AdresaPortu  ,D0);    // Zapis dolniho byte delitele ryhlosti
  outp(Config.AdresaPortu+1,D1);    // Zapis horniho byte delitele ryhlosti
  outp(Config.AdresaPortu+3,3);
};




//------------------------------------------------------------------------
// Inicializuje aktualni port
// Pokud port nelze inicializovat, vyhlasi chybu
//------------------------------------------------------------------------
void InitPort()
{
  unsigned char PortSet;

  if(Config.AdresaPortu==0)
  {
    GetScr(2,17,11,67,14);
    Frame(18,12,67,14,64+15,0);
    gotoxy(38,12); cprintf(" Chyba ");
    gotoxy(20,13); cprintf("Port COM%x na adrese %.4Xh nelze inicializovat !",Config.Port+1,Config.AdresaPortu);
    while(1)
    {
      GetKey(); if((Scan==1) | (Scan==28)) break;
    }
    SetScr(2,17,11,67,14);
  }
  else
  {
    NastavRychlostPortu(PortSpeed[Config.Speed]);

    PortSet=0;

    switch(Config.PocBit)
    {
      case 5: break;              // 5 bitu
      case 6: PortSet|=1; break;  // 6 bitu
      case 7: PortSet|=2; break;  // 7 bitu
      case 8: PortSet|=3; break;  // 8 bitu
      default: break;
    }

    if(Config.StpBit==1) PortSet+=4;

    //cprintf("XXX%X",Config.Parita);

    switch(Config.Parita)
    {
      case 0: break;                         //Neni
      case 1: PortSet|=8+16; break;          //Suda
      case 2: PortSet|=8+16; break;          //Licha
      case 3: PortSet|=8+32; break;          //Vzdy 0
      case 4: PortSet|=8+16+32; break;       //Vzdy 1
      default: break;
    }

    outp(Config.AdresaPortu+3,PortSet);

    outp(Config.AdresaPortu+1,0);    // Vypnuti preruseni
    outp(Config.AdresaPortu+4,1);    // Aktivace DTR
  }
}



//------------------------------------------------------------------------
// Vraci 1 pokud je port pripraven vyslat dalsi znak
//------------------------------------------------------------------------
unsigned char SendReady()     // 0 = Vysilac je prazdny a pripraven vysilat
{
  if((inp(Config.AdresaPortu+5)&(32+64))==(32+64)) return(1);
  else return(0);
}



//------------------------------------------------------------------------
// Vraci 1 pokud port prijal dalsi znak
//------------------------------------------------------------------------
unsigned char CharReady()     // 1 = Znak byl prijat
{
  if((inp(Config.AdresaPortu+5) & 1)==1) return(1);
  else return(0);
}



//------------------------------------------------------------------------
// Vraci prijaty znak, pokud je prijimac prazdny ceka na dalsi znak
//------------------------------------------------------------------------
unsigned char PrijatyByte()
{
  while(1)
  {
   if(CharReady()) return(inp(Config.AdresaPortu));
   if(inp(0x60)==1) return(0xFF);
  }
}



//-----------------------------------------------------------------------
//Prevede HEX. cislo z ASCII formy do byte
//------------------------------------------------------------------------
unsigned char CharToHex()
{
  unsigned char B;

  B=0;
  switch(HEX[1])
  {
    case '0': B=0; break;
    case '1': B=1; break;
    case '2': B=2; break;
    case '3': B=3; break;
    case '4': B=4; break;
    case '5': B=5; break;
    case '6': B=6; break;
    case '7': B=7; break;
    case '8': B=8; break;
    case '9': B=9; break;
    case 'A': B=10; break;
    case 'B': B=11; break;
    case 'C': B=12; break;
    case 'D': B=13; break;
    case 'E': B=14; break;
    case 'F': B=15; break;
  }

  switch(HEX[0])
  {
    case '0': B=B+0x00; break;
    case '1': B=B+0x10; break;
    case '2': B=B+0x20; break;
    case '3': B=B+0x30; break;
    case '4': B=B+0x40; break;
    case '5': B=B+0x50; break;
    case '6': B=B+0x60; break;
    case '7': B=B+0x70; break;
    case '8': B=B+0x80; break;
    case '9': B=B+0x90; break;
    case 'A': B=B+0xA0; break;
    case 'B': B=B+0xB0; break;
    case 'C': B=B+0xC0; break;
    case 'D': B=B+0xD0; break;
    case 'E': B=B+0xE0; break;
    case 'F': B=B+0xF0; break;
  }

  return(B);
}


//-----------------------------------------------------------------------
//Prevede HEX. cislo z byte do ASCII formy
//------------------------------------------------------------------------
void HexToChar(unsigned char B)
{

  switch(B >> 4)
  {
    case  0: HEX[0]='0'; break;
    case  1: HEX[0]='1'; break;
    case  2: HEX[0]='2'; break;
    case  3: HEX[0]='3'; break;
    case  4: HEX[0]='4'; break;
    case  5: HEX[0]='5'; break;
    case  6: HEX[0]='6'; break;
    case  7: HEX[0]='7'; break;
    case  8: HEX[0]='8'; break;
    case  9: HEX[0]='9'; break;
    case 10: HEX[0]='A'; break;
    case 11: HEX[0]='B'; break;
    case 12: HEX[0]='C'; break;
    case 13: HEX[0]='D'; break;
    case 14: HEX[0]='E'; break;
    case 15: HEX[0]='F'; break;
  }

  switch(B & 15)
  {
    case  0: HEX[1]='0'; break;
    case  1: HEX[1]='1'; break;
    case  2: HEX[1]='2'; break;
    case  3: HEX[1]='3'; break;
    case  4: HEX[1]='4'; break;
    case  5: HEX[1]='5'; break;
    case  6: HEX[1]='6'; break;
    case  7: HEX[1]='7'; break;
    case  8: HEX[1]='8'; break;
    case  9: HEX[1]='9'; break;
    case 10: HEX[1]='A'; break;
    case 11: HEX[1]='B'; break;
    case 12: HEX[1]='C'; break;
    case 13: HEX[1]='D'; break;
    case 14: HEX[1]='E'; break;
    case 15: HEX[1]='F'; break;
  }
}



//-----------------------------------------------------------------------
// Vyhradi pamet pro pouzivani funkci GetScr, SetScr, GetScreen, SetScreen
// Pokud neni pozadovana pamet v dispozici, vyhlasi chybu
//------------------------------------------------------------------------
void InitGSVideo()
{
  unsigned char B,SBError;

  printf("Probiha inicializace obrazovych bufferu : \n");

  SBError=0;
  for(B=0; B<ScrBuffers; B++)
  {
    printf("  Buffer[%X] ... ",B);
    ScrBuf[B]=NULL;
    ScrBuf[B]=(char far *) farmalloc(4096);
    if(ScrBuf!=NULL) printf("Ok\n");
    else { printf("Error\n"); SBError=1; }
  }
  if(SBError)
  {
    printf("\n Nedostatek pameti pro inicializaci vsech bufferu !\n");
    printf(" Nektere textove operace nemusi byt korektni, chcete Pokracovat?\n");
    printf("                       ( ESC = exit / ENTER )");
    while(1)
    {
      GetKey();
      if(Scan==1) ExitAbout(0xFF);
      if((Ascii==13) | (Scan==57)) return;
    }
  }
  else printf("\n\n");
}



//-----------------------------------------------------------------------
// Uvolni pamet alokovanou procedurou InitGSVideo()
//------------------------------------------------------------------------
void DoneGSVideo()
{
  char I;

  for(I=0;I<3;I++)
  {
    if(ScrBuf[I]!=NULL) farfree(ScrBuf[I]);
    else
    {
      printf("\nPred pouzitim procedury DoneGSVideo musi byt pouzita procedura InitGSVideo !\n");
      printf("\nChyba pri dealokaci bufferu %x\n",I);
    }
  }
}




//-----------------------------------------------------------------------
//  Vykresleni zakladni obrazovky
//------------------------------------------------------------------------
void InitScreen()
{
  textmode(3);
  textcolor(7);textbackground(0);
  window(1,1,80,25); clrscr();
  textcolor(1);textbackground(7);
  gotoxy(1,2);
  for(I=0;I<1840;I++) putch('°');
  textbackground(7);
  window(1,1,80,1); clrscr();
  window(1,1,80,25);
  textcolor(0);
  CenterText(1,NazevProgramu);
  gotoxy(1,2);
}



//------------------------------------------------------------------------
// Pokousi se odeslat znak, pri preteceni TimeOut vrati 0xFF
//------------------------------------------------------------------------
unsigned char VysliByte(char B)
{
  unsigned int TimeOut;

  TimeOut=4000;  // cca 4s
  while(1)
  {
    if(SendReady())
    {
      outp(Config.AdresaPortu,B);
      return(0);
    }
    else delay(1);
    TimeOut--;

    if(TimeOut==0) return(0xFF);

    if(kbhit())
    {
      GetKey();
      if((Scan==1) | ((Scan==45)&(Ascii=0)) | (Scan==28)) return(1);
    }
  }
}




//------------------------------------------------------------------------
// Vymaze buffer pro polozky menu
//------------------------------------------------------------------------
void ResetMenuItems()
{
  for(I=0;I<20;I++)
  {
    MenuItems[I].Y=0; MenuItems[I].Item="                   ";
  }
}




//------------------------------------------------------------------------
// Prekresleni hlavniho menu
//------------------------------------------------------------------------
void RefreshMainMenu()
{
  #define MainMenuItems 15

  ResetMenuItems();
  MenuItems[ 0].Y=3;  MenuItems[ 0].Item=" ASCII  \0x0";
  MenuItems[ 1].Y=4;  MenuItems[ 1].Item="I/O ASC \0x0";
  MenuItems[ 2].Y=5;  MenuItems[ 2].Item="  HEX   \0x0";
  MenuItems[ 3].Y=6;  MenuItems[ 3].Item="I/O HEX \0x0";
  MenuItems[ 4].Y=7;  MenuItems[ 4].Item="ASC/HEX \0x0";
  MenuItems[ 5].Y=8;  MenuItems[ 5].Item="HEX/ASC \0x0";
  MenuItems[ 6].Y=9;  MenuItems[ 6].Item=" 2xCOM  \0x0";
  MenuItems[ 7].Y=11; MenuItems[ 7].Item=" Set   \0x0";
  MenuItems[ 8].Y=12; MenuItems[ 8].Item="Utils  \0x0";
  MenuItems[ 9].Y=14; MenuItems[ 9].Item="Distel  \0x0";
  MenuItems[10].Y=16; MenuItems[10].Item="Follow  \0x0";
  MenuItems[11].Y=22; MenuItems[11].Item="Exit    \0x0";
  MenuItems[12].Y=10; MenuItems[12].Item[0]=0;
  MenuItems[13].Y=13; MenuItems[13].Item[0]=0;
  MenuItems[14].Y=21; MenuItems[14].Item[0]=0;

  for(I=0; I<=MainMenuItems-1; I++)
  {
    if(Config.Pol==I) textattr(15); else textattr(7+16);
    if(MenuItems[I].Item[0]!=0)
    {  gotoxy(2,MenuItems[I].Y); cprintf(" %s",MenuItems[I].Item); }
    else
    {  gotoxy(1,MenuItems[I].Y); printf("ÃÄÄÄÄÄÄÄÄÄ´"); }
  }
}



//------------------------------------------------------------------------
// Prekresleni menu Utility
//------------------------------------------------------------------------
void RefreshUtilsMenu()
{
  #define UtilsMenuItems 4
  ResetMenuItems();
  MenuItems[ 0].Y=11; MenuItems[ 0].Item="Vysilat nepretrzite   \0x0";
  MenuItems[ 1].Y=12; MenuItems[ 1].Item="Zobrazuj nepretrzite   \0x0";
  MenuItems[ 2].Y=13; MenuItems[ 2].Item="Zobrazuj prijate znaky \0x0";
  MenuItems[ 3].Y=14; MenuItems[ 3].Item="ECHO mode (COM1,COM2)  \0x0";

  for(I=0; I<UtilsMenuItems; I++)
  {
    if(UPol==I) textattr(15); else textattr(7+16);
    if(MenuItems[I].Item[0]!=0)
    { gotoxy(13,MenuItems[I].Y); cprintf(" %s",MenuItems[I].Item); }
  }
}


//------------------------------------------------------------------------
// Prekresleni menu Modem
//------------------------------------------------------------------------
void RefreshModemMenu()
{
  #define ModemMenuItems 2
  ResetMenuItems();
  MenuItems[ 0].Y=15; MenuItems[ 0].Item="Detekce modemu \0x0";
  MenuItems[ 1].Y=16; MenuItems[ 1].Item="Reset modemu   \0x0";

  for(I=0; I<ModemMenuItems; I++)
  {
    if(MPol==I) textattr(15); else textattr(7+16);
    if(MenuItems[I].Item[0]!=0)
    { gotoxy(13,MenuItems[I].Y); cprintf(" %s",MenuItems[I].Item); }
  }
}


//------------------------------------------------------------------------
// Prekresleni menu Nastaveni
//------------------------------------------------------------------------
void RefreshSetMenu()
{
  #define SetMenuItems 9
  ResetMenuItems();
  MenuItems[ 0].Y=10; MenuItems[ 0].Item="Port\0x0";
  MenuItems[ 1].Y=11; MenuItems[ 1].Item="Rychlost portu     \0x0";
  MenuItems[ 2].Y=12; MenuItems[ 2].Item="Nastaveni portu    \0x0";
  MenuItems[ 3].Y=13; MenuItems[ 3].Item="Funkce Ascii       \0x0";
  MenuItems[ 4].Y=14; MenuItems[ 4].Item="Bitova maska       \0x0";
  MenuItems[ 5].Y=15; MenuItems[ 5].Item="Delay pro ASCII    \0x0";
  MenuItems[ 6].Y=16; MenuItems[ 6].Item="Delay pro HEX      \0x0";
  MenuItems[ 7].Y=17; MenuItems[ 7].Item="Adresy portu       \0x0";
  MenuItems[ 8].Y=18; MenuItems[ 8].Item="Otevrit po startu  \0x0";
  MenuItems[ 9].Y=19; MenuItems[ 9].Item="Ukoncovaci dialog  \0x0";

  for(I=0; I<=SetMenuItems; I++)
  {
    if(SPol==I) textattr(15); else textattr(7+16);
    if(MenuItems[I].Item[0]!=0)
    {
      gotoxy(13,MenuItems[I].Y); cprintf(" %s",MenuItems[I].Item);

      if(I==0)
      {
        cprintf("    (%.4Xh)    ",Config.AdresaPortu);
      }
    }
  }
}



//------------------------------------------------------------------------
//  Vykresleni dolniho menu (klavesove zkratky)
//------------------------------------------------------------------------
void InitDownMenu(char S[])
{
  unsigned char C;

  textattr(16+32+64);
  window(1,25,80,25); clrscr();
  I=0;C=0;
  gotoxy(2,1);
  while(S[I]!=0)
  {
    if(S[I]=='~') { C=!C; I++; continue; }
    if(C) textcolor(4);  else textcolor(0);
    putch(S[I]);
    I++;
  }
  window(1,1,80,25);
}



//------------------------------------------------------------------------
// Zobrazi na dolnim ramu terminaloveho okna aktualni nastaveni portu
//------------------------------------------------------------------------
void ShowPortSet()
{
  gotoxy(30,23); cprintf(" %d0 Bd ", PortSpeed[Config.Speed] / 10);
  gotoxy(50,23); cprintf(" %d", Config.PocBit);
  switch(Config.Parita)
  {
    case 0: cprintf(" N "); break;
    case 1: cprintf(" S "); break;
    case 2: cprintf(" L "); break;
    case 3: cprintf(" V0 "); break;
    case 4: cprintf(" V1 "); break;
  }
  cprintf("%d ",Config.StpBit+1);
  gotoxy(63,23); cprintf(" COM%X: %3Xh ",Config.Port+1,Config.AdresaPortu);
  if(Config.SetFixAdress)
  { gotoxy(77,23); printf("F"); }     // Pri fixnich adresach portu
}



//------------------------------------------------------------------------
// Vyber rychlosti komunikacniho portu
//------------------------------------------------------------------------
void SelectSpeed()
{
  GetScr(1,33,9,44,23);
  Frame(35,10,44,23,16+7,0);
  gotoxy(37,10); cprintf(" [Bd] ");
  gotoxy(34,11); cprintf("Æµ");

  InitDownMenu("~Alt-X~ Exit  ~/~ Zmenit  ~ENTER/ESC~ Ok");

  ResetMenuItems();
  MenuItems[ 0].Y=11; MenuItems[ 0].Item="   110 \0x0";
  MenuItems[ 1].Y=12; MenuItems[ 1].Item="   150 \0x0";
  MenuItems[ 2].Y=13; MenuItems[ 2].Item="   300 \0x0";
  MenuItems[ 3].Y=14; MenuItems[ 3].Item="   600 \0x0";
  MenuItems[ 4].Y=15; MenuItems[ 4].Item="  1200 \0x0";
  MenuItems[ 5].Y=16; MenuItems[ 5].Item="  2400 \0x0";
  MenuItems[ 6].Y=17; MenuItems[ 6].Item="  4800 \0x0";
  MenuItems[ 7].Y=18; MenuItems[ 7].Item="  9600 \0x0";
  MenuItems[ 8].Y=19; MenuItems[ 8].Item=" 19200 \0x0";
  MenuItems[ 9].Y=20; MenuItems[ 9].Item=" 38400 \0x0";
  MenuItems[10].Y=21; MenuItems[10].Item=" 57600 \0x0";
  MenuItems[11].Y=22; MenuItems[11].Item="115225 \0x0";

  while(1)
  {
     for(I=0;I<12;I++)
     {
       if(Config.Speed==I) textattr(15); else textattr(7+16);
       if(MenuItems[I].Item[0]!=0)
       { gotoxy(36,MenuItems[I].Y); cprintf(" %s",MenuItems[I].Item); }
     }

     GetKey();
     if((Scan==72) & (Config.Speed>0)) Config.Speed--;
     if((Scan==80) & (Config.Speed<11)) Config.Speed++;
     if(Scan==71) Config.Speed=0;
     if(Scan==79) Config.Speed=11;
     if((Ascii==0) & (Scan==45)) { Exit=1; break; }

     if((Scan==75) | (Scan==1) | (Scan==28)) { Scan=0; break; }
  }
  SetScr(1,33,9,44,23);
  window(1,1,80,25);
}


// Preddefinovano
void InputHEXLine( int x, int y, int Size, char Atr, char *DefaultText);


void HexWordToText(unsigned int W)
{
  HEXWORD[3]=(W & 0x0F);
  if(HEXWORD[3] <= 9) HEXWORD[3]+='0'; else HEXWORD[3]+='A'-10;

  HEXWORD[2]=((W >> 4) & 0x0F);
  if(HEXWORD[2] <= 9) HEXWORD[2]+='0'; else HEXWORD[2]+='A'-10;

  HEXWORD[1]=((W >> 8) & 0x0F);
  if(HEXWORD[1] <= 9) HEXWORD[1]+='0'; else HEXWORD[1]+='A'-10;

  HEXWORD[0]=((W >> 12) & 0x0F);
  if(HEXWORD[0] <= 9) HEXWORD[0]+='0'; else HEXWORD[0]+='A'-10;
}



unsigned int TextToHexWord()
{
  unsigned int W;
  unsigned char I;

  if(HEXWORD[3] <= '9') W= (HEXWORD[3] - '0') * 1;
                   else W= (HEXWORD[3] - 'A' + 10) * 1;

  if(HEXWORD[2] <= '9') W+=(HEXWORD[2] - '0') * 0x10;
                   else W+=(HEXWORD[2] - 'A' + 10) * 0x10;

  if(HEXWORD[1] <= '9') W+=(HEXWORD[1] - '0') * 0x100;
                   else W+=(HEXWORD[1] - 'A' + 10) * 0x100;

  if(HEXWORD[0] <= '9') W+=(HEXWORD[0] - '0') * 0x1000;
                   else W+=(HEXWORD[0] - 'A' + 10) * 0x1000;

  return(W);
}



unsigned int InputWordHEXLine(int x, int y, char Atr, unsigned int Default)
{
  unsigned char Pos;
  textattr(Atr);
  Pos=0;
  for(I=0; I<4; I++) HEXWORD[I]=0;
  HexWordToText(Default);
  ShowTextCursor();

  while(1)
  {
    gotoxy(x-1,y+1);
    for(I=0; I<4; I++) cprintf("%c",HEXWORD[I]);
    cprintf("h");
    gotoxy(x-1+Pos,y+1);

    GetKey();
    Ascii=UpChr(Ascii);
    if(((Ascii>='0') & (Ascii<='9')) | ((Ascii>='A') & (Ascii<='F')))
    {
      HEXWORD[Pos]=Ascii;
      if(Pos<3) Pos++;
    }
    else
    switch(Scan)
    {
      case 1:
      case 28: break;
      case 14: HEXWORD[Pos]='0'; if(Pos) Pos--; break;
      case 75: if(Pos) Pos--;   break;
      case 77: if(Pos<3) Pos++; break;
    }

    if((Scan==1) | (Scan==28)) { Scan=0; break; }
  }
  HideTextCursor();
  return(TextToHexWord());
}



//------------------------------------------------------------------------
// Vyber komunikacniho portu
//------------------------------------------------------------------------
void SelectPort()
{
  unsigned char I;

  GetScr(1,33,8,60,23);
  Frame(35,9,49,15,16+7,0);
  gotoxy(34,10); cprintf("Æµ");
  InitDownMenu("~Alt-X~ Exit  ~/~ Zmenit  ~ENTER/ESC~ Ok");

  ResetMenuItems();
  MenuItems[ 0].Y=10; MenuItems[ 0].Item="COM1\0x0";
  MenuItems[ 1].Y=11; MenuItems[ 1].Item="COM2\0x0";
  MenuItems[ 2].Y=12; MenuItems[ 2].Item="COM3\0x0";
  MenuItems[ 3].Y=13; MenuItems[ 3].Item="COM4\0x0";
  MenuItems[ 4].Y=14; MenuItems[ 4].Item="COM5\0x0";

  while(1)
  {
     if(Config.Port==4) InitDownMenu("~Alt-X~ Exit  ~/~ Zmenit  ~ENTER/ESC~ Ok  ~F4~ Edit");
     else InitDownMenu("~Alt-X~ Exit  ~/~ Zmenit  ~ENTER/ESC~ Ok");

     for(I=0;I<5;I++)
     {
       if(Config.Port==I) textattr(15); else textattr(7+16);
       if(MenuItems[I].Item[0]!=0)
       {
         gotoxy(36,MenuItems[I].Y);
         if(AdrPort(I)!=0)
           cprintf(" %s / %.4Xh ",MenuItems[I].Item,AdrPort(I));
         else
           cprintf(" %s / none  ",MenuItems[I].Item);
       }
     }

     GetKey();
     if((Ascii==0) & (Scan==45)) { Exit=1; break; }

     switch(Scan)
     {
       case 62: if(Config.Port==4)
                Config.CustomPortAdress=InputWordHEXLine(45, MenuItems[4].Y-1, 16+15, Config.CustomPortAdress);
                Config.AdresaPortu=Config.CustomPortAdress;
                break;
       case 72: if(Config.Port>0) Config.Port--; break;
       case 80: if(Config.Port<4) Config.Port++; break;
       case 71: Config.Port=0; break;
       case 79: Config.Port=4; break;
     }


     if((Scan==75) | (Scan==1) | (Scan==28)) { Scan=0; break; }
  }
  Config.AdresaPortu=AdrPort(Config.Port);

  if(Config.Port==4)
  {
    GetScr(2,17,02,67,22);
    textattr(15);
    gotoxy(20, 3); cprintf("ÚÄÄÄÄÄÄÄÄÄÄÄÄÄ Upozorneni ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿");
    gotoxy(20, 4); cprintf("³                                         ³");
    gotoxy(20, 5); cprintf("³     Pokud Vami zvoleny port pouziva     ³");
    gotoxy(20, 6); cprintf("³    jine zarizeni, ktere neni urceno     ³");
    gotoxy(20, 7); cprintf("³      pro seriovou komunikaci muze       ³");
    gotoxy(20, 8); cprintf("³      zapis na tento port zpusobit       ³");
    gotoxy(20, 9); cprintf("³    v lepsim pripade konflikt s timto    ³");
    gotoxy(20,10); cprintf("³    zarizenim, nebo v horsim pripade     ³");
    gotoxy(20,11); cprintf("³     nestabilitu systemu, nebo popr.     ³");
    gotoxy(20,12); cprintf("³  ztratu nekterych dat z pevneho disku ! ³");
    gotoxy(20,13); cprintf("³   Doporucuji ukoncit vsechny ostatni    ³");
    gotoxy(20,14); cprintf("³    spustene aplikace a jeste jednou     ³");
    gotoxy(20,15); cprintf("³  zkontrolovat adresu zvoleneho portu.   ³");
    gotoxy(20,16); cprintf("³          Zvoleny port: %.4Xh            ³",Config.AdresaPortu);
    gotoxy(20,17); cprintf("³                                         ³");
    gotoxy(20,18); cprintf("³                                         ³");
    gotoxy(20,19); cprintf("³                                         ³");
    gotoxy(20,20); cprintf("³                                         ³");
    gotoxy(20,21); cprintf("ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ");

    I=0;
    while(1)
    {
      if(!I) textattr(16+14);  else textattr(8);
      gotoxy(29,18); cprintf("Nepodstupovat toto riziko");
      if(I) textattr(16+14);  else textattr(8);
      gotoxy(29,19); cprintf("Podstoupit toto riziko");

      GetKey(); if((Scan==1) | (Scan==28)) break;
      if(Scan==72) I=0;
      if(Scan==80) I=1;
    }

    if((Scan==1) | (!I))
    {
      Config.Port=0;
      Config.AdresaPortu=AdrPort(Config.Port);
    }
    SetScr(2,17,02,67,22);
  }
  Scan=0;
  InitPort();

  SetScr(1,33,8,60,23);
  window(1,1,80,25);
}



//------------------------------------------------------------------------
// Nastaveni parametru serioveho prenosu
//------------------------------------------------------------------------
void SetPort()
{
  unsigned char B;

  GetScr(1,33,9,55,23);
  Frame(35,10,55,14,16+7,0);
  gotoxy(34,12); cprintf("Æµ");
  InitDownMenu("~Alt-X~ Exit  ~PgUp/PgDn~ Zmenit  ~ENTER/ESC~ Ok");

  ResetMenuItems();
  MenuItems[ 0].Y=11; MenuItems[ 0].Item="Stop bitu   \0x0";
  MenuItems[ 1].Y=12; MenuItems[ 1].Item="Parita      \0x0";
  MenuItems[ 2].Y=13; MenuItems[ 2].Item="Delka slova \0x0";

  while(1)
  {
     B=0;
     for(I=0;I<3;I++)
     {
       if(STPPol==I) textattr(15); else textattr(7+16);
       if(MenuItems[I].Item[0]!=0)
       {
         gotoxy(36,MenuItems[I].Y); cprintf(" %s",MenuItems[I].Item);

         if(!B) switch(Scan)
         {
           case 81:                                        // PGUP
             switch(STPPol)
             {
               case 0: if(Config.StpBit<1) Config.StpBit++; break;
               case 1: Config.Parita++;
                       if(Config.Parita>4) Config.Parita=0;
                       break;
               case 2: if(Config.PocBit<8) Config.PocBit++;
                       if(Config.PocBit>8) Config.PocBit=8;
                       break;
             }; B=1; break;
           case 73:                                        // PGDN
             switch(STPPol)
             {
               case 0: if(Config.StpBit>0) Config.StpBit--; break;
               case 1: Config.Parita--;
                       if(Config.Parita>4) Config.Parita=4;
                       break;
               case 2: if(Config.PocBit>5) Config.PocBit--;
                       if(Config.PocBit>8) Config.PocBit=8;
                       break;
             }; B=1; break;
           default: break;
         }
         switch(I)
         {
           case 0: cprintf("%2x     ",Config.StpBit+1); break;
           case 1:
           switch(Config.Parita)
           {
             case 0: cprintf("Zadna  "); break;
             case 1: cprintf("Suda   "); break;
             case 2: cprintf("Licha  "); break;
             case 3: cprintf("Vzdy 0 "); break;
             case 4: cprintf("Vzdy 1 "); break;
             default: break;
           } break;
           case 2: cprintf("%2x     ",Config.PocBit); break;
         }

       }
     }

     switch(STPPol)
     {
       case 0: ShowHint("Pocet stop bitu ( 1, 2 )\0x0"); break;
       case 1: ShowHint("Parita ( Zadna, Suda, Licha, Vzdy 0, Vzdy 1 )\0x0"); break;
       case 2: ShowHint("Pocet vysilanych a prijimanych bitu ( 5, 6, 7, 8 )\0x0"); break;
     }
     GetKey();
     if((Scan==72) & (STPPol>0)) STPPol--;
     if((Scan==80) & (STPPol<2)) STPPol++;
     if(Scan==71) STPPol=0;
     if(Scan==79) STPPol=2;
     if((Ascii==0) & (Scan==45)) { Exit=1; break; }

     if((Scan==75) | (Scan==1) | (Scan==28)) { Scan=0; break; }

  }
  SetScr(1,33,9,55,23);
  window(1,1,80,25);
}



//------------------------------------------------------------------------
// Nastaveni zpozdeni pri vysilani ASCII bufferu
//------------------------------------------------------------------------
void SetDelayASCII()
{
  GetScr(1,33,12,54,16);
  Frame(35,14,49,16,16+7,0);
  gotoxy(34,15); cprintf("Æµ");
  gotoxy(37,14); cprintf(" Delay [ms] ");
  InitDownMenu("~Alt-X~ Exit  ~PgUp/PgDn//~ Zmenit  ~ENTER/ESC~ Ok");
  textattr(15);
  while(1)
  {
    gotoxy(36,15); cprintf("   %5d      ",Config.Delay_Ascii);

    GetKey();
    if((Ascii==0) & (Scan==45)) { Exit=1; break; }

    if((Scan==75) | (Scan==1) | (Scan==28)) { Scan=0; break; }
    if(Scan==80) Config.Delay_Ascii--;
    if(Scan==72) Config.Delay_Ascii++;
    if(Scan==73) Config.Delay_Ascii=Config.Delay_Ascii+100;
    if(Scan==81) Config.Delay_Ascii=Config.Delay_Ascii-100;
    if(Scan==71) Config.Delay_Ascii=0;
    if(Scan==79) Config.Delay_Ascii=30000;

    if(Config.Delay_Ascii<0) Config.Delay_Ascii=30000;
    if(Config.Delay_Ascii>30000) Config.Delay_Ascii=0;
  }
  SetScr(1,33,12,54,16);
}



//------------------------------------------------------------------------
// Nastaveni zpozdeni pri vysilani HEX bufferu
//------------------------------------------------------------------------
void SetDelayHEX()
{
  GetScr(1,33,13,54,17);
  Frame(35,15,49,17,16+7,0);
  gotoxy(34,16); cprintf("Æµ");
  gotoxy(37,15); cprintf(" Delay [ms] ");
  InitDownMenu("~Alt-X~ Exit  ~PgUp/PgDn//~ Zmenit  ~ENTER/ESC~ Ok");
  textattr(15);
  while(1)
  {
    gotoxy(36,16); cprintf("   %5d      ",Config.Delay_HEX);

    GetKey();
    if((Ascii==0) & (Scan==45)) { Exit=1; break; }

    if((Scan==75) | (Scan==1) | (Scan==28)) { Scan=0; break; }
    if(Scan==80) Config.Delay_HEX--;
    if(Scan==72) Config.Delay_HEX++;
    if(Scan==73) Config.Delay_HEX=Config.Delay_HEX+100;
    if(Scan==81) Config.Delay_HEX=Config.Delay_HEX-100;
    if(Scan==71) Config.Delay_HEX=0;
    if(Scan==79) Config.Delay_HEX=30000;

    if(Config.Delay_HEX<0) Config.Delay_HEX=30000;
    if(Config.Delay_HEX>30000) Config.Delay_HEX=0;
  }
  SetScr(1,33,13,54,17);
}




//------------------------------------------------------------------------
// Vyber znaku ktere nebudou vypisovany
//------------------------------------------------------------------------
void DisplayChars()
{
  #define DCHMenuItems 7

  GetScr(1,33,9,54,17);
  Frame(35,10,54,11+DCHMenuItems,16+7,0);
  gotoxy(34,13); cprintf("Æµ");
  InitDownMenu("~Alt-X~ Exit  ~PgUp/PgDn~ Zmenit");

  ResetMenuItems();
  MenuItems[ 0].Y=11; MenuItems[ 0].Item="Funkce znaku \0x0";
  MenuItems[ 1].Y=12; MenuItems[ 1].Item="#0Dh ( LF )  \0x0";
  MenuItems[ 2].Y=13; MenuItems[ 2].Item="#0Ah ( CR )  \0x0";
  MenuItems[ 3].Y=14; MenuItems[ 3].Item="#07h (BELL)  \0x0";
  MenuItems[ 4].Y=15; MenuItems[ 4].Item="#08h (BCK)   \0x0";
  MenuItems[ 5].Y=16; MenuItems[ 5].Item="#09h (TAB)   \0x0";
  MenuItems[ 6].Y=17; MenuItems[ 6].Item="#1Fh  (LR)   \0x0";

  while(1)
  {
     for(I=0; I<DCHMenuItems; I++)
     {
       if(DCPol==I) textattr(15); else textattr(7+16);
       if(MenuItems[I].Item[0]!=0)
       {
         gotoxy(36,MenuItems[I].Y); cprintf(" %s ",MenuItems[I].Item);
         if(Config.DisplayChars[I]) cprintf("Ano "); else cprintf(" Ne ");
       }
     }

     switch(DCPol)
     {
       case 0: ShowHint("Povolit prichozim znakum menit pozici kurzoru nebo jejich funkci (BELL)\0x0"); break;
       case 1: ShowHint("Povolit/Zakazat zobrazeni LF\0x0"); break;
       case 2: ShowHint("Povolit/Zakazat zobrazeni ENTER\0x0"); break;
       case 3: ShowHint("Povolit/Zakazat zobrazeni BELL\0x0"); break;
       case 4: ShowHint("Povolit/Zakazat zobrazeni Backspace\0x0"); break;
       case 5: ShowHint("Povolit/Zakazat zobrazeni Tab\0x0"); break;
       case 6: ShowHint("Povolit/Zakazat navrat o radek (LR)\0x0"); break;
     }
     GetKey();
     if((Scan==72) & (DCPol>0)) DCPol--;
     if((Scan==80) & (DCPol<DCHMenuItems-1)) DCPol++;
     if(Scan==71) DCPol=0;
     if(Scan==79) DCPol=DCHMenuItems-1;

     if((Ascii==0) & (Scan==45)) { Exit=1; break; }
     if((Scan==73) | (Scan==81))
       Config.DisplayChars[DCPol]=!Config.DisplayChars[DCPol];

     if((Scan==75) | (Scan==1) | (Scan==28)) { Scan=0; break; }
  }
  SetScr(1,33,9,54,17);
}



//------------------------------------------------------------------------
// Vyber znaku ktere nebudou vypisovany
//------------------------------------------------------------------------
void SetBitMask()
{
  GetScr(1,33,9,54,20);
  Frame(35,10,46,19,16+7,0);
  gotoxy(34,14); cprintf("Æµ");
  InitDownMenu("~Alt-X~ Exit  ~PgUp/PgDn~ Zmenit");

  ResetMenuItems();
  MenuItems[ 0].Y=11; MenuItems[ 0].Item="Bit 0 \0x0";
  MenuItems[ 1].Y=12; MenuItems[ 1].Item="Bit 1 \0x0";
  MenuItems[ 2].Y=13; MenuItems[ 2].Item="Bit 2 \0x0";
  MenuItems[ 3].Y=14; MenuItems[ 3].Item="Bit 3 \0x0";
  MenuItems[ 4].Y=15; MenuItems[ 4].Item="Bit 4 \0x0";
  MenuItems[ 5].Y=16; MenuItems[ 5].Item="Bit 5 \0x0";
  MenuItems[ 6].Y=17; MenuItems[ 6].Item="Bit 6 \0x0";
  MenuItems[ 7].Y=18; MenuItems[ 7].Item="Bit 7 \0x0";

  while(1)
  {
     for(I=0;I<8;I++)
     {
       if(BMPol==I) textattr(15); else textattr(7+16);
       if(MenuItems[I].Item[0]!=0)
       {
         gotoxy(36,MenuItems[I].Y); cprintf(" %s ",MenuItems[I].Item);
         if(Config.BitMask & (1 << I)) cprintf(" 1 "); else cprintf(" 0 ");
       }
     }

     ShowHint("Prichozi znak & \0x0");
     printf("%.2Xh  (%d)",Config.BitMask,Config.BitMask);
     GetKey();
     if((Scan==72) & (BMPol>0)) BMPol--;
     if((Scan==80) & (BMPol<7)) BMPol++;
     if(Scan==71) BMPol=0;
     if(Scan==79) BMPol=7;

     if((Ascii==0) & (Scan==45)) { Exit=1; break; }
     if((Scan==73) | (Scan==81))
     {
       if(Config.BitMask & (1 << BMPol))
         Config.BitMask = (Config.BitMask & (255-(1 << BMPol)));
       else
         Config.BitMask = (Config.BitMask | (1 << BMPol));
     }

     if((Scan==75) | (Scan==1) | (Scan==28)) { Scan=0; break; }
  }
  SetScr(1,33,9,54,20);
}



void FixPortAdress()
{
  GetScr(1,33,9,54,20);
  Frame(35,16,42,19,16+7,0);
  gotoxy(34,17); cprintf("Æµ");
  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet");

  ResetMenuItems();
  MenuItems[ 0].Y=17; MenuItems[ 0].Item="BIOS \0x0";
  MenuItems[ 1].Y=18; MenuItems[ 1].Item="Pevne\0x0";

  Scan=Ascii=0;
  while(1)
  {
     for(I=0; I<2; I++)
     {
       if(Config.SetFixAdress==I) textattr(15); else textattr(7+16);
       gotoxy(36,MenuItems[I].Y); cprintf(" %s ",MenuItems[I].Item);
     }

     switch(Config.SetFixAdress)
     {
       case 0: ShowHint("Adresy portu COM1..4 nacist z promennych BIOSu (default)"); break;
       case 1: ShowHint("Pouzit pevne adresy portu ( COM1=3F8, COM2=2F8, COM3=3E8, COM4=2E8)"); break;
     }

     GetKey();
     if((Scan==72) & (Config.SetFixAdress>0)) Config.SetFixAdress=0;
     if((Scan==80) & (Config.SetFixAdress<2)) Config.SetFixAdress=1;
     if(Scan==71) Config.SetFixAdress=0;
     if(Scan==79) Config.SetFixAdress=1;

     if((Ascii==0) & (Scan==45)) { Exit=1; break; }
     if((Scan==73) | (Scan==81)) Config.SetFixAdress=!Config.SetFixAdress;
     if((Scan==75) | (Scan==1) | (Scan==28)) { Scan=0; break; }

  }
  SetScr(1,33,9,54,20);
}



void AutoOpen()
{
  GetScr(1,33,9,54,20);
  Frame(35,17,46,20,16+7,0);
  gotoxy(34,18); cprintf("Æµ");
  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet");

  ResetMenuItems();
  MenuItems[ 0].Y=18; MenuItems[ 0].Item="Off      \0x0";
  MenuItems[ 1].Y=19; MenuItems[ 1].Item="Autoopen \0x0";

  Scan=Ascii=0;
  while(1)
  {
     for(I=0; I<2; I++)
     {
       if(Config.AutoOpen==I) textattr(15); else textattr(7+16);
       gotoxy(36,MenuItems[I].Y); cprintf(" %s ",MenuItems[I].Item);
     }

     switch(Config.AutoOpen)
     {
       case 0: ShowHint("Po spusteni terminalu bude otevren naposledy otevreny terminal "); break;
       case 1: ShowHint("Po spusteni terminalu bude kurzor na prvnim radku hlavniho menu "); break;
     }

     GetKey();
     if((Scan==72) | (Scan==80)) Config.AutoOpen=!Config.AutoOpen;
     if(Scan==71) Config.AutoOpen=0;
     if(Scan==79) Config.AutoOpen=1;

     if((Ascii==0) & (Scan==45)) { Exit=1; break; }
     if((Scan==73) | (Scan==81)) Config.AutoOpen =! Config.AutoOpen;
     if((Scan==75) | (Scan==1) | (Scan==28)) { Scan=0; break; }

  }
  SetScr(1,33,9,54,20);
}



void MenuExitDialog()
{
  GetScr(1,33,9,54,20);
  Frame(35,18,49,21,16+7,0);
  gotoxy(34,19); cprintf("Æµ");
  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet");

  ResetMenuItems();
  MenuItems[ 0].Y=19; MenuItems[ 0].Item="Nezobrazovat\0x0";
  MenuItems[ 1].Y=20; MenuItems[ 1].Item=" Zobrazovat \0x0";

  Scan=Ascii=0;
  while(1)
  {
     for(I=0; I<2; I++)
     {
       if(Config.ExitDialog==I) textattr(15); else textattr(7+16);
       gotoxy(36,MenuItems[I].Y); cprintf(" %s ",MenuItems[I].Item);
     }

     switch(Config.ExitDialog)
     {
       case 0: ShowHint("Pri ukonceni terminalu se nezobrazi potvrzovaci dialog"); break;
       case 1: ShowHint("Pred ukoncenim terminalu se zobrazi potvrzovaci dialog"); break;
     }

     GetKey();
     if((Scan==72) & (Config.ExitDialog>0)) Config.ExitDialog=0;
     if((Scan==80) & (Config.ExitDialog<2)) Config.ExitDialog=1;
     if(Scan==71) Config.ExitDialog=0;
     if(Scan==79) Config.ExitDialog=1;

     if((Ascii==0) & (Scan==45)) { Exit=1; break; }
     if((Scan==73) | (Scan==81)) Config.ExitDialog=!Config.ExitDialog;
     if((Scan==75) | (Scan==1) | (Scan==28)) { Scan=0; break; }

  }
  SetScr(1,33,9,54,20);
}



void ExitDialog()
{
  GetScr(0,18,10,60,15);
  ShowHint("");
  InitDownMenu("~ESC~ Zpet  ~Alt+X / ENTER~ Exit ");
  Frame(23,11,58,13,15,1);
  textattr(15);
  gotoxy(25,12); cprintf("Opravdu chcete ukoncit terminal?");
  while(1)
  {
    GetKey();
    if((Scan==1)  | (UpChr(Ascii)=='N')) { Exit=0; break; }
    if((Scan==45) & (Ascii==0)) break;
    if((Scan==28) | (Scan==57)  | (UpChr(Ascii)=='Y') |
       (UpChr(Ascii)=='A')) break;
  }
  SetScr(0,18,10,60,15);
}


//------------------------------------------------------------------------
// Zobrazi chybove hlaseni Timeout, ESC prerusit akci, ENTER pokracuje
//------------------------------------------------------------------------
void DisplayTimeOutError()
{
  SaveCursorPos(3);
  HideTextCursor();
  window(1,1,80,25);
  InitDownMenu("~Alt+X~ Exit  ~ESC~ Prerusit  ~ENTER~ Pokracovat");
  ShowHint("Port nevyslal znak v pozadovanem case !");
  GetScr(2,17,11,67,14);
  Frame(18,12,65,14,64+15,0);
  gotoxy(38,12); cprintf(" Chyba ");
  gotoxy(22,13); cprintf("Chyba timeout komunikacniho portu !",Config.AdresaPortu);
  while(1)
  {
    GetKey();
    if((Scan==1) | (Scan==28))
    {
      SetScr(2,17,11,67,14);
      ShowTextCursor();
      break;
    }
    if((Scan==45) & (Ascii==0)) { Exit=1; break; }
  }
  LoadCursorPos(3);
  textattr(15);
}



//------------------------------------------------------------------------
// Vysilani znaku bez zastaveni
//------------------------------------------------------------------------
void SendNonstop()
{
  unsigned char Pause;

  GetScr(1,36,9,55,17);
  Frame(38,10,44,16,16+7,0);
  gotoxy(37,11); cprintf("Æµ");
  ResetMenuItems();
  MenuItems[ 0].Y=11; MenuItems[ 0].Item="#0   \0x0";
  MenuItems[ 1].Y=12; MenuItems[ 1].Item="#7   \0x0";
  MenuItems[ 2].Y=13; MenuItems[ 2].Item="#FF  \0x0";
  MenuItems[ 3].Y=14; MenuItems[ 3].Item="RND  \0x0";
  MenuItems[ 4].Y=15; MenuItems[ 4].Item="P40h \0x0";

  SNPol=0;

  while(1)
  {
     for(I=0; I<5; I++)
     {
       if(SNPol==I) textattr(15); else textattr(7+16);
       if(MenuItems[I].Item[0]!=0)
       { gotoxy(39,MenuItems[I].Y); cprintf(" %s",MenuItems[I].Item); }
     }

     switch(SNPol)
     {
       case 0: ShowHint("Vysilani znaku 0  (NULL)\0x0"); break;
       case 1: ShowHint("Vysilani znaku 7  (BELL)\0x0"); break;
       case 2: ShowHint("Vysilani znaku FFh\0x0"); break;
       case 3: ShowHint("Vysilani nahodneho znaku\0x0"); break;
       case 4: ShowHint("Posila data z portu 40h na komunikacni port\0x0"); break;
     }

     GetKey();

     switch(Scan)
     {
       case 72: if(SNPol>0) SNPol--; break;
       case 80: if(SNPol<4) SNPol++; break;
       case 71: SNPol=0;             break;
       case 79: SNPol=4;             break;
     }

     if((Ascii==0) & (Scan==45)) { Exit=1; break; }
     if((Scan==75) | (Scan==1))  { Scan=0; SetScr(1,36,9,44,17); return; }
     if((Scan==77) | (Scan==28)) { Scan=0; break; }
  }
  SetScr(1,36,9,44,17);

  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet  ~Space~ Pause");

  GetScr(1,11,1,79,23);
  Frame(12,2,79,23,15,0);
  gotoxy(46,2); printf(" Vysila se ");

  ShowPortSet();

  InitPort();
  window(14,3,79,22);
  ShowTextCursor();
  Pause=0;

  while(1)
  {
    if(kbhit())
    {
      GetKey();
      if(Scan==1) break;
      if((Ascii==0) & (Scan==45)) { Exit=1; break; }
    }

    if(!Pause)
    {
      switch(SNPol)
      {
        case 0: Byte=0;            break;
        case 1: Byte=7;            break;
        case 2: Byte=0xFF;         break;
        case 3: Byte=random(256);  break;
        case 4: Byte=inp(0x40); break;
        default:Byte=0;            break;
      }

      if(VysliByte(Byte)==0xFF) DisplayTimeOutError();
      else cprintf("%.2X ",Byte);

      if(Scan==57) { Pause=1; Scan=0; }
    }
    else if(Scan==57) { Pause=0; Scan=0; };

    if(Exit) break;
    if(Scan==1) break;
  }
  window(1,1,80,25);
  SetScr(1,11,1,79,23);
}



//------------------------------------------------------------------------
// Zobrazeni prichozich znaku bez cekani na dalsi
//------------------------------------------------------------------------
void ViewCOM()
{
  GetScr(1,11,1,79,23);
  Frame(12,2,79,23,15,0);
  gotoxy(37,2); printf(" Prijem ");

  ShowPortSet();

  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet");
  InitPort();
  textattr(15);
  window(14,3,79,22);
  ShowTextCursor();

  while(1)
  {
    if(kbhit())
    {
      GetKey(); if(Scan==1) break;
      if((Ascii==0) & (Scan==45)) { Exit=1; break; }
    }
    cprintf("%.2X ",inp(Config.AdresaPortu));
  }
  HideTextCursor();
  window(1,1,80,25);
  SetScr(1,11,1,79,23);
}



//------------------------------------------------------------------------
// Zobrazeni prijatych znaku s cekanim na dalsi
//------------------------------------------------------------------------
void ViewReceive()
{
  GetScr(1,11,1,79,23);
  Frame(12,2,79,23,15,0);
  gotoxy(37,2); printf(" Prijem ");

  ShowPortSet();

  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet");
  InitPort();
  textcolor(15);textbackground(0);
  window(14,3,79,22);
  ShowTextCursor();

  while(1)
  {
    if(kbhit())
    {
      GetKey();
      if(Scan==1) break;
      if((Ascii==0) & (Scan==45)) { Exit=1; break; }
    }

    if(CharReady())
    {
      cprintf("%.2X ",FilterChar(inp(Config.AdresaPortu)));
    }

    if(Exit) break;

  }
  HideTextCursor();
  window(1,1,80,25);
  SetScr(1,11,1,79,23);
}




void EchoMode()
{
  unsigned long C1,C2;

  HideTextCursor();
  window(1,1,80,25);
  InitPort();
  InitDownMenu("~Alt+X~ Exit  ~ESC~ Zpet");
  ShowHint("Posila zpet prijate znaky a zobrazuje jejich pocet...");
  GetScr(2,17,11,67,14);
  Frame(18,12,65,14,15,1);
  textattr(15);
  gotoxy(38,12); cprintf(" ECHO MODE ");
  C1=C2=0;
  while(1)
  {
    Scan=Ascii=0;
    if(kbhit()) GetKey();

    if(inp(0x3FD) & 1) { C1++; outp(0x3F8,inp(0x3F8)); }
    if(inp(0x2FD) & 1) { C2++; outp(0x2F8,inp(0x2F8)); }

    gotoxy(23,13); cprintf("COM1: %10.0f      COM2: %10.0f ",(float)C1, (float)C2);

    if((Scan==1) | (Scan==28)) break;
    if((Scan==45) & (Ascii==0)) { Exit=1; break; }
  }
  SetScr(2,17,11,67,14);
  LoadCursorPos(3);
  textattr(15);
}



//------------------------------------------------------------------------
// Nastavi normalni textovy kurzor ( blikajici podtrzitko )
//------------------------------------------------------------------------
void NormalTextCursor()
{
  asm{
  MOV AH,1
  MOV CH,15
  MOV CL,16
  INT 10h }
}



//------------------------------------------------------------------------
// Nastavi textovy kurzor pro insert mod  ( blikajici plny znak )
//------------------------------------------------------------------------
void InsertTextCursor()
{
  asm{
  MOV AH,1
  MOV CH,0
  MOV CL,16
  INT 10h }
}



//------------------------------------------------------------------------
// Filtrace klaves - vynuluje funkcni klavesy  ( pro Ascii )
//------------------------------------------------------------------------
void FilterKeys()
{
  switch (Scan)
  {
    case 73:   //PgUp
    case 81:   //PgDn
    case 71:   //Home
    case 79:   //End
    case 72:   //Up
    case 80:   //Down
    case 75:   //Left
    case 77:   //Right
    case 83:   //Delete
    case 59:   //F1
    case 60:
    case 61:
    case 62:
    case 63:
    case 64:
    case 65:
    case 66:
    case 67:
    case 68: Scan=0; Ascii=0; //F10
    break;
  }

  switch (Ascii)
  {
    case 10:    //LN
    case 26: Scan=0; Ascii=0; //EOF
    break;
  }
}



//------------------------------------------------------------------------
// Filtrace klaves - vynuluje funkcni klavesy  ( pro HEX )
//------------------------------------------------------------------------
void FilterHEXKeys()
{
  switch (Scan)
  {
    case 73:   //PgUp
    case 81:   //PgDn
    case 71:   //Home
    case 79:   //End
    case 72:   //Up
    case 80:   //Down
    case 75:   //Left
    case 77:   //Right
    case 83:   //Delete
    case 59:   break; //F1
    case 68: Scan=0; Ascii=0; break;  //F10
    default: Scan=Ascii=0;
  }

  switch (Ascii)
  {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case  10:    //LN
    case  26: Scan=Ascii=0; break;  //EOF
    default: Ascii=0;
  }
}




//------------------------------------------------------------------------
// Vraci 128, pokud je zapnuty prepisovaci mod  ( stisknuto Insert )
//------------------------------------------------------------------------
unsigned char Insert()
{
  return(peekb(0,0x0417) & 128);
}



//------------------------------------------------------------------------
// Vraci stav klaves SHIFT, 0=zadny
//------------------------------------------------------------------------
unsigned char Shift()
{
  return(peekb(0,0x0417) & 3);
}




//------------------------------------------------------------------------
// Zobrazi znak Ch na pozici X,Y s barvou Atr
//------------------------------------------------------------------------
void PutChar(char X,char Y,char Atr,char Ch)
{
  gotoxy(X+1,Y+1);
  textattr(Atr);
  putch(Ch);

  //pokeb(0xB800,Y*80*2+X*2,Ch);
  //pokeb(0xB800,Y*80*2+X*2+1,Atr);
  // Je to to same, rychlejsi, ale nefunguje vzdy spolehlive
}




//------------------------------------------------------------------------
// Vstupni textovy radek na souradnicich X,Y   Length znaku dlouhy
// s barvou Attr
// OldStr je vychozi text
// Vystupem je promenna Vstup
//------------------------------------------------------------------------
void InputTextLine(int x,int y,int Size,char Atr,char *DefaultText)
{
  unsigned char S[256];
  unsigned int I,Pos;
  unsigned char _Ins;

  for(I=0; I<256; ++I) S[I]=0x0;
  for(I=0; I<256; ++I) Vstup[I]=0x0;
  for(I=0; I<strlen(DefaultText); ++I) S[I]=DefaultText[I];
  for(I=0; I<Size+1; ++I) PutChar(x+I, y, Atr, 0x00);
  Pos=strlen(DefaultText);
  textattr(Atr);
  if(Insert()) InsertTextCursor(); else NormalTextCursor();
  gotoxy(x+1,y+1);
  I=0;
  while(DefaultText[I]!=0)
  {
    switch(DefaultText[I])
    {
      case 10:
      case 13: putch('þ'); break;
      default: putch(DefaultText[I]); break;
    }
    I++;
    if(I>=Size) break;
  }

  while(1)
  {
    gotoxy(x+1+Pos,y+1);
    Scan=0;Ascii=0;
    GetKey();
    //HideTextCursor();

    if((Scan==1) | (Scan==28) | ((Scan==15) & (Input_TAB_ENABLED))) break;

    if((Scan==46) & (Ascii==3))                     // Ctrl+C
    {
      for(I=0; I<sizeof(S); I++) S[I]=0;
      gotoxy(x+1,y+1);
      for(I=0;I<Size;++I)
      {
        switch(S[I])
        {
          case 10:
          case 13: putch('þ'); break;
          default: putch(S[I]); break;
        }
      }
      Pos=0;
      gotoxy(x+1+Pos,y+1);
      Scan=0; Ascii=0;
    }

    switch(Scan)
    {
      case 82: if(_Ins!=Insert())                   // Insert
                 if(Insert()) InsertTextCursor(); else NormalTextCursor();
              _Ins=Insert();
              break;
      case 83:                                      // Delete
               S[Pos]=0;
               for(I=Pos; I<Size; ++I) S[I]=S[I+1];
               gotoxy(x+1,y+1);
               for(I=0;I<Size;++I)
               {
                 switch(S[I])
                 {
                   case 10:
                   case 13: putch('þ'); break;
                   default: putch(S[I]); break;
                 }
               }
               gotoxy(x+1+Pos,y+1);
               break;
      case 75: if(Pos>0)--Pos; break;               // Left
      case 77: if(Pos<strlen(S)) ++Pos; break;      // Right
      case 71:                                      // Home
      case 72:                                      // Up
      case 73: Pos=0; break;                        // PgUp
      case 79:                                      // End
      case 80:                                      // Down
      case 81: Pos=strlen(S); break;                // PgDn
      case 14: if(Pos>0)                            // Backspace
               {
                 --Pos;
                 S[Pos]=0x00;
                 gotoxy(x+1,y+1); putch(0x20);
                 for(I=Pos; I<Size; ++I) S[I]=S[I+1];

                 gotoxy(x+1,y+1);
                 for(I=0;I<Size;++I)
                 {
                   switch(S[I])
                   {
                     case 10:
                     case 13: putch('þ'); break;
                     default: putch(S[I]); break;
                   }
                 }
                 gotoxy(x+1+Pos,y+1);
               }
               break;
      default: FilterKeys();
               if(Pos<Size)
               {
                 if(Insert())
                 {
                   if(Ascii!=0) { S[Pos]=Ascii; ++Pos;}
                   gotoxy(x+1,y+1);
                   for(I=0;I<Size;++I)
                   {
                     switch(S[I])
                     {
                       case 10:
                       case 13: putch('þ'); break;
                       default: putch(S[I]); break;
                     }
                   }
                   gotoxy(x+1+Pos,y+1);
                 }
                 else
                 {
                   if(Ascii!=0)
                   {
                     if(S[Size-1]==0)
                     {
                       for(I=(Size-1);I>Pos;--I) S[I]=S[I-1];
                       S[Pos]=Ascii; ++Pos;
                     }
                     gotoxy(x+1,y+1);
                     for(I=0;I<Size;++I)
                     {
                       switch(S[I])
                       {
                         case 10:
                         case 13: putch('þ'); break;
                         default: putch(S[I]); break;
                       }
                     }
                     gotoxy(x+1+Pos,y+1);
                   }
                 }
               }
    }
  }
  ShowTextCursor();

  if(_CancelModeEnabled)
  {
    if(Scan==1)
      for(I=0;I<strlen(DefaultText);++I) Vstup[I]=DefaultText[I];
    else
      for(I=0;I<strlen(S);++I) Vstup[I]=S[I];
  }
  else
    for(I=0;I<strlen(S);++I) Vstup[I]=S[I];
  NormalTextCursor();
}




//------------------------------------------------------------------------
// Editace uzivatelskeho bufferu (0..11) ( pro Ascii )
//------------------------------------------------------------------------
void CustomASCIIBuf(unsigned char _Buffer)
{
  SaveCursorPos(0);
  GetScr(2,12,9,69,13);
  window(1,1,80,25);
  Frame(13,10,69,13,16+32+64,0);
  gotoxy(36,10); cprintf(" Buffer 0%X ",_Buffer+1);
  InputTextLine(13,11,55,16+15, &Config.CustomASCIIBuffer[_Buffer][0]);
  Scan=0;

  for(I=0;I<60;I++) Config.CustomASCIIBuffer[_Buffer][I]=Vstup[I];

  textattr(15);
  SetScr(2,12,9,69,13);
  window(24,3,79,22);
  LoadCursorPos(0);
}





//------------------------------------------------------------------------
// Vstupni radek pro HEX
// Vystupem je promenna Vstup ( HEXa cislo v textu )
//------------------------------------------------------------------------
void InputHEXLine( int x, int y, int Size, char Atr, char *DefaultText)
{
  unsigned char S[256];
  unsigned int i,I,Pos;
  unsigned char _Ins,Hex_Pos;

  for(I=0; I<256; ++I) S[I]=0x0;
  for(I=0; I<256; ++I) Vstup[I]=0x0;

  for(I=0; I<strlen(DefaultText); ++I) S[I]=DefaultText[I];

  for(I=0; I<Size+1; ++I) PutChar(x+I, y, Atr, 0x00);
  Pos=strlen(DefaultText);
  textcolor(Atr & 15);textbackground((Atr & (16+32+64+128))>>4);
  gotoxy(x+1,y+1); cprintf("%s",DefaultText);

  NormalTextCursor();

  Hex_Pos=0;
  while(1)
  {
    Scan=0;Ascii=0;
    GetKey();
    Ascii=UpChr(Ascii);

    if((Scan==1) | (Scan==28) | ((Scan==15) & (Input_TAB_ENABLED))) break;

    if((Scan==46) & (Ascii==3))            // Ctrl+C
    {
      gotoxy(x+1,y+1);
      for(I=0; I<sizeof(S); I++) S[I]=0;
      for(I=0;I<Size;++I) putch(S[I]);
      Pos=0;
      Scan=0; Ascii=0;
      gotoxy(x+1+Pos,y+1);
    }

    if((Ascii==224) | (Ascii==0) | (Scan==14))
    {
      switch(Scan)
      {
        case 71:                                              // Home
        case 72:                                              // Up
        case 73: if(Ascii==0) Pos=0; break;                   // PgUp
        case 79:                                              // End
        case 80:                                              // Down
        case 81: if(Ascii==0) Pos=strlen(S); break;           // PgDn
        case 14: if(Pos>0)                                    // Backspace
                 {
                   Pos--; S[Pos]=0;
                   for(I=Pos; I<Size; ++I) S[I]=S[I+1];

                   if((Pos>0) & (S[Pos-1]!=0x20))
                   {
                     Pos--; S[Pos]=0;
                     for(I=Pos; I<Size; ++I) S[I]=S[I+1];
                   }

                   if((Pos>0) & (S[Pos-1]!=0x20))
                   {
                     Pos--; S[Pos]=0;
                     for(I=Pos; I<Size; ++I) S[I]=S[I+1];
                   }

                   Hex_Pos=0;
                   HideTextCursor();
                   PutChar(x+Pos,y,Atr,0);

                   for(I=0; I<Size; ++I) PutChar(x+I,y,Atr,S[I]);
                   ShowTextCursor();
                 }
                 break;
      }
      Scan=0; Ascii=0;
    }

    if(((UpChr(Ascii)>='A') & (UpChr(Ascii)<='F')) |
      ((UpChr(Ascii)>='0') & (UpChr(Ascii)<='9')))
    {
      HideTextCursor();
      if(Pos<Size)
      {
        if((Pos>0) & (S[Pos-1]!=0x20)) Hex_Pos=1;
        if(Ascii!=0)
        {
          if(S[Size-1]==0)
          {
            for(I=(Size-1); I>Pos; --I) S[I]=S[I-1];
            S[Pos]=Ascii; ++Pos;
            Hex_Pos++;
            if((Hex_Pos>=2) & (Pos<Size-1))
            {
              Hex_Pos=0; S[Pos]=0x20; ++Pos;
            }
          }
          gotoxy(x+1,y+1);
          for(I=0;I<Size;++I) putch(S[I]);
          gotoxy(x+1+Pos,y+1);
        }
      }
      ShowTextCursor();
    }
    gotoxy(x+Pos+1,y+1);
  }

  if(_CancelModeEnabled)
  {
    if(Scan==1)
      for(I=0;I<strlen(DefaultText);++I) Vstup[I]=DefaultText[I];
    else
      for(I=0;I<sizeof(S);++I) Vstup[I]=S[I];
  }
  else
    for(I=0;I<sizeof(S);++I) Vstup[I]=S[I];
  NormalTextCursor();
}




//------------------------------------------------------------------------
// Editace uzivatelskeho bufferu (0..11)  ( pro HEX )
//------------------------------------------------------------------------
void EditCustomHEXBuf(unsigned char _Buffer)
{
  SaveCursorPos(0);
  GetScr(2,12,9,70,13);
  window(1,1,80,25);
  Frame(13,10,70,13,16+32+64,0);
  gotoxy(36,10); cprintf(" Buffer 0%X ",_Buffer+1);
  InputHEXLine(13,11,56,16+15, &Config.CustomHEXBuffer[_Buffer][0]);
  Scan=0;

  for(I=0;I<60;I++) Config.CustomHEXBuffer[_Buffer][I]=Vstup[I];

  textattr(15);
  SetScr(2,12,9,70,13);
  window(24,3,79,22);
  LoadCursorPos(0);
}



#include "TERMEGG.H"



//------------------------------------------------------------------------
// Ascii terminal
// Znaky z klavesnice jsou posilany primo na port, nejsou lokalne
// zobrazovany, zobrazuji se pouze prijate znaky

// Kombinaci klaves Ctrl+F1 .. F10 se edituji Bufferu
// Klavesami F1 .. F10 se potom znaky v Bufferech vysilaji postupne
// s definovatelnym zpozdenim ( Nastaveni/Delay pro ASCII )
// Buffery jsou odlisne pro Ascii a Hex terminaly
//------------------------------------------------------------------------
void Terminal()
{
  InitPort();
  GetScr(0,11,1,79,23);
  Frame(12,2,79,23,15,0);
  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet  ~F1..F12~ Vlozit buf  ~Ctrl+F1..F12~ Edit buf");
  textattr(15);
  gotoxy(38,2); cprintf(" Terminal (ASCII) ");
  ShowPortSet();
  window(13,3,79,22);
  ShowTextCursor();
  Config.CustomASCIIBuffer[10][60]=0;

  //while(inp(0x60)<128);
  Input_TAB_ENABLED=0;

  while(1)
  {
    if(kbhit())
    {
      GetKey();
      if(Scan==1) break;

      if((((Scan>=94) & (Scan<=103)) | (Scan==137) | (Scan==138)) & (Ascii==0))
      {
        if(Scan<=103) CustomASCIIBuf(Scan-94);
        else CustomASCIIBuf(Scan-137+10);
        Ascii=0;
      }

      if((Scan>=59) & (Scan<=68) & (Ascii==0))
      {
        textattr(15);
        for(I=0;Config.CustomASCIIBuffer[Scan-59][I]!=0;I++)
        {
          if(Delay(Config.Delay_Ascii)) break;
          //putch(Config.CustomASCIIBuffer[Scan-59][I]);

          if(VysliByte(Config.CustomASCIIBuffer[Scan-59][I])==0xFF)
          {
             DisplayTimeOutError();
             if(Scan==1) break;
          }
        }
        Ascii=0;
      }


      if(((Scan==133) | (Scan==134)) & (Ascii==0))
      {
        textattr(15);
        for(I=0;Config.CustomASCIIBuffer[Scan-133+10][I]!=0;I++)
        {
          if(Delay(Config.Delay_Ascii)) break;
          //putch(Config.CustomASCIIBuffer[Scan-133+10][I]);

          if(VysliByte(Config.CustomASCIIBuffer[Scan-133+10][I])==0xFF)
          {
            DisplayTimeOutError();
            if(Scan==1) break;
          }
         }
        Ascii=0;
      }


      if(Ascii)
        if(VysliByte(Ascii)==0xFF)
        {
          DisplayTimeOutError();
          if(Scan==1) break;
        }
    }

    if(CharReady())
    {
       Byte=FilterChar(inp(Config.AdresaPortu) & Config.BitMask);

       if((Config.DisplayChars[0]) & (Byte==0x1F))
       {
         if(wherey() > 1) gotoxy(wherex(),wherey()-1);
       }
       else cprintf("%c",Byte);
    }

    if((Scan==45)&(Ascii==0)) Exit=1;
    if((Scan==1) | (Exit)) break;
  }
  HideTextCursor();
  window(1,1,80,25);
  SetScr(0,11,1,79,23);
  InitDownMenu("~Alt-X~ Exit");
}



//------------------------------------------------------------------------
// I/O Ascii terminal
// Dve okna - Vstupni a Vystupni
// Ve vstupnim ( hornim ) okne jsou zobrazovany znaky z klavesnice ( vysilane )
// Ve vystupnim okne jsou zobrazovany znaky prijate

// Kombinaci klaves Ctrl+F1 .. F10 se edituji Bufferu
// Klavesami F1 .. F10 se potom znaky v Bufferech vysilaji postupne
// s definovatelnym zpozdenim ( Nastaveni/Delay pro ASCII )
// Buffery jsou odlisne pro Ascii a Hex terminaly
//------------------------------------------------------------------------
void TerminalIOASC()
{

  InitPort();
  GetScr(0,11,1,79,23);
  Frame(12,2,79,12,15,0);
  Frame(12,13,79,23,15,0);
  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet  ~F1..F12~ Vlozit buf  ~Ctrl+F1..F12~ Edit buf");
  textattr(15);
  gotoxy(35,2); cprintf(" Terminal (ASCII) Vstup ");
  gotoxy(35,13); cprintf(" Terminal (ASCII) Vystup ");
  gotoxy(65,12); cprintf(" KEYB ");

  ShowPortSet();

  ShowTextCursor();
  gotoxy(1,1);

  Cursor_X_1=12; Cursor_Y_1=2;
  LoadCursorPos(1);
  Cursor_X_2=12; Cursor_Y_2=13;
  Config.CustomASCIIBuffer[10][60]=0;
  LoadCursorPos(1);
  ShowTextCursor();
  Input_TAB_ENABLED=0;
  while(1)
  {
    if(kbhit())
    {
      GetKey();

      TestEgg(Ascii);

      window(13,3,79,11);
      LoadCursorPos(1);

      if(Scan==1) break;

      if((((Scan>=94) & (Scan<=103)) | (Scan==137) | (Scan==138)) & (Ascii==0))
      {
        if(Scan<=103) CustomASCIIBuf(Scan-94);
        else CustomASCIIBuf(Scan-137+10);
        Ascii=0;
      }

      if((Scan>=59) & (Scan<=68) & (Ascii==0))
      {
        textattr(15);
        for(I=0;Config.CustomASCIIBuffer[Scan-59][I]!=0;I++)
        {
          if(Delay(Config.Delay_Ascii)) break;
          putch(Config.CustomASCIIBuffer[Scan-59][I]);

          if(VysliByte(Config.CustomASCIIBuffer[Scan-59][I])==0xFF)
          {
            DisplayTimeOutError();
            if(Scan==1) break;
          }
         }
        Ascii=0;
      }


      if(((Scan==133) | (Scan==134)) & (Ascii==0))
      {
        textattr(15);
        for(I=0; Config.CustomASCIIBuffer[Scan-133+10][I]!=0; I++)
        {
          if(Delay(Config.Delay_Ascii)) break;
          putch(Config.CustomASCIIBuffer[Scan-133+10][I]);

          if(VysliByte(Config.CustomASCIIBuffer[Scan-133+10][I])==0xFF)
          {
            DisplayTimeOutError();
            if(Scan==1) break;
          }
         }
        Ascii=0;
      }


      if(Ascii)
        if(VysliByte(Ascii)==0xFF)
        {
          DisplayTimeOutError();
          if(Scan==1) break;
        }


      switch(Ascii)
      {
       case 0: break;
       case 10:
       case 13: Cursor_Y_1++; Cursor_X_1=23; LoadCursorPos(1); Ascii=0; break;
       case  8: if(Cursor_X_1 > 11)
                {
                  Cursor_X_1--;
                  if(Cursor_X_1 < 12)
                  {
                    if(Cursor_Y_1 > 2) { Cursor_X_1=78; Cursor_Y_1--; }
                    else Cursor_X_1=12;
                  }
                  LoadCursorPos(1);
                }
                Ascii=0; break;
       case  9: break;
       default: cprintf("%c",Ascii); break;
      }
      SaveCursorPos(1);
      if(Cursor_X_1 < 12) Cursor_X_1=12;
      if(Cursor_X_1 > 78) { Cursor_X_1=12; Cursor_Y_1++; LoadCursorPos(1); }
      if(Cursor_Y_1 > 10) { RolWindowUP(12,2,78,10); Cursor_Y_1=10; LoadCursorPos(1); }
    }

    if(CharReady())
    {
       HideTextCursor();
       window(13,14,79,22);
       LoadCursorPos(2);
       Byte=FilterChar(inp(Config.AdresaPortu) & Config.BitMask);
       cprintf("%c",Byte);
       SaveCursorPos(2);
    }

    if((Scan==45)&(Ascii==0)) Exit=1;
    if((Scan==1) | (Exit)) break;
  }
  HideTextCursor();
  InitDownMenu("~Alt-X~ Exit");
  window(1,1,80,25);
  SetScr(0,11,1,79,23);
}



//------------------------------------------------------------------------
// Hex terminal
// Na hornim radku se zobrazuji hex. cisla zadavane z klavesnice
// Ve spodnim okne se zobrazuji prijate znaky v hex. tvaru

// Kombinaci klaves Ctrl+F1 .. F10 se edituji Bufferu
// Klavesami F1 .. F10 se potom znaky v Bufferech vysilaji postupne
// s definovatelnym zpozdenim ( Nastaveni/Delay pro HEX )
// Buffery jsou odlisne pro Ascii a Hex terminaly
//------------------------------------------------------------------------
void TerminalHEX()
{
  unsigned char Hex_Pos, Ch, Break;
  unsigned char RolBuf[60];

  InitPort();
  GetScr(0,11,1,79,23);
  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet  ~F1..F12~ Vlozit buf  ~Ctrl+F1..F12~ Edit buf");
  textattr(15);
  Frame(12,2,79,23,15,0);
  gotoxy(38,2); cprintf(" Terminal (HEX) ");
  gotoxy(13,4); for(I=0; I<67; I++) putch('Ä');
  ShowPortSet();
  window(13,3,79,3);
  ShowTextCursor();

  Hex_Pos=0;
  HEX[0]=HEX[1]=0;
  Config.CustomHEXBuffer[10][60]=0;

  Cursor_X_0=12; Cursor_Y_0=2; LoadCursorPos(0);
  Cursor_X_1=12; Cursor_Y_1=4; LoadCursorPos(1);

  for(I=0; I<60; I++) RolBuf[I]=0;
  Input_TAB_ENABLED=0;

  while(1)
  {

    if(kbhit())
    {
      GetKey();
      if(Scan==1) break;
      if((Ascii==0) & (Scan==45)) Exit=1;

      if((((Scan>=94) & (Scan<=103)) | (Scan==137) | (Scan==138)) & (Ascii==0))
      {
        if(Scan<=103) EditCustomHEXBuf(Scan-94);
        else EditCustomHEXBuf(Scan-137+10);
        Ascii=0;
      }

      //if(Hex_Pos==0)

      // F1..F10 = 59..68 ; F11 = 133 ; F12 = 134
      if((((Scan>=59) & (Scan<=68)) | (Scan==133)| (Scan==134)) & (Ascii==0))
      {
        textattr(15);
        window(13,3,79,3);

        Hex_Pos=0;
        I=0;
        Break=0;

        while(1)
        {
          window(13,3,79,11);

          if(Scan<=68)
            RolBuf[51]=HEX[0]=Config.CustomHEXBuffer[Scan-59][Hex_Pos];
          else
            RolBuf[51]=HEX[0]=Config.CustomHEXBuffer[Scan-133+10][Hex_Pos];

          if(HEX[0]==0) Break=1;
          Hex_Pos++;

          if(Scan<=68)
            RolBuf[52]=HEX[1]=Config.CustomHEXBuffer[Scan-59][Hex_Pos];
          else
            RolBuf[52]=HEX[1]=Config.CustomHEXBuffer[Scan-133+10][Hex_Pos];

          if(HEX[1]==0) Break=1;
          Hex_Pos++; Hex_Pos++;

          if(inp(0x60)==1) Break=1;

          if(Break) break;
          else
          {
             for(I=0; I<53; I++) putch(RolBuf[I]);
             for(I=0; I<53; I++) RolBuf[I]=RolBuf[I+3];

             if(VysliByte(Ch)==0xFF)
             {
               DisplayTimeOutError();
               if(Scan==1) break;
             }
          }

          if(Delay(Config.Delay_HEX)) break;
        }
        Ascii=0;
        SaveCursorPos(0);
        Hex_Pos=0;
      }

      Ascii=UpChr(Ascii);
      if((Ascii<48) | (Ascii>70) | ((Ascii>57) & (Ascii<65))) Ascii=1;
      else
      {
        HEX[Hex_Pos]=Ascii;

        ShowTextCursor();

        window(13,3,79,11);
        RolBuf[51]=HEX[0];
        RolBuf[52]=HEX[1];

        textattr(15);
        for(I=0; I<53; I++) putch(RolBuf[I]);

        Hex_Pos++;

        if(Hex_Pos>=2)
        {
          RolBuf[51]=HEX[0];
          RolBuf[52]=HEX[1];
          for(I=0; I<53; I++) RolBuf[I]=RolBuf[I+3];

          Hex_Pos=0;
          if(VysliByte(CharToHex())==0xFF)
          {
            DisplayTimeOutError();
            if(Scan==1) break;
          }

          HEX[0]=0; HEX[1]=0;
        }
      }
    }

    if(CharReady())
    {
       window(14,5,78,22);
       LoadCursorPos(1);
       HideTextCursor();
       cprintf("  %.2X ",inp(Config.AdresaPortu) & Config.BitMask);
       SaveCursorPos(1);
     }

    if((Scan==45)&(Ascii==0)) Exit=1;
    if((Scan==1) | (Exit)) break;
  }
  HideTextCursor();
  InitDownMenu("~Alt-X~ Exit            ");
  window(1,1,80,25);
  SetScr(0,11,1,79,23);
}



//------------------------------------------------------------------------
// I/O Hex terminal
// Na hornim okne se zobrazuji hex. cisla zadavane z klavesnice
// Ve spodnim okne se zobrazuji prijate znaky v hex. tvaru

// Kombinaci klaves Ctrl+F1 .. F10 se edituji Bufferu
// Klavesami F1 .. F10 se potom znaky v Bufferech vysilaji postupne
// s definovatelnym zpozdenim ( Nastaveni/Delay pro HEX )
// Buffery jsou odlisne pro Ascii a Hex terminaly
//------------------------------------------------------------------------
void TerminalIOHEX()
{
  unsigned char Hex_Pos,Ch,Break;

  InitPort();
  GetScr(0,11,1,79,23);
  Frame(12,2,79,12,15,0);
  Frame(12,13,79,23,15,0);
  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet  ~F1..F12~ Vlozit buf  ~Ctrl+F1..F12~ Edit buf");
  textattr(15);
  gotoxy(37,2);  cprintf(" Terminal (HEX) Vstup ");
  gotoxy(36,13); cprintf(" Terminal (HEX) Vystup ");
  gotoxy(65,12); cprintf(" KEYB ");
  ShowPortSet();

  ShowTextCursor();

  Cursor_X_0=13; Cursor_Y_0=2;  LoadCursorPos(0);
  Cursor_X_1=13; Cursor_Y_1=13; LoadCursorPos(1);

  Hex_Pos=0;
  Config.CustomHEXBuffer[10][60]=0;
  while(inp(0x60)<128);
  Input_TAB_ENABLED=0;

  while(1)
  {
    if(kbhit())
    {
      GetKey();
      if(Scan==1) break;
      if((Ascii==0) & (Scan==45)) Exit=1;

      if((((Scan>=94) & (Scan<=103)) | (Scan==137) | (Scan==138)) & (Ascii==0))
      {
        LoadCursorPos(0);
        if(Scan<=103) EditCustomHEXBuf(Scan-94);
        else EditCustomHEXBuf(Scan-137+10);
        SaveCursorPos(0);
        Ascii=0;
      }

      if((((Scan>=59) & (Scan<=68)) | (Scan==133)| (Scan==134)) & (Ascii==0))
      {
        textattr(15);
        window(14,3,79,11);
        LoadCursorPos(0);

        if(Hex_Pos!=0)
        {
          HEX[1]=0;
          Ch=CharToHex();
          cprintf("%.2X  ",HEX[1]);

          if(VysliByte(Ch)==0xFF)
          {
            DisplayTimeOutError();
            if(Scan==1) break;
          }
        }

        Hex_Pos=0;
        I=0;
        Break=0;

        while(1)
        {
          if(Scan<=68)
            HEX[0]=Config.CustomHEXBuffer[Scan-59][Hex_Pos];
          else
            HEX[0]=Config.CustomHEXBuffer[Scan-133+10][Hex_Pos];

          if(HEX[0]==0) Break=1;
          Hex_Pos++;

          if(Scan<=68)
            HEX[1]=Config.CustomHEXBuffer[Scan-59][Hex_Pos];
          else
            HEX[1]=Config.CustomHEXBuffer[Scan-133+10][Hex_Pos];

          if(HEX[1]==0) Break=1;
          Hex_Pos++; Hex_Pos++;

          if(inp(0x60)==1) Break=1;

          Ch=CharToHex();

          if(Break) break;
          else
          {
            textattr(15);
            HexToChar(Ch);
            cprintf("%c%c  ",HEX[0],HEX[1]);
            if(VysliByte(Ch)==0xFF)
            {
              DisplayTimeOutError();
              if(Scan==1) break;
            }
          }

          if(Delay(Config.Delay_HEX)) break;
        }
        Ascii=0;
        SaveCursorPos(0);
        Hex_Pos=0;
      }

      Ascii=UpChr(Ascii);
      if((Ascii<48) | (Ascii>70) | ((Ascii>57) & (Ascii<65))) Ascii=1;
      else
      {
        HEX[Hex_Pos]=Ascii;

        ShowTextCursor();

        window(14,3,79,11);
        LoadCursorPos(0);
        textattr(15);
        cprintf("%c",HEX[Hex_Pos]);
        if((Hex_Pos==1) & (Cursor_X_0 < 78)) cprintf("  ");
        SaveCursorPos(0);

        Hex_Pos++;

        if(Hex_Pos>=2)
        {
          Hex_Pos=0;
          if(VysliByte(CharToHex())==0xFF)
          {
            DisplayTimeOutError();
            if(Scan==1) break;
          }
          HEX[0]=0; HEX[1]=0;
        }
      }
    }


    if(CharReady())
    {
       HideTextCursor();
       window(14,14,78,22);
       LoadCursorPos(1);
       cprintf(" %.2X  ",inp(Config.AdresaPortu) & Config.BitMask);
       SaveCursorPos(1);
    }

    if((Scan==45)&(Ascii==0)) Exit=1;
    if((Scan==1) | (Exit)) break;
  }

  HideTextCursor();
  InitDownMenu("~Alt-X~ Exit");
  window(1,1,80,25);
  SetScr(0,11,1,79,23);
}



//------------------------------------------------------------------------
// I/O ASCII/HEX terminal
// Dve okna - Vstupni a Vystupni
// Ve vstupnim ( hornim ) okne jsou zobrazovany znaky z klavesnice ( vysilane )
// Ve vystupnim okne jsou zobrazovany znaky prijate v HEX tvaru

// Kombinaci klaves Ctrl+F1 .. F12 se edituji Bufferu
// Klavesami F1 .. F12 se potom znaky v Bufferech vysilaji postupne
// s definovatelnym zpozdenim ( Nastaveni/Delay pro ASCII )
// Buffery jsou odlisne pro Ascii a Hex terminaly
//------------------------------------------------------------------------
void TerminalIOASCHEX()
{
  unsigned char Ch,Break;

  InitPort();
  GetScr(0,11,1,79,23);
  Frame(12,2,79,12,15,0);
  Frame(12,13,79,23,15,0);
  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet  ~F1..F12~ Vlozit buf  ~Ctrl+F1..F12~ Edit buf");
  textattr(15);
  gotoxy(35,2);  cprintf(" Terminal (ASCII) Vstup ");
  gotoxy(35,13); cprintf(" Terminal (HEX) Vystup ");
  gotoxy(65,12); cprintf(" KEYB ");
  ShowPortSet();

  ShowTextCursor();
  gotoxy(1,1);

  Cursor_X_1=13; Cursor_Y_1=2;  LoadCursorPos(1);
  Cursor_X_2=13; Cursor_Y_2=13; LoadCursorPos(2);

  Config.CustomHEXBuffer[10][60]=0;
  while(inp(0x60)<128);
  Input_TAB_ENABLED=0;

  while(1)
  {
    if(kbhit())
    {
      GetKey();

      //TestEgg(Ascii);

      window(14,3,79,11);
      LoadCursorPos(1);

      if(Scan==1) break;

      if((((Scan>=94) & (Scan<=103)) | (Scan==137) | (Scan==138)) & (Ascii==0))
      {
        if(Scan<=103) CustomASCIIBuf(Scan-94);
        else CustomASCIIBuf(Scan-137+10);
        Ascii=0;
      }

      if((Scan>=59) & (Scan<=68) & (Ascii==0))
      {
        textattr(15);
        for(I=0; Config.CustomASCIIBuffer[Scan-59][I]!=0; I++)
        {
          if(Delay(Config.Delay_Ascii)) break;
          //TestEgg(Config.CustomASCIIBuffer[Scan-59][I]);
          putch(Config.CustomASCIIBuffer[Scan-59][I]);

          if(VysliByte(Config.CustomASCIIBuffer[Scan-59][I])==0xFF)
          {
            DisplayTimeOutError();
            if(Scan==1) break;
          }
        }
        Ascii=0;
      }

      if(((Scan==133) | (Scan==134)) & (Ascii==0))
      {
        textattr(15);
        for(I=0;Config.CustomASCIIBuffer[Scan-133+10][I]!=0;I++)
        {
          if(Delay(Config.Delay_Ascii)) break;
          //TestEgg(Config.CustomASCIIBuffer[Scan-133+10][I]);
          putch(Config.CustomASCIIBuffer[Scan-133+10][I]);

          if(VysliByte(Config.CustomASCIIBuffer[Scan-133+10][I])==0xFF)
          {
            DisplayTimeOutError();
            if(Scan==1) break;
          }
         }
        Ascii=0;
      }

      if(Ascii)
        if(VysliByte(Ascii)==0xFF)
        {
          DisplayTimeOutError();
          if(Scan==1) break;
        }


      switch(Ascii)
      {
       case 0: break;
       case 10:
       case 13: Cursor_Y_1++; Cursor_X_1=12; LoadCursorPos(1); Ascii=0; break;
       case  8: if(Cursor_X_1 > 11)
                {
                  Cursor_X_1--;
                  if(Cursor_X_1 < 12)
                  {
                    if(Cursor_Y_1 > 2) { Cursor_X_1=78; Cursor_Y_1--; }
                    else Cursor_X_1=12;
                  }
                  LoadCursorPos(1);
                }
                Ascii=0; break;
       case  9: break;
       default: cprintf("%c",Ascii); break;
      }
      SaveCursorPos(1);
      if(Cursor_X_1 < 13) Cursor_X_1=13;
      if(Cursor_X_1 > 78) { Cursor_X_1=13; Cursor_Y_1++; LoadCursorPos(1); }
      if(Cursor_Y_1 > 10) { RolWindowUP(12,2,78,10); Cursor_Y_1=10; LoadCursorPos(1); }
    }


    if(CharReady())
    {
       HideTextCursor();
       window(14,14,78,22);
       LoadCursorPos(2);
       cprintf(" %.2X  ",inp(Config.AdresaPortu) & Config.BitMask);
       SaveCursorPos(2);
    }

    if((Scan==45)&(Ascii==0)) Exit=1;
    if((Scan==1) | (Exit)) break;
  }

  HideTextCursor();
  InitDownMenu("~Alt-X~ Exit            ");
  window(1,1,80,25);
  SetScr(0,11,1,79,23);
}




//------------------------------------------------------------------------
// I/O HEX/ASCII terminal
// Dve okna - Vstupni a Vystupni
// Ve vstupnim ( hornim ) okne jsou zobrazovany znaky z klavesnice v hex ( vysilane )
// Ve vystupnim okne jsou zobrazovany znaky prijate

// Kombinaci klaves Ctrl+F1 .. F12 se edituji Bufferu
// Klavesami F1 .. F12 se potom znaky v Bufferech vysilaji postupne
// s definovatelnym zpozdenim ( Nastaveni/Delay pro HEX )
// Buffery jsou odlisne pro Ascii a Hex terminaly
//------------------------------------------------------------------------
void TerminalIOHEXASC()
{
  unsigned char Hex_Pos,Ch,Break;

  InitPort();
  GetScr(0,11,1,79,23);
  Frame(12,2,79,12,15,0);
  Frame(12,13,79,23,15,0);
  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet  ~F1..F12~ Vlozit buf  ~Ctrl+F1..F12~ Edit buf");
  textattr(15);
  gotoxy(35,2);  cprintf(" Terminal (HEX) Vstup ");
  gotoxy(35,13); cprintf(" Terminal (ASCII) Vystup ");
  gotoxy(65,12); cprintf(" KEYB ");
  ShowPortSet();

  ShowTextCursor();

  Cursor_X_0=13; Cursor_Y_0=2;  LoadCursorPos(0);
  Cursor_X_1=13; Cursor_Y_1=13; LoadCursorPos(1);

  Hex_Pos=0;
  Config.CustomHEXBuffer[10][60]=0;
  while(inp(0x60)<128);
  Input_TAB_ENABLED=0;

  while(1)
  {
    if(kbhit())
    {
      GetKey();
      if((Scan==45)&(Ascii==0)) Exit=1;

      window(14,3,79,11);
      LoadCursorPos(1);

      if(Scan==1) break;

      if((((Scan>=94) & (Scan<=103)) | (Scan==137) | (Scan==138)) & (Ascii==0))
      {
        LoadCursorPos(0);
        if(Scan<=103) EditCustomHEXBuf(Scan-94);
        else EditCustomHEXBuf(Scan-137+10);
        SaveCursorPos(0);
        Ascii=0;
      }

      if((((Scan>=59) & (Scan<=68)) | (Scan==133)| (Scan==134)) & (Ascii==0))
      {
        textattr(15);
        LoadCursorPos(0);

        if(Hex_Pos!=0)
        {
          HEX[1]=0;
          Ch=CharToHex();
          cprintf("%.2X  ",HEX[1]);

          if(VysliByte(Ch)==0xFF)
          {
            DisplayTimeOutError();
            if(Scan==1) break;
          }
        }

        Hex_Pos=0;
        I=0;
        Break=0;

        while(1)
        {
          if(Scan<=68)
            HEX[0]=Config.CustomHEXBuffer[Scan-59][Hex_Pos];
          else
            HEX[0]=Config.CustomHEXBuffer[Scan-133+10][Hex_Pos];

          if(HEX[0]==0) Break=1;
          Hex_Pos++;

          if(Scan<=68)
            HEX[1]=Config.CustomHEXBuffer[Scan-59][Hex_Pos];
          else
            HEX[1]=Config.CustomHEXBuffer[Scan-133+10][Hex_Pos];

          if(HEX[1]==0) Break=1;
          Hex_Pos++; Hex_Pos++;

          if(inp(0x60)==1) Break=1;

          Ch=CharToHex();

          if(Break) break;
          else
          {
            textattr(15);
            HexToChar(Ch);
            cprintf("%c%c  ",HEX[0],HEX[1]);
            if(VysliByte(Ch)==0xFF)
            {
              DisplayTimeOutError();
              if(Scan==1) break;
            }
          }

          if(Delay(Config.Delay_HEX)) break;
        }
        Ascii=0;
        SaveCursorPos(0);
        Hex_Pos=0;
      }

      Ascii=UpChr(Ascii);
      if((Ascii<48) | (Ascii>70) | ((Ascii>57) & (Ascii<65))) Ascii=1;
      else
      {
        HEX[Hex_Pos]=Ascii;

        ShowTextCursor();

        window(14,3,79,11);
        LoadCursorPos(0);
        textattr(15);
        cprintf("%c",HEX[Hex_Pos]);
        if((Hex_Pos==1) & (Cursor_X_0 < 78)) cprintf("  ");
        SaveCursorPos(0);

        Hex_Pos++;

        if(Hex_Pos>=2)
        {
          Hex_Pos=0;
          if(VysliByte(CharToHex())==0xFF)
          {
            DisplayTimeOutError();
            if(Scan==1) break;
          }
          HEX[0]=0; HEX[1]=0;
        }
      }
    }


    if(CharReady())
    {
       HideTextCursor();
       window(13,14,79,22);
       LoadCursorPos(1);
       Byte=FilterChar(inp(Config.AdresaPortu) & Config.BitMask);
       cprintf("%c",Byte);
       SaveCursorPos(1);
    }

    if((Scan==1) | (Exit)) break;
  }
  HideTextCursor();
  InitDownMenu("~Alt-X~ Exit            ");
  window(1,1,80,25);
  SetScr(0,11,1,79,23);
}




//------------------------------------------------------------------------
// Prekresleni menu T2COM / Nastaveni
//------------------------------------------------------------------------
void RefreshT2CSetMenu()
{
  #define T2CSetMenuItems 4
  #define T2CSMENU_X 20
  #define T2CSMENU_Y 10

  ResetMenuItems();
  MenuItems[ 0].Y=T2CSMENU_Y+0; MenuItems[ 0].Item="Port 1 (Horni okno)                 \0x0";
  MenuItems[ 1].Y=T2CSMENU_Y+1; MenuItems[ 1].Item="Port 2 (Dolni okno)                 \0x0";
  MenuItems[ 2].Y=T2CSMENU_Y+2; MenuItems[ 2].Item="Povoleni odesilani znaku na port 1  \0x0";
  MenuItems[ 3].Y=T2CSMENU_Y+3; MenuItems[ 3].Item="Typ vypisu v hornim okne            \0x0";
  MenuItems[ 4].Y=T2CSMENU_Y+4; MenuItems[ 4].Item="Typ vypisu ve spodnim okne          \0x0";

  for(I=0; I<=T2CSetMenuItems; I++)
  {
    if(NT2CPol==I) textattr(10); else textattr(16+32+64);
    gotoxy(T2CSMENU_X,MenuItems[I].Y);
    cprintf(" %s   ",MenuItems[I].Item);

    if(NT2CPol==0) textattr(10); else textattr(16+32+64);
    gotoxy(T2CSMENU_X+34,MenuItems[0].Y);
    cprintf("COM%d / %.4Xh",Config.T2C_PORT1+1,AdrPort(Config.T2C_PORT1));

    if(NT2CPol==1) textattr(10); else textattr(16+32+64);
    gotoxy(T2CSMENU_X+34,MenuItems[1].Y);
    cprintf("COM%d / %.4Xh",Config.T2C_PORT2+1,AdrPort(Config.T2C_PORT2));

    if(NT2CPol==2) textattr(10); else textattr(16+32+64);
    gotoxy(T2CSMENU_X+40,MenuItems[2].Y);
    if(Config.T2C_ENABLE_SEND) cprintf("  Ano "); else cprintf("   Ne ");

    if(NT2CPol==3) textattr(10); else textattr(16+32+64);
    gotoxy(T2CSMENU_X+40,MenuItems[3].Y);
    if(Config.T2C_TYPP1) cprintf("  Hex "); else cprintf("Ascii ");

    if(NT2CPol==4) textattr(10); else textattr(16+32+64);
    gotoxy(T2CSMENU_X+40,MenuItems[4].Y);
    if(Config.T2C_TYPP2) cprintf("  Hex "); else cprintf("Ascii ");
  }
}



//------------------------------------------------------------------------
// Menu T2COM / Nastaveni
//------------------------------------------------------------------------
void T2C_Nastaveni()
{
  HideTextCursor();
  SaveCursorPos(0);
  window(1,1,80,25);
  GetScr(2,T2CSMENU_X-2,T2CSMENU_Y-2,T2CSMENU_X+55,T2CSMENU_Y+T2CSetMenuItems);
  GetScr(3,0,24,79,24);
  InitDownMenu("~ESC/ENTER~ OK  ~PGUP/PGDN~ Change");
  Frame(T2CSMENU_X-1,T2CSMENU_Y-1,T2CSMENU_X+45,T2CSMENU_Y+T2CSetMenuItems+1,16+32+64,0);
  gotoxy(T2CSMENU_X+18,T2CSMENU_Y-1); printf(" Nastaveni ");
  while(1)
  {
    RefreshT2CSetMenu();
    GetKey();
    switch(Scan)
    {
      case 72: if(NT2CPol) NT2CPol--; break;                     // UP
      case 80: if(NT2CPol < T2CSetMenuItems) NT2CPol++; break;   // DN
      case 71: NT2CPol=0; break;                                 // Home
      case 79: NT2CPol=T2CSetMenuItems; break;                   // End
      case 81: switch(NT2CPol)                                   // PgDn
               {
                 case 0: if(Config.T2C_PORT1 < 4)
                           Config.T2C_PORT1++; break;
                 case 1: if(Config.T2C_PORT2 < 4)
                           Config.T2C_PORT2++; break;
                 case 2: Config.T2C_ENABLE_SEND=!Config.T2C_ENABLE_SEND; break;
                 case 3: if(!Config.T2C_TYPP1) Config.T2C_TYPP1=1; break;
                 case 4: if(!Config.T2C_TYPP2) Config.T2C_TYPP2=1; break;
               }
               break;
      case 73: switch(NT2CPol)                                   // PgUp
               {
                 case 0: if(Config.T2C_PORT1)
                           Config.T2C_PORT1--; break;
                 case 1: if(Config.T2C_PORT2)
                           Config.T2C_PORT2--; break;
                 case 2: Config.T2C_ENABLE_SEND=!Config.T2C_ENABLE_SEND; break;
                 case 3: Config.T2C_TYPP1=!Config.T2C_TYPP1; break;
                 case 4: Config.T2C_TYPP2=!Config.T2C_TYPP2; break;
               }
               break;
    }
    if((Scan==1) | (Ascii==13)) break;
  }
  Scan=0;
  SetScr(2,T2CSMENU_X-2,T2CSMENU_Y-2,T2CSMENU_X+55,T2CSMENU_Y+T2CSetMenuItems);
  SetScr(3,0,24,79,24);
  LoadCursorPos(0);
}



void ShowT2CPortSet()
{
  textattr(15);
  gotoxy(40,2);  cprintf(" COM%d / %.4Xh ",Config.T2C_PORT1+1,AdrPort(Config.T2C_PORT1));
  gotoxy(40,13); cprintf(" COM%d / %.4Xh ",Config.T2C_PORT2+1,AdrPort(Config.T2C_PORT2));

  gotoxy(30,23); cprintf(" %d0 Bd ", PortSpeed[Config.Speed] / 10);
  gotoxy(30,12); cprintf(" %d0 Bd ", PortSpeed[Config.Speed] / 10);

  gotoxy(50,23); cprintf(" %d",Config.PocBit);
  switch(Config.Parita)
  {
    case 0: cprintf(" N "); break;
    case 1: cprintf(" S "); break;
    case 2: cprintf(" L "); break;
    case 3: cprintf(" V0 "); break;
    case 4: cprintf(" V1 "); break;
  }
  cprintf("%d ",Config.StpBit+1);

  gotoxy(50,12); cprintf(" %d",Config.PocBit);
  switch(Config.Parita)
  {
    case 0: cprintf(" N "); break;
    case 1: cprintf(" S "); break;
    case 2: cprintf(" L "); break;
    case 3: cprintf(" V0 "); break;
    case 4: cprintf(" V1 "); break;
  }
  cprintf("%d ",Config.StpBit+1);
}




//------------------------------------------------------------------------
// 2xCOM terminal
// Dve okna - Vstupni/Vystupni a Vstupni
// V hornim okne (Vstupnim/Vystupnim) jsou zobrazovany prichozi znaky,
// podle nastaveni bud v Ascii nebo v Hex forme, znaky z klavesnice
// jsou pri povolenem odesilani odesilany na port1 v ascii forme.
// Ve spodnim okne jsou zobrazovany prijate znaky.

// Kombinaci klaves Ctrl+F1 .. F12 se edituji Bufferu
// Klavesami F1 .. F12 se potom znaky v Bufferech vysilaji postupne
// s definovatelnym zpozdenim ( Nastaveni/Delay pro HEX/Ascii )
// Buffery jsou odlisne pro Ascii a Hex terminaly
//------------------------------------------------------------------------
void Terminal2COM()
{
  unsigned char Ch,Break;
  unsigned int OldPort,OldPortAdr;
  unsigned int Port1Adr,Port2Adr;
  unsigned char Hex_Pos;

  GetScr(0,11,1,79,23);
  Frame(12,2,79,12,15,0);
  Frame(12,13,79,23,15,0);
  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet  ~Ctrl+N~ Set  ~Ctrl+F1..F12~ EditBuf  ~F1..F12~ VlozitBuf");
  textattr(15);

  OldPortAdr=Config.AdresaPortu;
  OldPort=Config.Port;

  Port1Adr=AdrPort(Config.T2C_PORT1);
  Port2Adr=AdrPort(Config.T2C_PORT2);

  Config.Port=Config.T2C_PORT2;
  Config.AdresaPortu=Port2Adr; InitPort();

  Config.Port=Config.T2C_PORT1;
  Config.AdresaPortu=Port1Adr; InitPort();

  ShowT2CPortSet();

  ShowTextCursor();
  gotoxy(1,1);

  Cursor_X_1=13; Cursor_Y_1=2;  LoadCursorPos(1);
  Cursor_X_2=13; Cursor_Y_2=13; LoadCursorPos(2);

  while(1)
  {
    if(kbhit())
    {
      GetKey();
      if(Scan==1) break;

      if((Scan==49) & (Ascii==14))
      {
        T2C_Nastaveni();
        Port1Adr=AdrPort(Config.T2C_PORT1);
        Port2Adr=AdrPort(Config.T2C_PORT2);

        Config.Port=Config.T2C_PORT1;
        Config.AdresaPortu=Port1Adr; InitPort();

        Config.Port=Config.T2C_PORT2;
        Config.AdresaPortu=Port2Adr; InitPort();
        if(Ascii!=27)
        {
          Frame(12,2,79,12,15,0);
          Frame(12,13,79,23,15,0);
          Cursor_X_1=13; Cursor_Y_1=2;  LoadCursorPos(1);
          Cursor_X_2=13; Cursor_Y_2=13; LoadCursorPos(2);
        }
        ShowT2CPortSet();
        textattr(15);
        Ascii=0;
      }

      if((((Scan>=94) & (Scan<=103)) | (Scan==137) | (Scan==138)) & (Ascii==0))
      {
        Config.AdresaPortu=Port1Adr;
        if(!Config.T2C_TYPP1)
        {
          if(Scan<=103) CustomASCIIBuf(Scan-94);
          else CustomASCIIBuf(Scan-137+10);
        }
        else
        {
          if(Scan<=103) EditCustomHEXBuf(Scan-94);
          else EditCustomHEXBuf(Scan-137+10);
        }
        Ascii=0;
      }


      if((((Scan>=59) & (Scan<=68)) | (Scan==133) | (Scan==134)) & (Ascii==0))
      {
        Config.AdresaPortu=Port1Adr;
        if(!Config.T2C_TYPP1)
        {
          if(Scan<=68)
          for(I=0; Config.CustomASCIIBuffer[Scan-59][I]!=0; I++)
          {
            if(Delay(Config.Delay_Ascii)) break;
            if(VysliByte(Config.CustomASCIIBuffer[Scan-59][I])==0xFF)
            {
              DisplayTimeOutError();
              if(Scan==1) break;
            }
          }
          else
          for(I=0; Config.CustomASCIIBuffer[Scan-123][I]!=0; I++)
          {
            if(Delay(Config.Delay_Ascii)) break;
            if(VysliByte(Config.CustomASCIIBuffer[Scan-123][I])==0xFF)
            {
              DisplayTimeOutError();
              if(Scan==1) break;
            }
          }
        }
        else
        {
          if(Hex_Pos!=0)
          {
            HEX[1]=0;
            Ch=CharToHex();

            if(VysliByte(Ch)==0xFF)
            {
              DisplayTimeOutError();
              if(Scan==1) break;
            }
          }

          Hex_Pos=0;
          I=0;
          Break=0;

          while(1)
          {
            if(Scan<=68)
              HEX[0]=Config.CustomHEXBuffer[Scan-59][Hex_Pos];
            else
              HEX[0]=Config.CustomHEXBuffer[Scan-123][Hex_Pos];

            if(HEX[0]==0) Break=1;
            Hex_Pos++;

            if(Scan<=68)
              HEX[1]=Config.CustomHEXBuffer[Scan-59][Hex_Pos];
            else
              HEX[1]=Config.CustomHEXBuffer[Scan-123][Hex_Pos];

            if(HEX[1]==0) Break=1;
            Hex_Pos++; Hex_Pos++;

            if(inp(0x60)==1) Break=1;

            Ch=CharToHex();

            if(Break) break;
            else
            {
              HexToChar(Ch);
              if(VysliByte(Ch)==0xFF)
              {
                DisplayTimeOutError();
                if(Scan==1) break;
              }
            }

            if(Delay(Config.Delay_HEX)) break;
          }
          Hex_Pos=0;
        }
        Ascii=0;
      }


      if(Config.T2C_ENABLE_SEND)
      {
        if(Ascii)
        {
          Config.AdresaPortu=Port1Adr;
          if(VysliByte(Ascii)==0xFF)
          {
            DisplayTimeOutError();
            if(Scan==1) break;
            Ascii=0;
          }
        }
      }
    }

    Config.AdresaPortu=Port1Adr;
    if(CharReady())   // charready Port1
    {
       HideTextCursor();
       window(14,3,78,11);
       LoadCursorPos(1);
       if(Config.T2C_TYPP1) cprintf(" %.2X  ",inp(Config.AdresaPortu) & Config.BitMask);
       else
       {
         Byte=FilterChar(inp(Config.AdresaPortu) & Config.BitMask);
         cprintf("%c",Byte);
       }
       SaveCursorPos(1);
    }


    Config.AdresaPortu=Port2Adr;
    if(CharReady())   // Znak pripraven - Port2
    {
       HideTextCursor();
       window(14,14,78,22);
       LoadCursorPos(2);
       if(Config.T2C_TYPP2) cprintf(" %.2X  ",inp(Config.AdresaPortu) & Config.BitMask);
       else
       {
         Byte=FilterChar(inp(Config.AdresaPortu) & Config.BitMask);
         cprintf("%c",Byte);
       }
       SaveCursorPos(2);
    }

    if((Scan==45) & (Ascii==0)) Exit=1;
    if((Scan==1) | (Exit)) break;
  }

  Config.AdresaPortu=OldPortAdr;     // Obnoveni puvodni adresy portu
  Config.Port=OldPort;               // Obnoveni puvodniho cisla portu
  HideTextCursor();
  ShowHint(0x0);
  window(1,1,80,25);
  SetScr(0,11,1,79,23);
}




//------------------------------------------------------------------------
//  Menu Nastaveni
//------------------------------------------------------------------------
void Nastaveni()
{
  GetScr(0,10,8,44,23);
  ShowHint("\0x0");
  Frame(12,9,33,11+SetMenuItems,16+7,0);
  gotoxy(11,11); cprintf("Æµ");
  gotoxy(18,9); printf(" Nastaveni ");
  RefreshSetMenu();
  while(1)
  {
     InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet  ~ENTER~ Ok");
     switch(SPol)
     {
       case 0: ShowHint("Vyber serioveho portu\0x0"); break;
       case 1: ShowHint("Nastaveni rychlosti serioveho portu\0x0"); break;
       case 2: ShowHint("Nastaveni portu\0x0"); break;
       case 3: ShowHint("Povoli/Zakaze zobrazeni vybranych znaku\0x0"); break;
       case 4: ShowHint("Prichozi znak je nejprve \"ANDovan\" s timto bytem\0x0"); break;
       case 5: ShowHint("Nastaveni zpozdeni mezi jednotlivymi znaky pri vysilani bufferu ( ASCII )\0x0"); break;
       case 6: ShowHint("Nastaveni zpozdeni mezi jednotlivymi znaky pri vysilani bufferu ( HEX )\0x0"); break;
       case 7: ShowHint("Zpusob zjisteni adres portu COM\0x0"); break;
       case 8: ShowHint("Povoleni / Zakaz otevreni okna terminalu po spusteni \0x0"); break;
       case 9: ShowHint("Povoleni / Zakaz zobrazeni ukoncovaciho potvrzeni \0x0"); break;
     }

     RefreshSetMenu();

     GetKey();
     if((Scan==72) & (SPol > 0)) SPol--;
     if((Scan==80) & (SPol < SetMenuItems)) SPol++;
     if(Scan==71) SPol=0;
     if(Scan==79) SPol=SetMenuItems;

     if((Scan==28) | (Scan==77))
     {
        switch( SPol )
        {
          case 0:  SelectPort();     break;
          case 1:  SelectSpeed();    break;
          case 2:  SetPort();        break;
          case 3:  DisplayChars();   break;
          case 4:  SetBitMask();     break;
          case 5:  SetDelayASCII();  break;
          case 6:  SetDelayHEX();    break;
          case 7:  FixPortAdress();  break;
          case 8:  AutoOpen();       break;
          case 9:  MenuExitDialog(); break;
          default:                   break;
        }
     }
     if((Scan==45)&(Ascii==0)) Exit=1;
     if((Scan==75) | (Scan==1) | (Exit)) break;
  }
  SetScr(0,10,8,44,23);
  RefreshMainMenu();
}



//------------------------------------------------------------------------
// Menu Utility
//------------------------------------------------------------------------
void Utility()
{
  GetScr(3,10,9,53,23);
  ShowHint("\0x0");
  Frame(12,10,36,11+UtilsMenuItems,16+7,0);
  gotoxy(11,12); cprintf("Æµ");
  gotoxy(21,10); printf(" Utility ");
  while(1)
  {
     InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet  ~ENTER~ Ok");
     switch(UPol)
     {
       case 0: ShowHint("Vysila nepretrzite vybrany znak.\0x0"); break;
       case 1: ShowHint("Zobrazuje prijate znaky bez cekani na dalsi\0x0"); break;
       case 2: ShowHint("Zobrazuje prijate znaky, ceka na dalsi\0x0"); break;
       case 3: ShowHint("Posila prichozi znaky na COM1 a COM2 zpet\0x0"); break;
     }
     RefreshUtilsMenu();
     GetKey();
     if((Scan==72) & (UPol>0)) UPol--;                   // Up
     if((Scan==80) & (UPol<UtilsMenuItems-1)) UPol++;    // Down
     if(Scan==71) UPol=0;                                // Home
     if(Scan==79) UPol=UtilsMenuItems-1;                 // End


     if((Scan==28) | (Scan==77))
     {
        switch( UPol )
        {
          case 0:  SendNonstop(); break;
          case 1:  ViewCOM();     break;
          case 2:  ViewReceive(); break;
          case 3:  SetScr(3,10,9,53,23); EchoMode(); break;
          default: break;
        }
     }
     if((Scan==45)&(Ascii==0)) Exit=1;
     if((Scan==75) | (Scan==1) | (Exit)) break;
  }
  SetScr(3,10,9,53,23);
  RefreshMainMenu();
}



//------------------------------------------------------------------------
// Detekce modemu
//------------------------------------------------------------------------
void ModemDetect()
{
  unsigned int T;
  unsigned char MStav;
  unsigned char OKindex;

  GetScr(0,11,1,79,23);
  Frame(12,2,79,23,15,0);
  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet");
  textattr(15);
  gotoxy(38,2); cprintf(" Detekce modemu ");
  ShowPortSet();
  window(13,3,79,22);
  ShowTextCursor();

  MStav=0; I=0;

  while(1)
  {
    switch(MStav)
    {
      case 0:  Config.AdresaPortu=AdrPort(I);
               cprintf("Port %.3X ... ");
               MStav=1;
               break;

      case 1:  if(Config.AdresaPortu)
               {
                 MStav=2; T=0;
               }
               else
               {
                 cprintf("Adresa portu je 0h.\n");
                 I++; if(I>5) break;
                 MStav=0;
               }

      case 2:  if(SendReady()) MStav=3;
               else
               {
                 T++;
                 if(T>10000)
                 {
                   cprintf("Zadna odpoved !\n\r",Config.AdresaPortu);
                   I++; if(I>5) break;
                   MStav=0;
                 }
               }
               break;

      case 3: if(VysliByte('A')==0xFF) DisplayTimeOutError();
              if(VysliByte('T')==0xFF) DisplayTimeOutError();
              if(VysliByte(0x0D)==0xFF) DisplayTimeOutError();
              T=0;
              MStav=4;
              break;

      case 4: if(CharReady())
              {
                if(inp(Config.AdresaPortu)=='O') MStav=5;
                else
                {
                  T++; if(T>20000) MStav=0;
                }
              }
              break;

      case 5: if(CharReady())
              {
                if(inp(Config.AdresaPortu)=='K') MStav=6;
                else
                {
                  T++; if(T>20000) MStav=0;
                }
              }
              break;

      case 6: cprintf("Modem detected !");
              break;

      case 7:  if(kbhit())
               {
                 GetKey();
                 if(Scan==1) MStav=8;
               }
               break;

    }
    if(MStav==8) break;
  }
  Config.AdresaPortu=AdrPort(Config.Port);
  SetScr(0,11,1,79,23);
}





//------------------------------------------------------------------------
// Reset modemu
//------------------------------------------------------------------------
void ModemReset()
{

}



//------------------------------------------------------------------------
// Menu Modem
//------------------------------------------------------------------------
void ModemMenu()
{
  GetScr(3,10,13,53,23);
  ShowHint("\0x0");
  Frame(12,14,28,15+ModemMenuItems,16+7,0);
  gotoxy(11,15); cprintf("Æµ");
  gotoxy(17,14); printf(" Modem ");
  while(1)
  {
     InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet  ~ENTER~ Ok");
     switch(MPol)
     {
       case 0: ShowHint("Projde porty a pokusi se nalezt port s modemem.\0x0"); break;
       case 1: ShowHint("Odesle na port modemu nastavujici prikazy.\0x0"); break;
     }
     RefreshModemMenu();
     GetKey();
     if((Scan==72) & (MPol > 0)) MPol--;                   // Up
     if((Scan==80) & (MPol < ModemMenuItems-1)) MPol++;    // Down
     if(Scan==71) MPol=0;                                  // Home
     if(Scan==79) MPol=ModemMenuItems-1;                   // End


     if((Scan==28) | (Scan==77))
     {
        switch( MPol )
        {
          case 0: ModemDetect(); break;
          case 1: ModemReset();  break;
          default: break;
        }
     }
     if((Scan==45) & (Ascii==0)) Exit=1;
     if((Scan==75) | (Scan==1) | (Exit)) break;
  }
  SetScr(3,10,13,53,23);
  RefreshMainMenu();
}




//------------------------------------------------------------------------
//  Nacte status portu
//------------------------------------------------------------------------
void GetPortStatus()
{
  PortStatus=inp(Config.AdresaPortu+5);
}


//------------------------------------------------------------------------
// Pri ztrate predchoziho znaku vraci 2
//------------------------------------------------------------------------
unsigned char OverrunError()
{
  return(PortStatus & 2);
}



//------------------------------------------------------------------------
// Pri chybne parite vraci 4
//------------------------------------------------------------------------
unsigned char ParityError()
{
  return(PortStatus & 4);
}


//------------------------------------------------------------------------
// Pri chybnem stopbitu vraci 8
//------------------------------------------------------------------------
unsigned char StopbitError()
{
  return(PortStatus & 8);
}



//------------------------------------------------------------------------
// Pokud je prijmana trvala nula, vraci 16
//------------------------------------------------------------------------
unsigned char AlwaysZero()
{
  return(PortStatus & 16);
}




//------------------------------------------------------------------------
// Zobrazi pres hlavni menu seznam klavesovych zkratek
//------------------------------------------------------------------------
void KeybHelp(unsigned char SH)
{
  if(SH)
  {
    _KeybHelpStatus=1;
    SaveCursorPos(3);
    window(1,1,80,25);
    GetScr(3,19,4,69,20);
    Frame(20,6,68,21,16+32+64,0);

    gotoxy(34,6); cprintf(" Klavesove zkratky ");

    gotoxy(22,7);
    textattr(4+16+32+64); cprintf("SHIFT+F1     ");
    textattr(16+32+64);   cprintf("Zobrazit / Skryt ");

    gotoxy(22,8);
    textattr(4+16+32+64); cprintf("F1..F12      ");
    textattr(16+32+64);   cprintf("Odeslat buffer");

    gotoxy(22,9);
    textattr(4+16+32+64); cprintf("Alt+F1..F12  ");
    textattr(16+32+64);   cprintf("Vybrat buffer");

    gotoxy(22,10);
    textattr(4+16+32+64); cprintf("CTRL+F1..F12 ");
    textattr(16+32+64);   cprintf("Editovat buffer");

    gotoxy(22,11);
    textattr(4+16+32+64); cprintf("CTRL+L       ");
    textattr(16+32+64);   cprintf("Seznam zprav");

    gotoxy(22,12);
    textattr(4+16+32+64); cprintf("CTRL+T       ");
    textattr(16+32+64);   cprintf("Start/Stop casoveho odesilani");

    gotoxy(22,13);
    textattr(4+16+32+64); cprintf("CTRL+N       ");
    textattr(16+32+64);   cprintf("Nastaveni");

    gotoxy(22,14);
    textattr(4+16+32+64); cprintf("Ctrl+R       ");
    textattr(16+32+64);   cprintf("Vynulovani prijatych znaku");

    gotoxy(22,15);
    textattr(4+16+32+64); cprintf("Ctrl+O       ");
    textattr(16+32+64);   cprintf("Vynulovani odeslanych znaku");

    gotoxy(22,16);
    textattr(4+16+32+64); cprintf("Alt+T        ");
    textattr(16+32+64);   cprintf("Aktualni timeout");

    gotoxy(22,17);
    textattr(4+16+32+64); cprintf("CTRL+D       ");
    textattr(16+32+64);   cprintf("Vynulovat vse, vymazat obsah oken");

    gotoxy(22,18);
    textattr(4+16+32+64); cprintf("P            ");
    textattr(16+32+64);   cprintf("Stop/Start vypisovani zprav");

    gotoxy(22,19);
    textattr(4+16+32+64); cprintf("CTRL+I       ");
    textattr(16+32+64);   cprintf("Zobrazi pocet prepsanych znaku");

    gotoxy(22,20);
    textattr(4+16+32+64); cprintf("Alt+S        ");
    textattr(16+32+64);   cprintf("Zobrazi statistiku komunikace");

    LoadCursorPos(3);
    ShowHint("Vypisovani zprav pozastaveno... ");
    _PauseReceive=1;
  }
  else
  {
    SetScr(3,19,4,69,20);
    _KeybHelpStatus=0;
    _PauseReceive=0;
    ShowHint("Vypisovani zprav obnoveno... ");
  }
}



//------------------------------------------------------------------------
//  Prevede cisla z _Buffer na hexadecimalni text Text
//  _Buffer ... cislo bufferu pro CustomDistelBuffer
//  N ... pocet prevedenych cisel
// PocPozice ... Pozice _Bufferu od ktere se zacnou nacitat data
//------------------------------------------------------------------------
void ConvertHexToText(char _Buffer, unsigned char N, char PocPozice)
{
  unsigned int HI;
  unsigned int TI;

  HI=PocPozice; TI=0;
  while(1)
  {
    HexToChar(Config.CustomDistelBuffer[_Buffer][HI]); HI++;
    Text[TI]=HEX[0]; TI++;
    Text[TI]=HEX[1]; TI++;
    if(HI>=N) break;
    Text[TI]=' '; TI++;
  }
  Text[TI]=0;
}



//------------------------------------------------------------------------
//  Prevede hedadecimalni text Text na cisla do _Buffer
//  _Buffer ... cislo bufferu pro CustomDistelBuffer
// PocPozice ... Pozice _Bufferu od ktere se zapisou prevedena data
//------------------------------------------------------------------------
void ConvertTextToHex(char _Buffer, char PocPozice)
{
  unsigned int HI;
  unsigned int TI;

  HI=PocPozice; TI=0;
  while(1)
  {
    HEX[0]=Text[TI]; TI++;
    HEX[1]=Text[TI]; TI++; TI++;
    Config.CustomDistelBuffer[_Buffer][HI]=CharToHex(); HI++;
    if(Text[TI]==0) break;
  }
}




//------------------------------------------------------------------------
//  Zjisti aktualni kontrolni soucet bufferu a zapise ho do bufferu
//------------------------------------------------------------------------
void GetCRC(unsigned char _Buffer)
{
  unsigned char CS;

  CS=0;
  for(I=0; I< (Config.CustomDistelBuffer[_Buffer][1]+2); I++)
  {
    CS=(unsigned char) (CS + Config.CustomDistelBuffer[_Buffer][I]);
  }
  Config.CustomDistelBuffer[_Buffer][I]=CS;
}




//------------------------------------------------------------------------
//  Na souradnicich X0,Y0,X1,Y1 vytvori hexadecimalni editacni pole
//  _Buffer je cislo editovaneho bufferu
//  Attr je barva znaku
//  Velikost pole je vypocitana automaticky, pozor na velikost bufferu
//------------------------------------------------------------------------
void InputHexArray(char X0,char Y0,char X1,char Y1,char Attr,char _Buffer, char OnlyView)
{
   unsigned int   I, ArraySize; //, ArrayPos;
   unsigned char  CurX, CurY;

   window(X0,Y0,X1,Y1);
   textattr(Attr);
   clrscr();

   ArraySize=(((X1-X0+1)*(Y1-Y0+1))) / 3;
   CurX=0; CurY=0;

   ConvertHexToText(_Buffer,ArraySize+3, 3);

   while(1)
   {
     I=0;

     while(1)  // Test spravnosti hexa cisel v textu
     {
       if (!(((Text[I]>='0') & (Text[I]<='9')) | ((Text[I]>='A') & (Text[I]<='F'))))
       { Text[I]='0'; }    // Prvni cislo
       I++;
       if (!(((Text[I]>='0') & (Text[I]<='9')) | ((Text[I]>='A') & (Text[I]<='F'))))
       { Text[I]='0'; }    // Druhe cislo
       I++;
       if(I>=(ArraySize-1)*3) break;
       if (Text[I]!=0x20) Text[I]=0x20;
       I++;                // Mezera
     }

     gotoxy(1,1); cputs(Text);
     HideTextCursor();
     //for(ArrayPos=0; ArrayPos < (ArraySize*3-1); ArrayPos++)
     //{
     //  if(ArrayPos>Config.CustomDistelBuffer[_Buffer][2])
     //    textcolor(7);
     //  else
     //    textcolor(15);
     //  putch(Text[ArrayPos]);
     //}
     gotoxy(CurX+1,CurY+1);
     ShowTextCursor();
     if(!OnlyView) GetKey();

     if((Scan==46) & (Ascii==3))
     { for(I=0; I<sizeof(Text); I++) Text[I]=0; }

     if(!OnlyView)
     if((Ascii==224) | (Ascii==0) | (Scan==14))
     switch(Scan)
     {
       case 72: if(CurY) CurY--;          break;    // Up
       case 80: if(CurY<(Y1-Y0)) CurY++;  break;    // Down
       case 75: if(CurX)                            // Left
                {
                  CurX--;
                  if(Text[CurY*((X1-X0)+1) + CurX]==' ') CurX--;
                }
                break;
       case 77: if(CurX<(X1-X0)-1)                  // Right
                {
                  CurX++;
                  if(Text[CurY*((X1-X0)+1) + CurX]==' ') CurX++;
                }
                break;
       case 71: CurY=0; CurX=0;           break;    // Home
       case 79: CurY=Y1-Y0; CurX=X1-X0;   break;    // End
       case 73: CurX=0;                   break;    // PgUp
       case 81: CurX=(X1-X0);             break;    // PgDn

       case 14: if(CurX) CurX--;                    // Backspace
                if(Config.CustomDistelBuffer[_Buffer][2]-1)
                  Config.CustomDistelBuffer[_Buffer][2]--;
                if(Text[CurY*((X1-X0)+1) + CurX]==' ') CurX--;
                Text[CurY*((X1-X0)+1) + CurX]='0';
                if((CurX==0) & CurY>0) { CurY--; CurX=X1-X0; }
                break;
     }
     if(Ascii!=0)
     if(((UpChr(Ascii)>='A') & (UpChr(Ascii)<='F')) |
        ((UpChr(Ascii)>='0') & (UpChr(Ascii)<='9')))
     {
       if((CurX < (X1-X0)) & (CurY <= (Y1-Y0)))
       {
         Text[CurY*((X1-X0)+1) + CurX]=UpChr(Ascii);
         CurX++;
         if(Text[CurY*((X1-X0)+1) + CurX]==0x20)
         CurX++;

         if((CurX > (X1-X0)) & (CurY < (Y1-Y0)))
         {
           CurY++; CurX=0;
         }
       }
     }

     ConvertTextToHex(_Buffer, 3);
     GetCRC(_Buffer);
     ConvertHexToText(_Buffer, ArraySize+3, 3);

     if(OnlyView) { Scan=0; break; }
     if((Scan==1) | (Ascii==13)) break;
     if((Input_TAB_ENABLED) & (Scan==15)) break;
   }
   window(1,1,80,25);
   ConvertTextToHex(_Buffer, 3);
}




//------------------------------------------------------------------------
void EditDistelBuffer(unsigned char _Buffer)
{
  GetScr(4,12,04,65,23);
  GetScr(5,0,24,79,25);
  SaveCursorPos(0);
  window(1,1,80,25);
  InitDownMenu("~Alt-X~ Exit  ~ESC~ Zpet  ~ENTER~ Ok  ~TAB~ Dalsi par  ~Ctrl+C~ Clear");
  Frame(15,05,60,21,16+32+64,0);
  gotoxy(30,5);  cprintf(" Editace prikazu ");
  gotoxy(33,21); cprintf(" Buffer %d ",_Buffer+1);
  gotoxy(21,7);  cprintf("Adresa zarizeni");
  gotoxy(41,7);  cprintf("Delka zpravy");
  gotoxy(23,10); cprintf("ID prikazu");
  gotoxy(39,10); cprintf("Kontrolni soucet");
  gotoxy(17,14); cprintf("Zprava");
  gotoxy(28,14); cprintf("Popis zpravy:");

  if(!Config.CustomDistelBuffer[_Buffer][1])
      Config.CustomDistelBuffer[_Buffer][1]=1;


  textattr(16+15);
  gotoxy(26,8);  cprintf(" %.2X ",Config.CustomDistelBuffer[_Buffer][0]);
  gotoxy(45,8);  cprintf(" %.2X ",Config.CustomDistelBuffer[_Buffer][1]);
  gotoxy(26,11); cprintf(" %.2X ",Config.CustomDistelBuffer[_Buffer][2]);

  textattr(16+32+64);
  gotoxy(45,11); cprintf(" %2Xh ",Config.CustomDistelBuffer[_Buffer][2+Config.CustomDistelBuffer[_Buffer][1]]);

  InputHexArray(16,15,60,20,16+15,_Buffer,1);
  textattr(1+16+32+64);
  gotoxy(42,14); cprintf("%s",Config.DistelBufferPopis[_Buffer]);

  Scan=0;
  while(1)
  {
    Input_TAB_ENABLED=1;

    // Adresa
    HexToChar(Config.CustomDistelBuffer[_Buffer][0]);
    InputHEXLine(26,7,2,16+15, HEX);
    HEX[0]=Vstup[0]; HEX[1]=Vstup[1];
    Config.CustomDistelBuffer[_Buffer][0]=CharToHex();
    if(Scan==1) break;
    InputHexArray(16,15,60,20,16+15,_Buffer,1);

    GetCRC(_Buffer); InputHexArray(16,15,60,20,16+15,_Buffer,1);
    textattr(16+32+64);
    gotoxy(45,11); cprintf(" %2Xh ",Config.CustomDistelBuffer[_Buffer][2+Config.CustomDistelBuffer[_Buffer][1]]);


    // LENGTH
    HexToChar(Config.CustomDistelBuffer[_Buffer][1]);
    InputHEXLine(45,7,2,16+15, HEX);
    HEX[0]=Vstup[0]; HEX[1]=Vstup[1];
    Config.CustomDistelBuffer[_Buffer][1]=CharToHex();
    if(Scan==1) break;
    GetCRC(_Buffer); InputHexArray(16,15,60,20,16+15,_Buffer,1);

    // COMMAND
    HEX[0]=0; HEX[1]=0;
    HexToChar(Config.CustomDistelBuffer[_Buffer][2]);
    InputHEXLine(26,10,2,16+15, HEX);
    HEX[0]=Vstup[0]; HEX[1]=Vstup[1];
    Config.CustomDistelBuffer[_Buffer][2]=CharToHex();
    if(Scan==1) break;

    GetCRC(_Buffer); InputHexArray(16,15,60,20,16+15,_Buffer,1);
    textattr(16+32+64);
    gotoxy(45,11); cprintf(" %2Xh ",Config.CustomDistelBuffer[_Buffer][2+Config.CustomDistelBuffer[_Buffer][1]]);

    // DistelBuffer
    InputHexArray(16,15,60,20,16+15, _Buffer, 0); if(Scan==1) break;

    GetCRC(_Buffer);
    InputHexArray(16,15,60,20,16+15,_Buffer,1);
    textattr(16+32+64);
    gotoxy(45,11); cprintf(" %2Xh ",Config.CustomDistelBuffer[_Buffer][2+Config.CustomDistelBuffer[_Buffer][1]]);

    // Popis zpravy
    InputTextLine(41,13,18,16+32+64+1, Config.DistelBufferPopis[_Buffer]);
    memcpy(Config.DistelBufferPopis[_Buffer], Vstup, 18);
    InputHexArray(16,15,60,20,16+15,_Buffer,1);

    if((Scan==1) | (Ascii==13)) break;
  }
  Scan=0; Ascii=0;
  SetScr(4,12,04,65,23);
  SetScr(5,0,24,79,25);
  LoadCursorPos(0);
}



//------------------------------------------------------------------------
// Prekresleni menu Distel / Nastaveni
//------------------------------------------------------------------------
void RefreshDistelSetMenu()
{
  #define DistelSetMenuItems 18
  ResetMenuItems();
  MenuItems[ 0].Y=4;  MenuItems[ 0].Item="Zpozdeni v [ms] pro odesilani mezi bajty      \0x0";
  MenuItems[ 1].Y=5;  MenuItems[ 1].Item="Zpozdeni v [ms] pro odesilani mezi zpravami   \0x0";
  MenuItems[ 2].Y=6;  MenuItems[ 2].Item="Pocet automatickeho odeslani zpravy    \0x0";
  MenuItems[ 3].Y=7;  MenuItems[ 3].Item="Maximalni pocet prichozich prikazu            \0x0";
  MenuItems[ 4].Y=8;  MenuItems[ 4].Item="Timeout pro detekci zpravy                  \0x0";
  MenuItems[ 5].Y=9;  MenuItems[ 5].Item="Pipnout pri prichodu zpravy                   \0x0";
  MenuItems[ 6].Y=10; MenuItems[ 6].Item="Pipnout pri odeslani zpravy                   \0x0";
  MenuItems[ 7].Y=11; MenuItems[ 7].Item="Typ pipnuti                  \0x0";
  MenuItems[ 8].Y=12; MenuItems[ 8].Item="Minimalni povolena hodnota adresy             \0x0";
  MenuItems[ 9].Y=13; MenuItems[ 9].Item="Maximalni povolena hodnota adresy             \0x0";
  MenuItems[10].Y=14; MenuItems[10].Item="Zpusob detekovani jednotlivych zprav\0x0";
  MenuItems[11].Y=15; MenuItems[11].Item="Casove odesilani \0x0";
  MenuItems[12].Y=16; MenuItems[12].Item="Pocet byte v bufferu pro detekci zprav        \0x0";
  MenuItems[13].Y=17; MenuItems[13].Item="Maximalni povolena delka zpravy               \0x0";
  MenuItems[14].Y=18; MenuItems[14].Item="Minimalni povolena delka zpravy               \0x0";
  MenuItems[15].Y=19; MenuItems[15].Item="Maximalni povolena hodnota prikazu            \0x0";
  MenuItems[16].Y=20; MenuItems[16].Item="Minimalni povolena hodnota prikazu            \0x0";
  MenuItems[17].Y=21; MenuItems[17].Item="Pipnout pri preteceni timeoutu pri prijmu     \0x0";
  MenuItems[18].Y=22; MenuItems[18].Item="Obnovit stat. komunik. pri prijmu znaku       \0x0";


  for(I=0;I<=DistelSetMenuItems;I++)
  {
    if(DSPol==I) textattr(10); else textattr(16+32+64);
    gotoxy(15,MenuItems[I].Y);
    cprintf(" %s   ",MenuItems[I].Item);

    if(DSPol==0) textattr(10); else textattr(16+32+64);
    gotoxy(63,MenuItems[0].Y); cprintf("[%5u] ",Config.DistelSendByteDelay);

    if(DSPol==1) textattr(10); else textattr(16+32+64);
    gotoxy(63,MenuItems[1].Y); cprintf("[%5u] ",Config.DistelSendMsgDelay);

    if(DSPol==2) textattr(10); else textattr(16+32+64);
    gotoxy(58,MenuItems[2].Y);
    if(Config.DistelMaxAutoSend)
      cprintf("[%10.0f] ",(float) Config.DistelMaxAutoSend);
    else
      cprintf("   Neomezene ",(float) Config.DistelMaxAutoSend);

    if(DSPol==3) textattr(10); else textattr(16+32+64);
    gotoxy(65,MenuItems[3].Y); cprintf("[%3d] ",Config.DistelMaxRecMsgs);

    if(DSPol==4) textattr(10); else textattr(16+32+64);
    gotoxy(63,MenuItems[4].Y); cprintf("[%5u] ",Config.DistelTimeoutMsgDetect);

    if(DSPol==5) textattr(10); else textattr(16+32+64);
    gotoxy(65,MenuItems[5].Y);
    if(Config.DistelBeepAfterRecMsg) cprintf("Ano   "); else cprintf("Ne    ");

    if(DSPol==6) textattr(10); else textattr(16+32+64);
    gotoxy(65,MenuItems[6].Y);
    if(Config.DistelBeepAfterSendMsg) cprintf("Ano   "); else cprintf("Ne    ");

    if(DSPol==7) textattr(10); else textattr(16+32+64);
    gotoxy(48,MenuItems[7].Y);
    switch(Config.DistelBeepType)
    {
      case 0: cprintf("%23s","System  (PC SP./SB) "); break;
      case 1: cprintf("%23s",   "Kratke  (PC SP.) "); break;
      case 2: cprintf("%23s",   "Stredni (PC SP.) "); break;
      case 3: cprintf("%23s",   "Dlouhe  (PC SP.) "); break;
    }

    if(DSPol==8) textattr(10); else textattr(16+32+64);
    gotoxy(65,MenuItems[8].Y); cprintf("[%3d] ",Config.DistelAdrMin);

    if(DSPol==9) textattr(10); else textattr(16+32+64);
    gotoxy(65,MenuItems[9].Y); cprintf("[%3d] ",Config.DistelAdrMax);

    if(DSPol==10) textattr(10); else textattr(16+32+64);
    gotoxy(54,MenuItems[10].Y);
    switch(Config.DistelMSGDetectionType)
    {
      case 0: cprintf("   Test CRC byte "); break;
      case 1: cprintf("         Timeout "); break;
      case 2: cprintf("   Timeout + CRC "); break;
      case 3: cprintf(" Timeout+CRC+CMP "); break;
    }

    if(DSPol==11) textattr(10); else textattr(16+32+64);
    gotoxy(36,MenuItems[11].Y);
    switch(Config.DistelMSGSendMode)
    {
      case 0: cprintf("   Odesilat vice zprav, dle poradi "); break;
      case 1: cprintf("Vzdy odeslat pouze aktualni zpravu "); break;
    }

    if(DSPol==12) textattr(10); else textattr(16+32+64);
    gotoxy(65,MenuItems[12].Y); cprintf("[%3d] ",Config.DistelBytesForDetect);

    if(DSPol==13) textattr(10); else textattr(16+32+64);
    gotoxy(65,MenuItems[13].Y); cprintf("[%3d] ",Config.DistelMaxMSGLength);

    if(DSPol==14) textattr(10); else textattr(16+32+64);
    gotoxy(65,MenuItems[14].Y); cprintf("[%3d] ",Config.DistelMinMSGLength);

    if(DSPol==15) textattr(10); else textattr(16+32+64);
    gotoxy(65,MenuItems[15].Y); cprintf("[%3d] ",Config.DistelCmdMax);

    if(DSPol==16) textattr(10); else textattr(16+32+64);
    gotoxy(65,MenuItems[16].Y); cprintf("[%3d] ",Config.DistelCmdMin);

    if(DSPol==17) textattr(10); else textattr(16+32+64);
    gotoxy(65,MenuItems[17].Y);
    if(Config.DistelBeepTimeout) cprintf("Ano   "); else cprintf("Ne    ");

    if(DSPol==18) textattr(10); else textattr(16+32+64);
    gotoxy(65,MenuItems[18].Y);
    if(Config.DistelAktualizeStatIfRec) cprintf("Ano   "); else cprintf("Ne    ");
  }
}




//------------------------------------------------------------------------
// Menu Distel / Nastaveni
//------------------------------------------------------------------------
void DistelSettings()
{
  HideTextCursor();
  SaveCursorPos(0);
  window(1,1,80,25);
  GetScr(2,13,2,70,9+DistelSetMenuItems);
  GetScr(3,0,24,79,24);
  InitDownMenu("~ESC/ENTER~ OK  ~Ctrl/Left/Right~ Change  ~Ctrl+D~ Default  ~Del/Ctrl+C~ Reset");
  Frame(14,3,70,5+DistelSetMenuItems,16+32+64,0);
  gotoxy(38,3); printf(" Nastaveni ");
  while(1)
  {
    RefreshDistelSetMenu();
    GetKey();
    switch(Scan)
    {
      case 72: if(DSPol)   DSPol--; break;                   // UP
      case 80: if(DSPol<DistelSetMenuItems) DSPol++; break;  // DN
      case 71: DSPol=0; break;                               // Home
      case 79: DSPol=DistelSetMenuItems; break;              // End
      case 73:                                               // PgUp
               switch(DSPol)
               {
                 case 0: Config.DistelSendByteDelay-=100;    break;
                 case 1: Config.DistelSendMsgDelay-=100;     break;
                 case 2: Config.DistelMaxAutoSend-=100;      break;
                 case 3: Config.DistelMaxRecMsgs-=10;        break;
                 case 4: Config.DistelTimeoutMsgDetect-=100; break;
                 case 7: if(Config.DistelBeepType>0)
                           Config.DistelBeepType--;          break;
                 case 8: Config.DistelAdrMin-=10;            break;
                 case 9: Config.DistelAdrMax-=10;            break;
                 case 10: if(Config.DistelMSGDetectionType)
                            Config.DistelMSGDetectionType--; break;
                 case 11: Config.DistelMSGSendMode=0;        break;
                 case 12: Config.DistelBytesForDetect-=10;   break;
                 case 13: Config.DistelMaxMSGLength-=10;     break;
                 case 14: Config.DistelMinMSGLength-=10;     break;
                 case 15: Config.DistelCmdMax-=10;           break;
                 case 16: Config.DistelCmdMin-=10;           break;
                 case 17: Config.DistelBeepTimeout=
                          !Config.DistelBeepTimeout;         break;
                 case 18: Config.DistelAktualizeStatIfRec=
                          !Config.DistelAktualizeStatIfRec;  break;
               }
               break;
      case 81:                                   // PgDn
               switch(DSPol)
               {
                 case 0: Config.DistelSendByteDelay+=100;    break;
                 case 1: Config.DistelSendMsgDelay+=100;     break;
                 case 2: Config.DistelMaxAutoSend+=100;      break;
                 case 3: Config.DistelMaxRecMsgs+=10 ;       break;
                 case 4: Config.DistelTimeoutMsgDetect+=100; break;
                 case 7: if(Config.DistelBeepType<3)
                           Config.DistelBeepType++;          break;
                 case 8: Config.DistelAdrMin+=10;            break;
                 case 9: Config.DistelAdrMax+=10;            break;
                 case 10: if(Config.DistelMSGDetectionType<3)
                            Config.DistelMSGDetectionType++; break;
                 case 11: Config.DistelMSGSendMode=1;        break;
                 case 12: Config.DistelBytesForDetect+=10;   break;
                 case 13: Config.DistelMaxMSGLength+=10;     break;
                 case 14: Config.DistelMinMSGLength+=10;     break;
                 case 15: Config.DistelCmdMax+=10;           break;
                 case 16: Config.DistelCmdMin+=10;           break;
                 case 17: Config.DistelBeepTimeout=
                          !Config.DistelBeepTimeout;         break;
                 case 18: Config.DistelAktualizeStatIfRec=
                          !Config.DistelAktualizeStatIfRec;  break;
               }
               break;
      case 75:                                   // Left
               switch(DSPol)
               {
                 case 0: Config.DistelSendByteDelay--;       break;
                 case 1: Config.DistelSendMsgDelay--;        break;
                 case 2: Config.DistelMaxAutoSend--;         break;
                 case 3: Config.DistelMaxRecMsgs--;          break;
                 case 4: Config.DistelTimeoutMsgDetect--;    break;
                 case 5: Config.DistelBeepAfterRecMsg=
                         !Config.DistelBeepAfterRecMsg;      break;
                 case 6: Config.DistelBeepAfterSendMsg=
                         !Config.DistelBeepAfterSendMsg;     break;
                 case 7: if(Config.DistelBeepType>0)
                         Config.DistelBeepType--;            break;
                 case 8: Config.DistelAdrMin--;              break;
                 case 9: Config.DistelAdrMax--;              break;
                 case 10: if(Config.DistelMSGDetectionType)
                            Config.DistelMSGDetectionType--; break;
                 case 11: Config.DistelMSGSendMode=0;        break;
                 case 12: Config.DistelBytesForDetect--;     break;
                 case 13: Config.DistelMaxMSGLength--;       break;
                 case 14: Config.DistelMinMSGLength--;       break;
                 case 15: Config.DistelCmdMax--;             break;
                 case 16: Config.DistelCmdMin--;             break;
                 case 17: Config.DistelBeepTimeout=
                          !Config.DistelBeepTimeout;         break;
                 case 18: Config.DistelAktualizeStatIfRec=
                          !Config.DistelAktualizeStatIfRec;  break;
               }
               break;
      case 77:                                   // Right
               switch(DSPol)
               {
                 case 0: Config.DistelSendByteDelay++;       break;
                 case 1: Config.DistelSendMsgDelay++;        break;
                 case 2: Config.DistelMaxAutoSend++;         break;
                 case 3: Config.DistelMaxRecMsgs++;          break;
                 case 4: Config.DistelTimeoutMsgDetect++;    break;
                 case 5: Config.DistelBeepAfterRecMsg=
                         !Config.DistelBeepAfterRecMsg;      break;
                 case 6: Config.DistelBeepAfterSendMsg=
                         !Config.DistelBeepAfterSendMsg;     break;
                 case 7: if(Config.DistelBeepType<3)
                           Config.DistelBeepType++;          break;
                 case 8: Config.DistelAdrMin++;              break;
                 case 9: Config.DistelAdrMax++;              break;
                 case 10: if(Config.DistelMSGDetectionType<3)
                            Config.DistelMSGDetectionType++; break;
                 case 11: Config.DistelMSGSendMode=1;        break;
                 case 12: Config.DistelBytesForDetect++;     break;
                 case 13: Config.DistelMaxMSGLength++;       break;
                 case 14: Config.DistelMinMSGLength++;       break;
                 case 15: Config.DistelCmdMax++;             break;
                 case 16: Config.DistelCmdMin++;             break;
                 case 17: Config.DistelBeepTimeout=
                          !Config.DistelBeepTimeout;         break;
                 case 18: Config.DistelAktualizeStatIfRec=
                          !Config.DistelAktualizeStatIfRec;  break;
               }
               break;
      case 115:                                   // Ctrl+Right
               switch(DSPol)
               {
                 case 0: Config.DistelSendByteDelay-=10;     break;
                 case 1: Config.DistelSendMsgDelay-=10;      break;
                 case 2: Config.DistelMaxAutoSend-=10;       break;
                 case 3: Config.DistelMaxRecMsgs-=10;        break;
                 case 4: Config.DistelTimeoutMsgDetect-=10;  break;
                 case 12: Config.DistelBytesForDetect-=10;   break;
                 case 13: Config.DistelMaxMSGLength-=10;     break;
                 case 14: Config.DistelMinMSGLength-=10;     break;
                 case 15: Config.DistelCmdMax-10;            break;
                 case 16: Config.DistelCmdMin-10;            break;
                 case 18: Config.DistelAktualizeStatIfRec=
                          !Config.DistelAktualizeStatIfRec;  break;
               }
               break;
      case 116:                                   // Ctrl+Left
               switch(DSPol)
               {
                 case 0: Config.DistelSendByteDelay+=10;    break;
                 case 1: Config.DistelSendMsgDelay+=10;     break;
                 case 2: Config.DistelMaxAutoSend+=10;      break;
                 case 3: Config.DistelMaxRecMsgs+=10;       break;
                 case 4: Config.DistelTimeoutMsgDetect+=10; break;
                 case 12: Config.DistelBytesForDetect+=10;  break;
                 case 13: Config.DistelMaxMSGLength+=10;    break;
                 case 14: Config.DistelMinMSGLength+=10;    break;
                 case 15: Config.DistelCmdMax+=10;          break;
                 case 16: Config.DistelCmdMin+=10;          break;
                 case 17: Config.DistelBeepTimeout=
                          !Config.DistelBeepTimeout;        break;
                 case 18: Config.DistelAktualizeStatIfRec=
                          !Config.DistelAktualizeStatIfRec;  break;
               }
               break;
      case 155:                                   // Alt+Right
               switch(DSPol)
               {
                 case 0: Config.DistelSendByteDelay/=2;    break;
                 case 1: Config.DistelSendMsgDelay/=2;     break;
                 case 2: Config.DistelMaxAutoSend/=2;      break;
                 case 3: Config.DistelMaxRecMsgs/=2;       break;
                 case 4: Config.DistelTimeoutMsgDetect/=2; break;
                 case 8: Config.DistelAdrMin/=2;           break;
                 case 9: Config.DistelAdrMax/=2;           break;
                 case 12: Config.DistelBytesForDetect/=2;  break;
                 case 13: Config.DistelMaxMSGLength/=2;    break;
                 case 14: Config.DistelMinMSGLength/=2;    break;
                 case 15: Config.DistelCmdMax/=2;          break;
                 case 16: Config.DistelCmdMin/=2;          break;
                 case 17: Config.DistelBeepTimeout=
                          !Config.DistelBeepTimeout;       break;
                 case 18: Config.DistelAktualizeStatIfRec=
                          !Config.DistelAktualizeStatIfRec;  break;
               }
               break;
      case 157:                                   // Alt+Left
               switch(DSPol)
               {
                 case 0: Config.DistelSendByteDelay*=2;    break;
                 case 1: Config.DistelSendMsgDelay*=2;     break;
                 case 2: Config.DistelMaxAutoSend*=2;      break;
                 case 3: Config.DistelMaxRecMsgs*=2;       break;
                 case 4: Config.DistelTimeoutMsgDetect*=2; break;
                 case 8: Config.DistelAdrMin*=2;           break;
                 case 9: Config.DistelAdrMax*=2;           break;
                 case 12: Config.DistelBytesForDetect*=2;  break;
                 case 13: Config.DistelMaxMSGLength*=2;    break;
                 case 14: Config.DistelMinMSGLength*=2;    break;
                 case 15: Config.DistelCmdMax*=2;          break;
                 case 16: Config.DistelCmdMin*=2;          break;
                 case 17: Config.DistelBeepTimeout=
                          !Config.DistelBeepTimeout;       break;
                 case 18: Config.DistelAktualizeStatIfRec=
                          !Config.DistelAktualizeStatIfRec;  break;
               }
               break;
      case 83:                                        // Delete
               switch(DSPol)
               {
                 case 0: Config.DistelSendByteDelay=0;     break;
                 case 1: Config.DistelSendMsgDelay=0;      break;
                 case 2: Config.DistelMaxAutoSend=0;       break;
                 case 3: Config.DistelMaxRecMsgs=0;        break;
                 case 4: Config.DistelTimeoutMsgDetect=0;  break;
                 case 8: Config.DistelAdrMin=0;            break;
                 case 9: Config.DistelAdrMax=0;            break;
                 case 10: Config.DistelMSGDetectionType=0; break;
                 case 11: Config.DistelMSGSendMode=0;      break;
                 case 12: Config.DistelBytesForDetect=0;   break;
                 case 13: Config.DistelMaxMSGLength=0;     break;
                 case 14: Config.DistelMinMSGLength=0;     break;
                 case 15: Config.DistelCmdMax=0;           break;
                 case 16: Config.DistelCmdMin=0;           break;
                 case 17: Config.DistelBeepTimeout=0;      break;
                 case 18: Config.DistelAktualizeStatIfRec=0; break;
               }
               break;
      case 46: if(Ascii==3)                                  // Ctrl+C
               {
                 Config.DistelSendByteDelay=0;
                 Config.DistelSendMsgDelay=0;
                 Config.DistelMaxAutoSend=0;
                 Config.DistelMaxRecMsgs=0;
                 Config.DistelTimeoutMsgDetect=0;
                 Config.DistelAdrMin=0;
                 Config.DistelAdrMax=0;
                 Config.DistelMSGDetectionType=0;
                 Config.DistelMSGSendMode=0;
                 Config.DistelBytesForDetect=0;
                 Config.DistelMaxMSGLength=0;
                 Config.DistelMinMSGLength=0;
                 Config.DistelCmdMax=0;
                 Config.DistelCmdMin=0;
                 Config.DistelBeepTimeout=0;
                 Config.DistelAktualizeStatIfRec=0;
               }
               break;
      case 32: if(Ascii==4)                              // Ctrl+D
               {
                 SetDistelDefaultConfig();
               }
               break;
      case 57:
               switch(DSPol)
               {
                 case 5: Config.DistelBeepAfterRecMsg=
                           !Config.DistelBeepAfterRecMsg;
                         break;
                 case 6: Config.DistelBeepAfterSendMsg=
                           !Config.DistelBeepAfterSendMsg;
                         break;
               }
    }
    if((Scan==1) | (Ascii==13)) break;
  }
  Scan=0; Ascii=0;
  SetScr(2,13,2,70,9+DistelSetMenuItems);
  SetScr(3,0,24,79,24);
  LoadCursorPos(0);
}




unsigned char DistelBuffersList(unsigned char _Buffer)
{
  unsigned char SelectedBuf;
  unsigned char _Num,NumSort;

  HideTextCursor();
  SaveCursorPos(2);
  window(1,1,80,25);
  GetScr(2,14,4,70,18);
  GetScr(3,0,24,79,24);
  InitDownMenu("~ESC~ Zpet  ~ENTER~ Vybrat  ~1..9..A..C~ Por. cislo  ~Ctrl+C / Del~ Zrusit por. c.");
  Frame(20,5,66,19,16+32+64,0);
  gotoxy(37,5); printf(" Seznam zprav ");
  gotoxy(22,6); printf("Klav");
  gotoxy(32,6); printf("Zprava");
  gotoxy(49,6); printf("ADR");
  gotoxy(54,6); printf("LNG");
  gotoxy(59,6); printf("CMD");
  gotoxy(64,6); printf("NUM");
  DBLPol=_Buffer;

  while(1)
  {

    for(SelectedBuf=0; SelectedBuf<12; SelectedBuf++)
    {
      if(DBLPol==SelectedBuf) textattr(10); else textattr(16+32);

      gotoxy(21,7+SelectedBuf);
      if(SelectedBuf<9) cprintf(" F%d: ",SelectedBuf+1);
      else cprintf(" F%d:",SelectedBuf+1);
      if(strlen(Config.DistelBufferPopis[SelectedBuf]))
      {
        cprintf("%20s ",Config.DistelBufferPopis[SelectedBuf]);
      }
      else cprintf("------ NONAME ------ ");
      cprintf("   %2X",Config.CustomDistelBuffer[SelectedBuf][0]);
      cprintf("   %2X",Config.CustomDistelBuffer[SelectedBuf][1]);
      cprintf("   %2X",Config.CustomDistelBuffer[SelectedBuf][2]);

      if(DBLPol==SelectedBuf) textattr(10); else textattr(1+16+32);
      if(Config.DistelMsgNumber[SelectedBuf] > 0)
        cprintf("   %2d",Config.DistelMsgNumber[SelectedBuf]);
      else
        cprintf("     ");
    }

    GetKey();
    if((Ascii==0) | (Ascii==224))
    switch(Scan)
    {
      case 72: if(DBLPol) DBLPol--;     break;
      case 80: if(DBLPol<11) DBLPol++;  break;
      case 71: DBLPol=0;                break;
      case 79: DBLPol=11;               break;
      case 62: EditDistelBuffer(DBLPol);
               HideTextCursor();
               break;
    }


    if((Scan==46) & (Ascii==3))                            // Ctrl+C
    {
      for(I=0; I<12; I++) Config.DistelMsgNumber[I]=0;
    }

    if((Scan==83) & ((Ascii==224) | (Ascii==0)))           // Del
    {
      Config.DistelMsgNumber[DBLPol]=0;
    }

    if(Scan!=0)
    if(((Ascii>=49) & (Ascii<=57)) |
       ((UpChr(Ascii)>=65) & (UpChr(Ascii)<=67)))
    {
      if(Ascii<=57) _Num=Ascii-48;
      else _Num=UpChr(Ascii)-65+10;
      Config.DistelMsgNumber[DBLPol]=_Num;

      for(I=0; I<12; I++)
      {
        if(Config.DistelMsgNumber[I]==_Num)
        {
          if(I!=DBLPol) Config.DistelMsgNumber[I]=0;
        }
      }
    }

    if((Scan==1) | (Ascii==13)) break;
  }
  SetScr(2,14,4,70,18);
  SetScr(3,0,24,79,24);
  LoadCursorPos(2);
  //ShowTextCursor();
  Scan=0;
  if(Ascii==13) return(1); else return(0);
}



//---------------------------------------------------------------------------
//  Nastaveni jednickove parity
//---------------------------------------------------------------------------
void Set1Parity()
{
  unsigned char PortSet;

  PortSet=0;
  switch(Config.PocBit)
  {
    case 5: break;              // 5 bitu
    case 6: PortSet|=1; break;  // 6 bitu
    case 7: PortSet|=2; break;  // 7 bitu
    case 8: PortSet|=3; break;  // 8 bitu
    default: break;
  }
  if(Config.StpBit==1) PortSet+=4;
  PortSet|=8+16+32;  //Parita Vzdy 1
  outp(Config.AdresaPortu+3,PortSet);
}




//---------------------------------------------------------------------------
//  Nastaveni nulove parity
//---------------------------------------------------------------------------
void Set0Parity()
{
  unsigned char PortSet;

  PortSet=0;
  switch(Config.PocBit)
  {
    case 5: break;              // 5 bitu
    case 6: PortSet|=1; break;  // 6 bitu
    case 7: PortSet|=2; break;  // 7 bitu
    case 8: PortSet|=3; break;  // 8 bitu
    default: break;
  }
  if(Config.StpBit==1) PortSet+=4;
  PortSet|=8+32;     //Parita: Vzdy 0
  outp(Config.AdresaPortu+3,PortSet);
}



void RefreshSRMsgStatus(unsigned long MsgSended,unsigned long MsgReceived)
{
  window(1,1,80,25);
  textattr(10);
  gotoxy(15,22); cprintf("Odeslano:");
  gotoxy(48,22); cprintf("Prichozich:");
  textattr(11);
  gotoxy(31,22); cprintf("%5.0f ",(float) MsgSended);
  gotoxy(65,22); cprintf("%5.0f ",(float) MsgReceived);
}



void DistelBeep()
{
  switch(Config.DistelBeepType)
  {
    case 0: putch(7); break;
    case 1: sound(1000);  delay(1);   nosound(); break;
    case 2: sound(1000);  delay(50);  nosound(); break;
    case 3: sound(1000);  delay(150); nosound(); break;
  }
}



void ShowMsgLabel(unsigned char SelectedBuf)
{
  ShowHint("Aktualni zprava: ");
  textattr(16+14);
  cprintf("F%d / ",SelectedBuf+1);
  if(strlen(Config.DistelBufferPopis[SelectedBuf]))
  {
    textattr(16+11);
    cprintf("%s",Config.DistelBufferPopis[SelectedBuf]);
  }
  else
  {
    textattr(16+7);
    cprintf("NONAME");
  }
}




//------------------------------------------------------------------------
void ProtokolDistel()
{
  unsigned char NoMsg, Break, I, BufIndex, _Buffer, SelectedBuf;
  unsigned char Hex_Pos;
  unsigned char DistelBuf[256];
  unsigned char Adr_D[20];
  unsigned char Cmd_D[20];
  unsigned char Lng_D[20];
  unsigned long PocMsg_D[20];
  unsigned char PosY;
  long Timeout;
  unsigned long MsgSended, MsgReceived, MaxAutoSend, ZahozenoByte;
  unsigned long AllMsgReceived;
  unsigned char SendByteIndex, SendMsgIndex;
  unsigned char MsgExists;
  unsigned char CS, BufPos, LN;
  unsigned char DetectTimeout, ViewTimeout;
  unsigned long SetTimeout;
  unsigned int  minTimeout, maxTimeout, prumTimeout;
  unsigned char VBufSize, CHREC, _StatView;
  unsigned long OverrunChars, ParityErrors, StopBitErrors;
  unsigned char _StopReceiving;

  InitPort();
  window(1,1,80,25);
  GetScr(0,11,1,79,23);
  ShowHint("\0x0");
  InitDownMenu("~ESC~ Zpet  ~SHIFT+F1~ Klav.zkratky  ~F1..F12~ Odeslat zpravu");
  Frame(12,2,79,23,15,0);
  gotoxy(38,2); cprintf(" Protokol Distel ");
  gotoxy(13,4);  for(I=0; I<67; I++) putch('Ä');
  gotoxy(13,6);  for(I=0; I<67; I++) putch('Ä');
  gotoxy(13,21); for(I=0; I<67; I++) putch('Ä');
  ShowPortSet();
  gotoxy(15,5); cprintf("Pocet/ AD LN CM Data...");

  Cursor_X_0=23; Cursor_Y_0=2; LoadCursorPos(0);
  Cursor_X_1=23; Cursor_Y_1=2; LoadCursorPos(1);

  for(I=0; I < (sizeof(DistelBuf)-1); I++) DistelBuf[I]=0;
  for(I=0; I < sizeof(PocMsg_D); I++)  PocMsg_D[I]=0;

  ShowTextCursor();
  Input_TAB_ENABLED=1;

  PosY=0;
  _DistelAutoSend=0; // 1=probiha autom. odesilani znaku
  MsgSended=0;       // Pocet odeslani zpravy
  MsgReceived=0;     // Pocet prijeti zpravy
  _PauseReceive=0;   // 1 = stop
  ZahozenoByte=0;    // Zahozenych byte pri detekci zprav
  _Buffer=0;         // Cislo bufferu
  SelectedBuf=0;
  Timeout=0;
  SetTimeout=0;
  DetectTimeout=1;
  ViewTimeout=0;
  minTimeout=0xFFFF;
  maxTimeout=0;
  prumTimeout=0;
  BufIndex=0;
  BufPos=0;
  OverrunChars=0;
  ParityErrors=0;
  StopBitErrors=0;
  _StatView=0;
  _StopReceiving=0;

  ShowMsgLabel(SelectedBuf);
  HideTextCursor();

  while(1)
  {
    if(kbhit())
    {
      GetKey();

      if((_KeybHelpStatus) & (Scan==1)) { KeybHelp(0); Scan=0; }

      if(_DistelAutoSend==0)
      {
        // Editace bufferu (Stisk Ctrl+F1..F12)
        if((((Scan>=94) & (Scan<=103)) | (Scan==137) | (Scan==138)) & (Ascii==0))
        {
          if(Scan<=103) _Buffer=Scan-94; else _Buffer=(Scan-137+10);
          EditDistelBuffer(_Buffer);
          Ascii=0;
        }

        // Odeslani zpravy  (Stisk F1..F12)
        // F1..F10 = 59..68 ; F11 = 133 ; F12 = 134
        if((((Scan>=59) & (Scan<=68)) | (Scan==133)| (Scan==134)) & (Ascii==0))
        {
          if(Scan<=68) _Buffer=Scan-59; else _Buffer=(Scan-133+10);
          textbackground(0); window(13,3,79,3); clrscr();
          if(strlen(Config.DistelBufferPopis[_Buffer]))
          {
            textattr(16+32+64+1);
            cprintf("%s",Config.DistelBufferPopis[_Buffer]);
            textattr(7); cprintf(": ");
          }

          textattr(15);
          GetCRC(_Buffer);
          for(I=0; I<(Config.CustomDistelBuffer[_Buffer][1]+3); I++)
          {
            if(Delay(Config.DistelSendByteDelay))
            { Scan=Ascii=0; break; }

            if(I==0) Set1Parity(); else Set0Parity();

            VysliByte(Config.CustomDistelBuffer[_Buffer][I]);
            switch(I)
            {
              case 0: textcolor(14); break;
              case 1: textcolor(11); break;
              case 2: textcolor(10); break;
              default: textcolor(15); break;
            }
            if(I==(Config.CustomDistelBuffer[_Buffer][1]+2)) textattr(12);

            //if(I>((Config.CustomDistelBuffer[_Buffer][1]+3) -10))
                     // ... Zobrazit poslednich 10 byte

            // ... Zobrazit prvnich 10 byte
            if(I<(17-(strlen(Config.DistelBufferPopis[_Buffer])/3)))
            {
              cprintf("%.2X ",Config.CustomDistelBuffer[_Buffer][I]);
            }
          }

          if(Config.DistelBeepAfterSendMsg) DistelBeep();

          MsgSended++;
          HideTextCursor();
          RefreshSRMsgStatus(MsgSended,MsgReceived);
        }


        // Vyber bufferu pro automaticke odesilani (Alt+F1..F12)
        // F1..F10 .. 104-113;  F11..F12 .. 139-140
        if((((Scan>=104) & (Scan<=113)) | (Scan==139)| (Scan==140)) & (Ascii==0))
        {
          if(Scan<=113) SelectedBuf=Scan-104; else SelectedBuf=(Scan-139+10);
          ShowMsgLabel(SelectedBuf);
        }


        // Zobrazeni aktualniho Timeout
        if((Scan==20) & (Ascii==0))                         // Alt+T
        {
          ViewTimeout=1;
          SaveCursorPos(4);
          window(1,1,80,25);
          ShowHint("Aktualni timeout: ");
          cprintf("%3d ",Timeout);
          LoadCursorPos(4);
        }


        // Menu Nastaveni
        if((Scan==49) & (Ascii==14))                       // CTRL+N
        {
          DistelSettings();
        }

        if((Scan==25) & (UpChr(Ascii)==80))                 // P
        {
          if(_PauseReceive)
          {
            ShowHint("Obnoveno vypisovani zprav...");
            _PauseReceive=0;
          }
          else
          {
            ShowHint("Pozastaveno vypisovani zprav...");
            _PauseReceive=1;
          }
        }

        if((Scan==25) & (Ascii==0))                      // Alt+P
        {
          _StopReceiving=!_StopReceiving;
        }

        // Zobrazeni menu Seznamu zprav (bufferu)
        if((Scan==38) & (Ascii==12))                      // CTRL+L
        {
          if(DistelBuffersList(SelectedBuf))
          {
            SelectedBuf=DBLPol;
            ShowMsgLabel(SelectedBuf);
          }
        }


        // ----------- DEBUG MODE -----------------------
        if((Scan==143) & (Ascii==0))   // CTRL+NUM5
        {
          textattr(10);
          window(1,1,80,25); clrscr();
          textattr(16+32+64);
          window(1,1,80,1); clrscr();
          cprintf(" Debug mode....... test parity");

          textattr(14+16);
          window(1,25,80,25); clrscr();
          cprintf("ESC ... Exit");

          textattr(15);
          window(1,2,80,24);
          gotoxy(3,3); cprintf("Data ready");
          gotoxy(3,4); cprintf("Overrun error");
          gotoxy(3,5); cprintf("Parity error");
          gotoxy(3,6); cprintf("Framing error");
          gotoxy(3,7); cprintf("Break indicated");
          gotoxy(3,8); cprintf("Send buffer free");
          gotoxy(3,9); cprintf("Sender free");
          gotoxy(3,10); cprintf("Fifo data error");

          while(1)
          {
            /*
            outp(0x2F8, 0b000
            printf("Odeslan byte 1 , parita 0\n\r");
            if(Delay(1000)) break;

            printf("Odeslan byte 2 , parita 1\n\r");
            if(Delay(1000)) break;
            */

            outp(0x2FB,0x3B);
            if(inp(0x2FD) & 1)
            {
              I=inp(0x2F8);
              gotoxy(1,1);
              if(I!=7) cprintf("Znak: %1c   ",I);
              PortStatus=inp(0x2FD);
              gotoxy(10,1); cprintf("Status 2FD: %2X   2FA: %2X  ",PortStatus,inp(0x2FA));

              gotoxy(22,3);cprintf("%2d",PortStatus & 1);
              gotoxy(22,4);cprintf("%2d",PortStatus & 2);
              gotoxy(22,5);cprintf("%2d",PortStatus & 4);
              gotoxy(22,6);cprintf("%2d",PortStatus & 8);
              gotoxy(22,7);cprintf("%2d",PortStatus & 16);
              gotoxy(22,8);cprintf("%2d",PortStatus & 32);
              gotoxy(22,9);cprintf("%2d",PortStatus & 64);
              gotoxy(22,10);cprintf("%2d",PortStatus & 128);

              if(PortStatus & 4)
              {
                gotoxy(2,12); printf("Chybna parita byte: %.2X ",I);
              }
            }

            if(Delay(100)) { Exit=1; break; }
          }
        }
      }

      // Zobrazi pocet prepsanych znaku, chybu parity...
      if((Scan==23) & (Ascii==9))     // CTRL+I
      {
        SaveCursorPos(4);
        window(1,1,80,25);
        ShowHint("Ztraceno znaku: "); cprintf("%5.0f ",(float) OverrunChars);
        cprintf("     Chyb parity: %5.0f ",(float) ParityErrors);
        cprintf("     Spatnych stopbitu: %5.0f ",(float) StopBitErrors);
        LoadCursorPos(4);
      }


      // Zobrazeni / Skryti statistiky komunikace
      if(((Scan==31) & (Ascii==0)) & (!_KeybHelpStatus))  // Alt+S
      {
         if(!_StatView)
         {
           HideTextCursor();
           SaveCursorPos(3);
           window(1,1,80,25);
           GetScr(3,14,4,70,18);
           Frame(18,5,68,19,16+10,0);
           gotoxy(39,5); printf(" Statistika ");
           gotoxy(20,6); printf("Minimalni pocet cyklu mezi byte:");
           gotoxy(20,7); printf("Maximalni pocet cyklu mezi byte:");
           gotoxy(20,8); printf("Prumerny pocet cyklu mezi byte:");
           gotoxy(20,9);  printf("Odeslano zprav:          Prijato zprav:");
           gotoxy(20,10); printf("Zahozeno znaku:          Prepsano znaku:");
           gotoxy(20,11); printf("Chyb parity:             Chyb stopbitu:");
           gotoxy(20,12); printf("Celkovy pocet prijatych zprav:");

           _PauseReceive=1;
           _StatView=1;
           Scan=0;
         }
      }



      // Vynulovani promennych a vymazani okna prijatych zprav
      if((Scan==32) & (Ascii==4))    // Ctrl+D
      {
        BufPos=BufIndex=0;
        MsgReceived=0;     // Vynulovani poctu prijatych zprav
        MsgSended=0;       // Vynulovani poctu prijatych zprav
        OverrunChars=0;    // Vynulovani poctu prepsanych znaku
        ParityErrors=0;    // Reset poctu chyb parity
        StopBitErrors=0;   // Reset poctu spatnych stopbitu
        minTimeout=0xFFFF; // Reset minimalni hodnoty timeoutu
        maxTimeout=0;      // Reset maximalni hodnoty timeoutu
        prumTimeout=0;     // Reset prumerne hodnoty timeoutu

        memset(Adr_D, 0, sizeof(Adr_D)-1);
        memset(Cmd_D, 0, sizeof(Cmd_D)-1);
        memset(Lng_D, 0, sizeof(Lng_D)-1);
        memset(PocMsg_D, 0, sizeof(PocMsg_D)-1);

        RefreshSRMsgStatus(MsgSended,MsgReceived);
        ShowMsgLabel(SelectedBuf);
        if(!_PauseReceive)
        {
          textbackground(0);
          window(13,3,79,3); clrscr();
          window(13,7,79,20); clrscr(); window(1,1,80,25);
        }
      }



      if((Scan==24) & (Ascii==15))     // Ctrl+O
      {
        MsgSended=0;         // Vynulovani poctu odeslanuch zprav
        RefreshSRMsgStatus(MsgSended,MsgReceived);
      }


      // Vynulovani poctu prijatych zprav
      if((Scan==19) & (Ascii==18))     // Ctrl+R
      {
        MsgReceived=0;       // Vynulovani poctu prijatych zprav
        RefreshSRMsgStatus(MsgSended,MsgReceived);
      }


      // Zobrazeni / Skryti napovedy s funkcnimi tlacitky
      if(((Scan==84) & (Ascii==0)) & (!_StatView))        // Shift+F1
      {
         if(_KeybHelpStatus) KeybHelp(0); else KeybHelp(1);
      }



      // Start / Konec automatickeho odesilani zprav
      if(((Scan==20) & (Ascii==20)) | (_DistelAutoSend==2))   // CTRL+T
      {
        if(_DistelAutoSend) _DistelAutoSend=2;
        else
        {
          NoMsg=0;
          if(Config.DistelMSGSendMode==0)  // Nastaveno odeslani vice zprav
          {
            NoMsg=1;
            for(I=0; I<12; I++) if(Config.DistelMsgNumber[I]) NoMsg=0;
          }

          if(NoMsg)
          {
            GetScr(2,17,11,67,14);
            Frame(18,12,64,14,16+10,0);
            gotoxy(20,13); cprintf("Vyberte zpravy pro odesilani (CTRL+L) ...");
            delay(1000);
            SetScr(2,17,11,67,14);
          }
          else
          {
            textbackground(0);
            window(13,3,79,3);   clrscr();
            window(13,22,79,22); clrscr();
            window(1,1,80,25);
            _DistelAutoSend=1;
            SendMsgIndex=0;
            MaxAutoSend=Config.DistelMaxAutoSend+MsgSended;
            SendByteIndex=0;
            gotoxy(1,24);

            if(Config.DistelMSGSendMode==1)
            {
              ShowHint("Probiha automaticke odesilani zpravy ");
              cprintf("'%s' ",Config.DistelBufferPopis[SelectedBuf]);
              cprintf(" (F%d) ... ",SelectedBuf+1);
              gotoxy(13,3); textattr(16+32+64+1);
              if(strlen(Config.DistelBufferPopis[SelectedBuf]))
              {
                cprintf("%s",Config.DistelBufferPopis[SelectedBuf]);
                textattr(7); cprintf(": ");
              }
            }

            if(Config.DistelMSGSendMode==0)
            {
              ShowHint("Probiha automaticke odesilani zprav... ");
            }
          }
        }
      }
    }


    // Zobrazeni promennych statistiky komunikace
    if((Scan==1) & (_StatView > 0))
    {
      _StatView=0;
      _PauseReceive=0;
      SetScr(3,14,4,70,18);
      LoadCursorPos(3);
      Scan=0;
    }

    if(_StatView)
    {
      if(_StatView==1)
      {
        gotoxy(61,6);  printf("%3d ",minTimeout);
        gotoxy(61,7);  printf("%3d ",maxTimeout);
        gotoxy(61,8);  printf("%3d ",prumTimeout);
        gotoxy(36, 9); printf("%4.0f ",(float) MsgSended);
        gotoxy(60, 9); printf("%4.0f ",(float) MsgReceived);
        gotoxy(36,10); printf("%4.0f ",(float) ZahozenoByte);
        gotoxy(60,10); printf("%4.0f ",(float) OverrunChars);
        gotoxy(36,11); printf("%4.0f ",(float) ParityErrors);
        gotoxy(60,11); printf("%4.0f ",(float) StopBitErrors);

        AllMsgReceived=0;
        for(I=0; I < MsgReceived; I++)
        {
          AllMsgReceived+=PocMsg_D[I];
        }
        gotoxy(60,12); printf("%4.0f ",(float) AllMsgReceived);
      }

      if(Config.DistelAktualizeStatIfRec) _StatView=1;
      else _StatView=2;
    }



    // Konec automatickeho odesilani zprav
    if(_DistelAutoSend==2)
    {
      _DistelAutoSend=0;
      textattr(16+10);
      window(1,24,79,24); clrscr(); window(1,1,80,25);
      gotoxy(1,24); printf("Automaticke odesilani ukonceno.");
      gotoxy(13,3);
      Scan=Ascii=0;
    }

    if(!DetectTimeout) SetTimeout++;


//========== Prijem znaku ==============
    if((CharReady()) & (!_StopReceiving))
    {
       DistelBuf[BufIndex]=inp(Config.AdresaPortu) & Config.BitMask;
       CHREC=1;
       GetPortStatus();
       if(OverrunError()) OverrunChars++;
       if(ParityError())  ParityErrors++;
       if(StopbitError()) StopBitErrors++;

       BufIndex++;
       // if(BufIndex >= sizeof(DistelBuf)) BufIndex=0;

       Timeout=Config.DistelTimeoutMsgDetect;

       if(Config.DistelAktualizeStatIfRec)
         if(_StatView==2) _StatView=1;

       switch(DetectTimeout)
       {
         case 0:  DetectTimeout=1; break;
         case 1:  DetectTimeout=0; SetTimeout=0; break;
         default: DetectTimeout=1;
       }

      if(DetectTimeout)
      {
        // Vypocet minimalni, maximalni a prumerne hodnoty timeoutu
        if(SetTimeout < minTimeout) minTimeout=SetTimeout;
        if(SetTimeout > maxTimeout) maxTimeout=SetTimeout;
        prumTimeout=(minTimeout + maxTimeout) / 2;

        if(ViewTimeout==1)
        {
          ViewTimeout=0;

          SaveCursorPos(4);
          window(1,1,80,25);
          gotoxy(40,24); printf("Pocet cyklu: %5d  ",SetTimeout);
          LoadCursorPos(4);
        }
      }
    }
    else
    {
      if(_DistelAutoSend)
      {
        if(SendByteIndex==0) Set1Parity(); else Set0Parity();

        // -----------  Odesilani pouze jedne, aktualni zpravy --------
        if(Config.DistelMSGSendMode==1)
        {
          if(Delay(Config.DistelSendByteDelay))
          { _DistelAutoSend=2; Scan=Ascii=0; }

          VysliByte(Config.CustomDistelBuffer[SelectedBuf][SendByteIndex]);
          window(1,1,80,25);
          if(SendByteIndex<(17-(strlen(Config.DistelBufferPopis[SelectedBuf])/3)))
          {
            textattr(15);
            gotoxy(SendByteIndex*3+15+strlen(Config.DistelBufferPopis[SelectedBuf]),3);
            switch(SendByteIndex)
            {
              case 0: textcolor(14); break;
              case 1: textcolor(11); break;
              case 2: textcolor(10); break;
              default: textcolor(15); break;
            }
            if(SendByteIndex==(Config.CustomDistelBuffer[SelectedBuf][1]+2)) textattr(12);
            cprintf("%.2X ",Config.CustomDistelBuffer[SelectedBuf][SendByteIndex]);
          }

          SendByteIndex++;

          if(SendByteIndex > (Config.CustomDistelBuffer[SelectedBuf][1]+2))
          {
            if(Config.DistelBeepAfterSendMsg) DistelBeep();
            MsgSended++;
            RefreshSRMsgStatus(MsgSended,MsgReceived);
            SendByteIndex=0;
            if(Config.DistelMaxAutoSend)
            {
              if(MaxAutoSend <= MsgSended) _DistelAutoSend=2;
            }

            if(Delay(Config.DistelSendMsgDelay))
            { _DistelAutoSend=2; Scan=Ascii=0; }
          }
        }


        // ------ Postupne odesilani vice zprav, podle poradi ---------
        if(Config.DistelMSGSendMode==0)
        {
          if(SendMsgIndex > 11)
          {
            SendMsgIndex=0;
            MsgSended++;
            if(Config.DistelMaxAutoSend)
            {
              if(MaxAutoSend <= MsgSended)
              {
                _DistelAutoSend=2;
                if(Config.DistelBeepAfterSendMsg) DistelBeep();
              }
            }
          }

          if(Config.DistelMsgNumber[SendMsgIndex]==SendMsgIndex+1)
          {
            VysliByte(Config.CustomDistelBuffer[SendMsgIndex][SendByteIndex]);
            if(SendByteIndex < (17-(strlen(Config.DistelBufferPopis[SendMsgIndex])/3)))
            {
              window(1,1,80,25);
              if(SendByteIndex==0)
              {
                textbackground(0); window(13,3,79,3); clrscr(); window(1,1,80,25);
                gotoxy(13,3);
                if(Config.DistelBufferPopis[SendMsgIndex][0])
                {
                  textattr(16+32+64+1);
                  cprintf("%s",Config.DistelBufferPopis[SendMsgIndex]);
                  textattr(7);
                  cprintf(": ");
                }
              }
              gotoxy(SendByteIndex*3+13+strlen(Config.DistelBufferPopis[SendMsgIndex])+2,3);
              switch(SendByteIndex)
              {
                case 0: textcolor(14); break;
                case 1: textcolor(11); break;
                case 2: textcolor(10); break;
                default: textcolor(15); break;
              }
              if(SendByteIndex==(Config.CustomDistelBuffer[SendMsgIndex][1]+2)) textattr(12);
              cprintf("%.2X ",Config.CustomDistelBuffer[SendMsgIndex][SendByteIndex]);
            }

            SendByteIndex++;
            if(Delay(Config.DistelSendByteDelay))
            { _DistelAutoSend=2; Scan=Ascii=0; }


            if(SendByteIndex > (Config.CustomDistelBuffer[SendMsgIndex][1]+2))
            {
              if(Config.DistelBeepAfterSendMsg) DistelBeep();
              SendMsgIndex++;
              SendByteIndex=0;
              RefreshSRMsgStatus(MsgSended,MsgReceived);

              if(Delay(Config.DistelSendMsgDelay))
              { _DistelAutoSend=2; Scan=Ascii=0; }
            }
          }
          else
          {
            SendMsgIndex++;
          }
        }
      }
    }



    // Detekce prichozich zprav
//==================================================================

    if(Timeout>=0) Timeout--;

    if((Config.DistelMSGDetectionType==3) & ((CHREC) | (!Timeout)))
    {
      CHREC=0;

      // Zjisteni poctu byte v bufferu (aktualni velikosti bufferu)
      if(BufIndex >  BufPos)  VBufSize=(BufIndex-BufPos);
      if(BufIndex <  BufPos)  VBufSize=((sizeof(DistelBuf)) - BufPos + BufIndex);
      if(BufIndex == BufPos)  VBufSize=0;

      // Je-li v bufferu prednastaveny pocet byte nebo dosel Timeout...
      if((VBufSize >= Config.DistelBytesForDetect) | (Timeout<=1))
      {
        if((Timeout==0) & (Config.DistelBeepTimeout)) DistelBeep();

        // Celkova delka pomyslne zpravy (bez CRC)
        LN=DistelBuf[BufPos+1]+2;

        // Je-li adresa a delka zpravy v povolenem rozsahu...
        if((DistelBuf[BufPos+0] >= Config.DistelAdrMin) &
           (DistelBuf[BufPos+0] <= Config.DistelAdrMax) &
           (DistelBuf[BufPos+1] >= Config.DistelMinMSGLength) &
           (DistelBuf[BufPos+1] <= Config.DistelMaxMSGLength) &
           (DistelBuf[BufPos+2] >= Config.DistelCmdMin) &
           (DistelBuf[BufPos+2] <= Config.DistelCmdMax) &
           (LN < VBufSize) & (LN >= 2))
        {
           // Vypocet CRC pro aktualni ( pomyslnou ) zpravu v bufferu
          CS=0;
          for(I=0; I < LN; I++)  CS=CS + DistelBuf[BufPos+I];

          //if((CS) & LN
          if(CS==DistelBuf[LN+BufPos])
          {
            MsgExists=0;

            for(PosY=0; PosY < MsgReceived; PosY++)
            {
              if((DistelBuf[BufPos+0] == Adr_D[PosY] ) &
                 (DistelBuf[BufPos+1] == Lng_D[PosY] ) &
                 (DistelBuf[BufPos+2] == Cmd_D[PosY] ))
              {
                MsgExists=1;
                PocMsg_D[PosY]++;
                RefreshSRMsgStatus(MsgSended, MsgReceived);
                if(!_PauseReceive)
                {
                  textattr(7);
                  if(!Timeout) textattr(10);
                  if(VBufSize >= Config.DistelBytesForDetect) textattr(8);
                  gotoxy(15, 7+PosY);
                  cprintf("%5.0f: ",(float) PocMsg_D[PosY]);

                  for(I=0; I < 19; I++)
                  {
                    switch(I)
                    {
                      case 0: textcolor(14); break;
                      case 1: textcolor(11); break;
                      case 2: textcolor(10); break;
                      default: textcolor(15); break;
                    }
                    if(I==LN) textcolor(12);
                    if(I <= LN)
                      cprintf("%.2X ",DistelBuf[(unsigned char) (I+BufPos)]);
                    else cprintf("   ");
                  }
                }
              }
            }

            if(!MsgExists)
            {
              if(MsgReceived<=Config.DistelMaxRecMsgs)
              {
                PocMsg_D[PosY]++;
                MsgExists=1;
                textattr(10);
                RefreshSRMsgStatus(MsgSended,MsgReceived);
                if(!_PauseReceive)
                {
                  textattr(12);
                  //if(!Timeout) textattr(10);
                  //if(VBufSize >= Config.DistelBytesForDetect) textattr(8);
                  gotoxy(15, 7+PosY);
                  cprintf("%5.0f: ",(float) PocMsg_D[PosY]);
                  for(I=0; I < 19; I++)
                  {
                    switch(I)
                    {
                      case 0: textcolor(14); break;
                      case 1: textcolor(11); break;
                      case 2: textcolor(10); break;
                      default: textcolor(15); break;
                    }
                    if(I==LN) textcolor(12);
                    if(I <= LN)
                      cprintf("%.2X ",DistelBuf[(unsigned char) (I+BufPos)]);
                    else cprintf("   ");
                  }
                }
                Adr_D[MsgReceived]=DistelBuf[BufPos+0];
                Lng_D[MsgReceived]=DistelBuf[BufPos+1];
                Cmd_D[MsgReceived]=DistelBuf[BufPos+2];
                if(Config.DistelBeepAfterRecMsg) DistelBeep();

                MsgReceived++;
              }
            }
            BufPos=(BufPos+LN);
          }
          //else if(VBufSize) BufPos++;
        }
      }
      if(VBufSize>=Config.DistelBytesForDetect) BufPos++;
    }


//------------------------------------------------------------------
    if((Config.DistelMSGDetectionType==0) | // Detekce: Test CRC
       (Config.DistelMSGDetectionType==2))  // Detekce CRC + Timeout
    {
      //if(Timeout>=0) Timeout--;

      if((BufIndex >= Config.DistelBytesForDetect) |
         ((Timeout <= 0) & (Config.DistelMSGDetectionType==2)))
      {
        while(1)
        {
          PosY=0;

          LN=DistelBuf[BufPos+1]+2; // Celkova delka pomyslne zpravy
                                    // (bez CRC)
          MsgExists=0;

          // Vypocet CRC pro aktualni ( pomyslnou ) zpravu v bufferu
          CS=0;
          for(I=0; I < LN; I++)
          { CS=(unsigned char) (CS + DistelBuf[BufPos+I]); }

          if((LN>2) & (DistelBuf[BufPos+I] == CS) &
             (Config.DistelMaxMSGLength >= LN)    &
             (Config.DistelMinMSGLength <= LN))
          {
            if((DistelBuf[BufPos]>=Config.DistelAdrMin) &
               (DistelBuf[BufPos]<=Config.DistelAdrMax))
            {
              for(PosY=0; PosY < MsgReceived; PosY++)
              {
                if((DistelBuf[BufPos+0] == Adr_D[PosY] ) &
                   (DistelBuf[BufPos+2] == Cmd_D[PosY] ))
                {
                  MsgExists=1;
                  PocMsg_D[PosY]++;
                  RefreshSRMsgStatus(MsgSended,MsgReceived);
                  if(!_PauseReceive)
                  {
                    if(!Timeout) textattr(10); else textattr(7);
                    gotoxy(15, 7+PosY);
                    cprintf("%5.0f/ ",(float) PocMsg_D[PosY]);

                    for(I=0; I < 19; I++)
                    {
                      switch(I)
                      {
                        case 0: textcolor(14); break;
                        case 1: textcolor(11); break;
                        case 2: textcolor(10); break;
                        default: textcolor(15); break;
                      }
                      if(I==LN) textcolor(12);
                      if(I <= LN) cprintf("%.2X ",DistelBuf[I+BufPos]);
                      else cprintf("   ");
                    }
                  }
                }
              }

              if(!MsgExists)
              {
                if(MsgReceived<=Config.DistelMaxRecMsgs)
                {
                  PocMsg_D[PosY]++;
                  textattr(10);
                  RefreshSRMsgStatus(MsgSended,MsgReceived);
                  if(!_PauseReceive)
                  {
                    if(!Timeout) textattr(10); else textattr(7);
                    gotoxy(15, 7+PosY);
                    cprintf("%5.0f/ ",(float) PocMsg_D[PosY]);
                    for(I=0; I < 19; I++)
                    {
                      switch(I)
                      {
                        case 0: textcolor(14); break;
                        case 1: textcolor(11); break;
                        case 2: textcolor(10); break;
                        default: textcolor(15); break;
                      }
                      if(I==LN) textcolor(12);
                      if(I <= LN) cprintf("%.2X ",DistelBuf[I+BufPos]);
                      else cprintf("   ");
                    }
                  }
                  Adr_D[MsgReceived]=DistelBuf[BufPos+0];
                  Cmd_D[MsgReceived]=DistelBuf[BufPos+2];
                  if(Config.DistelBeepAfterRecMsg) DistelBeep();

                  MsgReceived++;
                }
              }

            }
            BufPos++;
          }
          else
          {
            ZahozenoByte++; BufPos++;
            //gotoxy(1,1); cprintf("%4d ",ZahozenoByte);
          }


          if(BufPos>=BufIndex)
          {
            BufIndex=0; BufPos=0;
            break;
          }
        }
      }
    }



//-----------------------------------------------------------------
    // Detekce prichozich zprav pomoci Timeoutu
    if(Config.DistelMSGDetectionType==1)
    {
      //if(Timeout>=0) Timeout--;

      if(Timeout<=0)
      {
        PosY=0;
        MsgExists=0;

        if(Timeout==0)
        while(1)
        {
          for(PosY=0; PosY < MsgReceived; PosY++)
          {
            if((DistelBuf[0]==Adr_D[PosY]) & (DistelBuf[2]==Cmd_D[PosY]))
            {
              PocMsg_D[PosY]++;
              RefreshSRMsgStatus(MsgSended,MsgReceived);
              if(!_PauseReceive)
              {
                textattr(10);
                gotoxy(13, 7+PosY); cprintf("%5.0f/ ",(float) PocMsg_D[PosY]);
                textattr(15);
                I=0;
                while(1)
                {
                  if(I<BufIndex) cprintf("%.2X ",DistelBuf[I]);
                  else cprintf("   ");
                  if(I>=15) break;
                  I++;
                }
              }
              MsgExists=1;
              BufIndex=0;
              break;
            }
          }

          if(!MsgExists)
          {
            if(MsgReceived<=Config.DistelMaxRecMsgs)
            {
              PocMsg_D[PosY]++;
              textattr(10);
              RefreshSRMsgStatus(MsgSended,MsgReceived);
              if(!_PauseReceive)
              {
                textattr(10);
                gotoxy(13, 7+PosY); cprintf("%5.0f/ ",(float) PocMsg_D[PosY]);
                textattr(15);
                I=0;
                while(1)
                {
                  if(I<BufIndex) cprintf("%.2X ",DistelBuf[I]);
                  else cprintf("   ");
                  if(I>=15) break;
                  I++;
                }
              }
              Adr_D[MsgReceived]=DistelBuf[0];
              Cmd_D[MsgReceived]=DistelBuf[2];
              if(Config.DistelBeepAfterRecMsg) DistelBeep();

              MsgReceived++;
              BufIndex=0;
              break;
            }
            else break;
          }
          else break;
        }
      }
    }


    // Alt+X ... exit
    if((Scan==45)&(Ascii==0)) Exit=1;

    if(!_DistelAutoSend) { if((Scan==1) | (Exit)) break; }
    else { if(Scan==1) _DistelAutoSend=2; }
  }
  HideTextCursor();
  if(_KeybHelpStatus) KeybHelp(0);
  window(1,1,80,25);
  SetScr(0,11,1,79,23);
  RefreshMainMenu();
}


//------------------------------------------------------------------------
//               Sledovani promennych po komunikaci
//------------------------------------------------------------------------
void Follow()
{
  unsigned char RecBuf[10];
  unsigned char Buff_Index;
  unsigned int  Promenna[20];
  unsigned char Loop;

  InitPort();
  GetScr(0,11,1,79,23);
  Frame(12,2,79,23,15+16,0);
  InitDownMenu("~F1..F12~ Cti promn  ~Ctrl+F1..F12~ Edit promn");
  textattr(15+16);
  gotoxy(40,2); cprintf(" Sledovani ");
  ShowPortSet();
  window(13,3,79,22);
  ShowTextCursor();

  Input_TAB_ENABLED=0;
  Buff_Index=0;
  Loop=0;

  while(1)
  {
    if(kbhit())
    {
      GetKey();
      if(Scan==1) break;

      if((((Scan>=94) & (Scan<=103)) | (Scan==137) | (Scan==138)) & (Ascii==0))
      {
        Ascii=0;
      }

      // Zapnuti/Vypnuti automatickeho zobrazovani promennych
      if((Scan==38) & (Ascii==12))
      {
        if(Loop) Loop=0; else Loop=1;
        gotoxy(30,22); cprintf("Loop: ");
        if(Loop) cprintf("ON"); else cprintf("OFF");
      }

      if((Scan>=59) & (Scan<=68) & (Ascii==0))
      {
        textattr(15+16);
        VysliByte('C');
        VysliByte('M');
        VysliByte('D');
        VysliByte(Scan-59); // Vysli cislo promenne
        if(VysliByte(0)==0xFF)
	{ DisplayTimeOutError(); delay(2000);}  // Vysli prikaz
        Ascii=0;
      }
    }

    if(CharReady())
    {
       if(Buff_Index<10) { RecBuf[Buff_Index]=inp(Config.AdresaPortu); }
       else Buff_Index=0;
       if(Buff_Index<10) Buff_Index++; else Buff_Index=0;

       if(RecBuf[Buff_Index-3]=='R')
       {
         if((RecBuf[Buff_Index-4]=='A')&
           (RecBuf[Buff_Index-5]=='V'))
           {

           }
       }
    }

    if((Scan==45)&(Ascii==0)) Exit=1;
    if((Scan==1) | (Exit)) break;
  }
  HideTextCursor();
  window(1,1,80,25);
  SetScr(0,11,1,79,23);
  InitDownMenu("~Alt-X~ Exit");
}



//------------------------------------------------------------------------
/*
void Napoveda()
{
  unsigned char Line[81];
  unsigned long L;

  GetScr(0,0,0,79,25);
  ShowHint("\0x0");

  if ((handle = _open("TERMINAL.TXT", O_RDONLY)) == -1)
  {
    printf("Nelze otevrit soubor \"TERMHELP.TXT\" !\n");
    return;
  }
  InitDownMenu("~ESC~ Zpet  ~DOWN~ Dalsi  ~UP~ Predchozi");

  L=0;
  textattr(16+10);
  gotoxy(1,1);
  clrscr();
  lseek(handle, 0L, SEEK_SET);
  do
  {
    if((Scan==80) || (Scan==72))
    {
      lseek(handle, L*80, SEEK_SET);
      _read(handle, &Line, 80); Line[80]=0;
      cprintf(Line);
    }

    GetKey();
    if(Scan==72)
    {
      gotoxy(1,25); delline(); gotoxy(1,1);
      if(L>160) L=L-160;
    }

    if(Scan==80)
    {
      gotoxy(1,1); delline(); gotoxy(1,25);
      L+=80;
      //insline();
    }


  } while(Scan!=1);
  close(handle);
  SetScr(0,0,0,79,25);
}

*/




//------------------------------------------------------------------------
//  Test kontrolni sumy EXE souboru
//------------------------------------------------------------------------
void EXEtest()
{
  const BTr=200;

  unsigned EXEAttr;
  unsigned char header[4];
  unsigned long L;
  unsigned char buf[BTr];
  unsigned int BytesRead;

  if ((handle = _open(Param, O_RDONLY )) == -1)
  {
     printf("Nelze otevrit %s pro kontrolni soucet !",Param);
     printf("Pokracovat ? (Enter/Esc) .... ");
     GetKey();
     if(Scan!=28) ExitAbout(0xFF);
  }
  else
  {
    ActCRCtest=0;
    lseek(handle, (filelength(handle)-7), SEEK_SET);
    _read(handle, header, 3);
    header[3]=0;

    if((header[0]=='C') & (header[1]=='C') & (header[2]=='S'))
    {
      printf("Probiha test kontrolni sumy EXE souboru ....");
      lseek(handle, 0, SEEK_SET);
      for(L=0; L < (filelength(handle) / BTr); L++)
      {
        BytesRead=_read(handle, buf, BTr);
        if(eof(handle)) for(I=BytesRead-7; I<sizeof(buf); I++) buf[I]=0;
        for(I=0; I<BTr; I++) ActCRCtest=ActCRCtest+buf[I];
      }

      lseek(handle, filelength(handle)-4, SEEK_SET);
      _read(handle, &CRCtest, 4);

      printf("\n\n Kontrolni soucet ulozeny v EXE: %X%X \n",CRCtest);
      printf(" Vypocteny kontrolni soucet EXE: %X%X \n\n",ActCRCtest);

      if(CRCtest != ActCRCtest)
      {
         printf("Upozorneni: detekovana zmena EXE souboru !\n\n");
         printf("\nPokracovat?  (Enter/Esc) .... ");
         GetKey();
         printf("\n\n\n");
         if(Scan!=28) ExitAbout(0xFF);
      }
      else printf("Soucet je Ok\n\n");

      _close(handle);

    }
    else
    {
      _close(handle);

      printf("Probiha zapis kontrolni sumy do EXE souboru .... ");
      if ((handle = _open(Param, O_RDWR )) != -1)
      {
        _dos_setfileattr(Param,0);
        _dos_getfileattr(Param,&EXEAttr);
        if(EXEAttr & 1==1)
        {
          printf("\n  Nelze zapsat do EXE souboru !");
          Delay(1000);
          return;
        }

        lseek(handle, 0, SEEK_SET);
        for(L=0; L < filelength(handle)/BTr; L++)
        {
          BytesRead=_read(handle, buf, BTr);
          if(BytesRead<BTr) for(I=BytesRead; I<sizeof(buf); I++) buf[I]=0;
          for(I=0; I<BTr; I++) ActCRCtest=ActCRCtest+buf[I];
        }

        lseek(handle, 0, SEEK_END);
        _write(handle, "CCS", 3);
        if(_write(handle, &ActCRCtest, 4)!=-1)
        printf("Ok\n");
        _close(handle);
      }
    }
  }
}


void ParamHelp()
{
  printf("\n\nTerminal\n\n");
  printf("Syntaxe: TERMINAL.EXE /F\n");
  printf("\n   /F  ... Nedetekovat adresu portu pres BIOS, pouzit fixni adresy\n\n");
  exit(0);
}




//=====================================================================


void main(unsigned char PocPar, char *ParamStr[])
{
  Param=ParamStr[0];
  _CancelModeEnabled=0;

  EXEtest();

  Config.SetFixAdress=0;
  for(I=0; I<PocPar; I++)
  {
    if(ParamStr[I][0]=='/')
    switch(ParamStr[I][1])
    {
      case 'f':
      case 'F': Config.SetFixAdress=1; break;
      case '?': ParamHelp(); break;
    }
  }

  setcbrk(0);
  Config.Port=1;
  Config.ExitDialog=1;

  ReadConfigFile();
  Config.AdresaPortu=AdrPort(Config.Port);

  AutoOpenStatus=0;
  if(Config.AutoOpen)
  {
    AutoOpenStatus=1;
  }
  else Config.Pol=0;

  InitGSVideo();
  InitScreen();
  HideTextCursor();
  InitPort();

  InitDownMenu("~Alt-X~ Exit");
  Frame(1,2,10,23,16+7,0);
  RefreshMainMenu();

  UPol=SPol=SNPol=NPol=DCPol=STPPol=DistelPol=DSPol=DBLPol=BMPol=MPol=0;
  HEX[2]=0;

  while(1)
  {
     InitDownMenu("~Alt-X~ Exit                                          ");
     switch(Config.Pol)
     {
       case  0: ShowHint("Znaky => COM. Prijate znaky jsou zobrazovany\0x0"); break;
       case  1: ShowHint("Znaky => Vstup. okno + => COM. Prijate znaky => Vyst. okno"); break;
       case  2: ShowHint("HexDec. cisla => COM po zadani celeho cisla v hex. tvaru. Prij. zn. zobr."); break;
       case  3: ShowHint("HexDec. => Vstup. okno + => COM po zadani celeho cisla. Prijate zobraz."); break;
       case  4: ShowHint("HexDec. => Vstup. okno + => COM po zadani celeho cisla. Prijate zobraz."); break;
       case  5: ShowHint("Znaky => Vstup. okno + => COM po zadani celeho cisla. Prijate zobraz."); break;
       case  6: ShowHint("Znaky => Vstup. okno + => COM po zadani celeho cisla. Prijate zobraz."); break;
       case  7: ShowHint("Konfigurace"); break;
       case  8: ShowHint("Ruzne pomucky"); break;
       case  9: ShowHint("Komunikacni protokol..."); break;
       case 10: ShowHint("Follow - sledovani promennych pres komunikaci"); break;
       case 11: ShowHint("Ukonceni programu");
                if(Config.ExitDialog) printf("...");
                break;
     }

     if(AutoOpenStatus)
     {
       if(Config.ActivatePol) { Scan=28; Ascii=13; }
       AutoOpenStatus=0;
     }
     else GetKey();

     if((Scan==45) & (Ascii==0)) Exit=1;
     if((Scan==72) & (Config.Pol > 0)) Config.Pol--;
     if((Scan==80) & (Config.Pol < MainMenuItems-4)) Config.Pol++;
     if(Scan==71) Config.Pol=0;
     if(Scan==79) Config.Pol=MainMenuItems-5;

     RefreshMainMenu();
     if((Scan==28) | (Scan==77))
     {
       switch( Config.Pol )
       {
          case  0: Terminal();         break;
          case  1: TerminalIOASC();    break;
          case  2: TerminalHEX();      break;
          case  3: TerminalIOHEX();    break;
          case  4: TerminalIOASCHEX(); break;
          case  5: TerminalIOHEXASC(); break;
          case  6: Terminal2COM();     break;
          case  7: Nastaveni();        break;
          case  8: Utility();          break;
          case  9: ProtokolDistel();   break;
          case 10: Follow();           break;
          case 11: Exit=1;             break;
          default: break;
       }

       if(Scan==45)
       {
         Config.ActivatePol=0;
         switch(Config.Pol)
         {
           case  0: Config.ActivatePol=1;
           case  1: Config.ActivatePol=1;
           case  2: Config.ActivatePol=1;
           case  3: Config.ActivatePol=1;
           case  4: Config.ActivatePol=1;
           case  5: Config.ActivatePol=1;
           case  6: Config.ActivatePol=1;
           case  9: Config.ActivatePol=1;
           case 10: Config.ActivatePol=1;
         }
       }
     }

     if(Exit)
     {
       if(Config.ExitDialog) ExitDialog();
       if(Exit) break;
     }
  }
  DoneGSVideo();
  textattr(7);
  ShowHint("Zapisuji konfiguraci...");
  WriteConfigFile();
  ShowTextCursor();
  ExitAbout(0);
}
