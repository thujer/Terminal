unsigned char EBuf[10];

unsigned char X,Y;

unsigned char EggText[]={
"                                                                            \0x00"
"                                                                            \0x00"
"                                                                            \0x00"
"                                 ���������                                  \0x00"
"                                (()()())())                                 \0x00"
"                                � �Ŀ �Ŀ �                                 \0x00"
"                                � �0� �0� �                                 \0x00"
"                                � ��� ��� �                                 \0x00"
"                                �    �    �                                 \0x00"
"                                �   ���   �                                 \0x00"
"                                � �����Ŀ �                                 \0x00"
"                                � ������� �                                 \0x00"
"                                (���������)                                 \0x00"
"                                     �                                      \0x00"
"                                   ���Ŀ                                    \0x00"
"                                   � � �                                    \0x00"
"                                  �� � ��                                   \0x00"
"                                     �                                      \0x00"
"                                     �                                      \0x00"
"                                    ���                                     \0x00"
"                                   �� ��                                    \0x00"
"                                  ��   ��                                   \0x00"
"                                                                            \0x00"
"                                                                            \0x00"
"                                                                            \0x00"
};



// Direction=1 .. Nahoru
void RolScreen(unsigned char PocRadku, unsigned char Direction)
{
   if(Direction) asm MOV AH,7;
   else          asm MOV AH,6;

   asm{

        MOV CL,0
        MOV CH,0
        MOV DL,79
        MOV DH,24
        MOV AL,PocRadku
        MOV BH,0
        INT 0x10
                     }
}


void GoAnim()
{
  HideTextCursor();
  delay(200);
  gotoxy(35,12);cprintf("�����Ŀ\0x00");
  gotoxy(35,12);cprintf("�----��\0x00");
  gotoxy(41,18);cprintf("�-\0x00");
  delay(200);
  gotoxy(35,12);cprintf("�����Ŀ\0x00");
  gotoxy(35,12);cprintf("�-___-�\0x00");
  gotoxy(41,18);cprintf("���   \0x00");
  delay(200);
  gotoxy(35,12);cprintf("�����Ŀ\0x00");
  gotoxy(35,12);cprintf("�_____�\0x00");
  gotoxy(41,18);cprintf("����  \0x00");
  delay(200);
  gotoxy(35,12);cprintf("�����Ŀ\0x00");
  gotoxy(35,12);cprintf("�-___-�\0x00");
  gotoxy(41,18);cprintf("����  \0x00");
  delay(200);
  gotoxy(35,12);cprintf("�����Ŀ\0x00");
  gotoxy(35,12);cprintf("�----��\0x00");
  gotoxy(41,18);cprintf("�-    \0x00");
  delay(200);
  gotoxy(35,12);cprintf("�����Ŀ\0x00");
  gotoxy(35,12);cprintf("�������\0x00");
  ShowTextCursor();
}

void Egg()
{
  window(1,1,80,25);
  SaveCursorPos(4);
  GetScr(4,0,0,79,24);

  textattr(15);

  for(X=0; X<25; X++)
  {
    RolScreen(1,1);
    gotoxy(1,1);
    cprintf("%s",&EggText[(24-X)*80]);
    delay(10);
  }

  GoAnim();

  //GetKey();
  delay(500);

  for(X=0; X<25; X++)
  {
    RolScreen(1,1);
    for(Y=0; Y<80; Y++)
    {
      pokeb(0xB800, Y*2 + 0*80*2, ScrBuf[4][Y*2 + (24-X)*80*2]);
      pokeb(0xB800, Y*2 + 0*80*2 + 1, ScrBuf[4][Y*2 + (24-X)*80*2 + 1]);
    }
    delay(10);
  }
  LoadCursorPos(4);
}




void TestEgg(unsigned char Ch)
{
    EBuf[6]=Ch;
    for(X=0; X<6; X++)
    {
       EBuf[X]=EBuf[X+1];
       if((EBuf[0]=='K')&(EBuf[1]=='o')&(EBuf[2]=='u')&(EBuf[3]=='t')&
       (EBuf[4]=='a')&(EBuf[5]=='s'))
         Egg();
    }
}