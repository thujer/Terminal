Borland C++ - COM & LPT terminal application, DistEl protocol

NÁVOD K PROGRAMU TERMINAL
---

Tento program byl napsán pro zjednodušení využití sériových portů (COM).
Nastavení parametrů portu se provádí přímo, tzn. přímým zápisem
na adresy portů uložených na adresách Proměnných ROM-BIOSu:

Adresa  Bytu Obsah

▀▀▀▀▀▀▀ ▀▀▀▀ ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
```
0:0400    2  Adresa portu prvního RS-232 adaptéru (COM1)
0:0402    2  Port COM2
0:0404    2  Port COM3
0:0406    2  Port COM4
0:0408    2  Adresa portu první paralelní tiskárny (LPT1)
0:040A    2  Port LPT2
0:040C    2  Port LPT3
0:040E    2  Port LPT4
```


Výhodou tohoto způsobu ovládaní portu je, že lze nastavit vetšinu

parametrů portu, které nejsou jinak DOSovým aplikacím přístupné.


Nežádoucím důsledkem tohoto nastavování je občas konfliktní

komunikace pod Windows32 (viz kap. 2).

Je to způsobeno tím, že Terminal si nastaví parametry portu které

Windows nenastavují a to způsobí, že Windows pak nejsou schopny

si port správně nastavit.



1
NÁVOD K POUŽITÍ
---

1.2
Nastavení
---

Nastavení portu je přístupné v hlavním menu pod položkou "Nastaveni".
Můžete zde nastavit různé parametry komunikace:
"Port" zde si můžete zvolit komunikační port. Port COM5 není skutečný,
ale můžete jej stiskem klávesy F4 nastavit na libovolnou adresu.
Po výběru portu je port inicializován, takže pokud se objeví hlášení:
              "Port XXX na adrese AAA nelze inicializovat"
znamená to že adresa portu je nastavena na 0h,
a port s nejvetší pravděpodobností není připojen. Může ale nastat případ,
kdy je v počítači na ISA sběrnici přítomen interní modem. Tyto modemy
standardně emulují sériový port, podmínkou však je aby byla adresa tohoto
emulovaného portu zapsána v proměnných BIOSu. To lze zjistit v tabulce
postu: v tabulce zařízení, kde je výpis COM portů se objeví další adresa portu
( standardne COM3 nebo COM4 ). Pokud se takto nestane, bude port v počítači
přítomen, ale pro většinu DOSových aplikací "neviditelný", protože jeho adresa
v proměnných BIOSu bude nastavena na 0h. Windows by měly modem najít ale adresu
portu modemu stejně nenastaví. Ke "zpřístupnění" portu modemu můžete použít u
Terminálu parametr /F, nebo zmenit v nastavení položku "Adresy portu" na
hodnotu "Pevné". To způsobí, že Terminál nebude zjišťovat adresy portů z
proměnných BIOSu, ale dosadí si vlastní - pevné adresy (tzn. COM1=3F8,
COM2=2F8, COM3=3E8, COM4=2E8).

Dalším nastavením je "Rychlost portu", k tomuto snad není co dodávat,
můžete nastavit všechny možné rychlosti portu, které samotný port
podporuje: od 150 do 153600 Bd.

"Nastaveni portu" zde můžete nastavit parametry portu jako je počet
stop-bitů (1..2), parita (žádná, sudá, lichá, vždy 0, vždy 1),
délka slova (5..8 bitů).

"Potlacit znaky" toto nastavení zamezí v Ascii výpisu zobrazování znaků
jako je např. 07 (bell) - nedojde k nepříjemnému pípání PC speakeru,
08 (Backspace) - nesmažou se vám předchozí přijatý znak,
0Dh zabrání návratu kurzoru na začátek řádku, 0Ah zabrání posunu kurzoru
na další řádek. Místo potlačeného znaku se zobrazí jen "neškodná" mezera.

"Bitova maska" - při přijmutí znaku je nejdříve provedena logická funkce AND
s tímto bytem, a pak je teprve znak dále zpracováván.

"Delay pro Ascii/HEX" nastavení minimální doby mezi odesláním jednotlivých
znaků v Ascii/HEX módu při odesílání znaků z bufferu.

"Adresy portu" - Určuje, zda jsou adresy portu načteny z proměnných BIOSu,
nebo jsou dosazeny konstantní hodnoty

"Ukoncovaci dialog" - Jestliže je nastaveno zobrazovat, je před ukončením
programu zobrazen potvrzovací dialog, zda chcete program opravdu ukončit.


1.3
Utility
---

V tomto menu jsou na výběr některé pomůcky pro komunikaci.
( Vysilat nepretrzite, Zobrazuj nepretrzite, Zobrazuj prijate znaky )


"Vysilat nepretrzite"
-------------------
Posílá neustále na port vybraný znak (00h, 07h, FFh, RND, P40h) aktuální
rychlostí portu, bez čekání, tzn. bez ohledu na hodnotu parametru Delay.
Pozastavit vysílání lze klávesou SPACE.
RND je náhodný znak,
P40h posílá znaky načtené z portu 40h - je to téměř totožné jako RND ale
jen o něco rychlejší ( hlavně u pomalejších PC ).


"Zobrazuj nepretrzite"
--------------------
Zobrazuje ve výstupním okně aktuální stav na portu, nečeká na příznak
přijetí znaku.


"Zobrazuj prijate znaky"
----------------------
Zobrazuje ve výstupním okně přijaté znaky v HEX formě
(čeká na příznak přijetí znaku).


"ECHO mode"
----------
Znak přijatý na COM1 nebo COM2, je poslán zpět. Zobrazuje se počet přijatých
znaků.

1.4
Komunikace
---

Pro odesílání a přijímání dat je možne využít několika módů Terminálu:

- ASCII: Znaky jsou posílány z klávesnice přímo na port - bez zobrazení
         a zobrazují se pouze přijaté znaky.
         V tomto módu lze využít ASCII buffer (viz. kap. 1.8 ) pro
         odesílání znaku.

- I/O ASCII: Znaky jsou posílány z klávesnice na port a zobrazovány
             v horním okně. Přijaté znaky jsou zobrazovány ve spodním
             okně.
             Opět lze využít ASCII buffer.

- HEX: Byty jsou odesílány po zadání druhé části hexadecimálního čísla,
       číslo je zobrazováno v horním řádku.
       Přijaté byty jsou zobrazovány ve spodním okně v hexadecimální formě.
       V tomto módu lze využít HEX buffer ( viz. kap. 1.8 ) pro odesílání
       znaku.

- I/O HEX: Je totožné s HEX, s tím rozdílem, že horní okno je větší
           kvůli přehlednosti.

- ASC/HEX: Znaky jsou odesílány na port a zobrazovány v ASCII formě
           v horním okně, přijaté byty jsou zobrazovány ve spodním okně
           v hex. formě. V tomto módu lze využít ASCII buffer pro odesílání
           znaků.

- 2xCOM: Znaky jsou přijímány ze dvou volitelných portů současně a
         zobrazovány v oddělených oknech (horním a dolním).
         V nastavení (klávesa Ctrl+N) je možné povolit odesílání znaků
         z klávesnice na port vybraný pro horní okno terminálu.
         Dále je zde možné nastavit v jaké formě budou znaky zobrazovány
         a jaký bude použit buffer pro editaci nebo odesílaní (Ascii/Hex).
         Na odesílání znaků z klávesnice toto nastavení nemá vliv.



1.5
Buffery pro odesílání znaků
---

V každém módu Terminálu lze použít buffer pro odesílání znaků. Velikost
tohoto bufferu je max. 55 bytů. Buffery pro ASCII a HEX módy jsou oddělené,
v módu ASC/HEX se využívá ASCII buffer. Editovat je lze pomocí kombinace
klávesy CTRL a kláves F1 až F12 ( takže pro ASCII módy je v dispozici
celkem 10 bufferu a každý může mít 55 bytů). Do bufferů lze také vložit
některé funkční znaky přes klávesu ALT (např. ALT 13 je ENTER;
ALT 27 je ESC). Buffer se do vysílání zařadí stiskem klávesy F1 až F12.
Vysílání lze pozastavit stiskem SPACE a přerušit stiskem ESC.
Po vyslání obsahu bufferu můžete pokračovat odesíláním znaků/bytů
z klávesnice.


2
Problémy s komunikací
---

 Občas se může stát, že na konkrétním portu operuje nějaká Windowsí rutina,
 což se projeví periodickým přijímáním nějakého znaku (vetšinou FFh),
 bez připojeného zařízení na portu...

 K odstranění tohoto problému někdy stačí jen znovu inicializovat port
 - inicializace probíhá vždy hned po spuštění programu, po výběru
 portu, nebo při otevření nějakého okna terminálu.
 Pokud toto nepomůže budete nejspíš muset resetovat počítač tvrdým resetem,
 nebo nejlépe na několik sekund počítač vypnout.
 Jestliže ani při tomto zákroku nedošlo k odstranění problému, je možné
 že používáte nějaký (DOSovský nebo Windowsí) program pro nastavení nebo
 přesměrování portu, nebo nějaký rezidentní program zapisující na port.
 Pokud je tomu tak, a chcete používat tento Terminal, nezbyde Vám nic
 jiného než rezident dočasně zablokovat, nebo pokud jej nepotřebujete,
 odstranit. Pokud spouštíte Terminal pod Windows, zkuste ho spustit
 ve výhradním ( "čistém" ) DOSu, zde by měl pracovat spolehlivěji.
 Jestli na konkrétním portu stále něco vysíla, je take docela možné,
 že v Proměnných ROM-BIOSu jsou chybně nastaveny adresy portů.
 V tomto případě zkuste změnit v nastavení terminálu "Adresy portu"
 na hodnotu "Pevne" nebo spustit Terminal s parametrem /F , to způsobí, že
 adresy portu se nebudou číst z Proměnných ROM-BIOSu ale dosadí se
 standardně používané adresy portů.

Při spouštění pod Windows se může stát, ze Terminal si nastaví port podle
svých parametrů, a po jeho ukončení si s tímto nastavením nevědí některé
Windowsí aplikace rady, v tomto případě je nutné, pokud již nechcete
používat Terminal, restartovat počítač, aby BIOS obnovil nastavení portu.




3
Popis přímého ovládáni sériových portů:
---
```
Port  Popis
▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
3f8H  Zápis: Vysílací registr. 8 bitů znaku k odvysílání
      Čtení: Přijímací registr. 8 bitů přijatého znaku
      Zápis: (když DLAB=1) dolní byte dělitele       Baud Dělitel █ Baud Dělitel
             Po OUT 3fbH,80H tento port drží dolní   ▀▀▀▀ ▀▀▀▀▀▀▀ █ ▀▀▀▀ ▀▀▀▀▀▀▀
             byte dělitele hodin, který společně      110    1040 █ 1200      96
             s horním bytem (port 3f9H) tvoří         150    768  █ 2400      48
             16-bitovou hodnotu, která určuje         300    384  █ 4800      24
             přenosovou rychlost podle tabulky        600    192  █ 9600      12
                                                     ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
3f9H  Zápis: Horní byte dělitele (když DLAB=1, po OUT 3fBH,80H)
      Zápis: Registr povolení přerušení
      ╔7┬6┬5┬4┬3┬2┬1┬0╗
      ║0 0 0 0│ │ │ │ ║
      ╚─┴─┴─┴─┴╦┴╦┴╦┴╦╝ bit
               ║ ║ ║ ╚═ 0: 1=povolí přerušení od přijatého znaku
               ║ ║ ╚═══ 1: 1=povolí přerušení po odvysílání znaku
               ║ ╚═════ 2: 1=povolí přerušení od stavu přijímací linky
               ║            (error nebo break)
               ╚═══════ 3: 1=povolí přerušení od stavu modemu (CTS,DSR,RI,RLSD)
      ┌───────────────────┐
      │ Důležité poznámky │ Pokud chcete používat přerušení, nezapomeňte
      └───────────────────┘ na následující:
        Nastavit adresu svého obslužného podprogramu na příslušné vektory
        Naprogramovat  Řadič přerušení, aby od daného IRQ mohlo být
         generováno přerušení
        Nastavit bitovou masku do registru povolení přerušení, aby se
         generovalo přerušení od událostí jen toho typu, který chcete
        Nastavit bit OUT2 na 1 (viz níže).
      V obslužném programu přerušení pak musíte:
        Odhlásit řadiči přerušení přerušení (magická sekvence OUT 20H,20H)
        Vyčtením registru identifikace přerušení zjistit, o jakou událost se
         jedná
        Podle typu události podniknout příslušnou akci (převzít přijatý
         znak, zapsat do registru vysílače další znak atd.).

3faH  Čtení: Registr identifikace přerušení. Po příchodu přerušení vyčtěte tento
             registr, čímž zjistíte, co přerušení způsobilo.
      ╔7┬6┬5┬4┬3┬2┬1┬0╗
      ║0 0 0 0 0│   │ ║
      ╚─┴─┴─┴─┴─┴─┴─┴╦╝ bit
                 ╚╦╝ ╚═ 0: 1=žádné otevřené přerušení (lze použít pro polling)
                  ║         0=příčinu přerušení určují bity 2-1
                  ║
                  ╚══ 2-1: 00=Změna stavu modemu (některého ze signálů CTS,DSR,
                              RI, DCD). Nuluje se čtením stavu modemu (3feH)
                           01=registr vysílače byl vyprázdněn.
                              Nuluje se zápisem do portu 3f8H.
                           10=příjem dalšího znaku byl ukončen.
                              Nuluje se čtením portu přijímacího bufferu 3f8H
                           11=změna stavu přijímací linky: došlo k chybě
                              přetečení, parity, rámce nebo přetržení.
                              (overrun, parity, framing error, break)
                              Nuluje se přečtením stavu linky (port 3fdH).

      Poznámka: Kombinace bývají ve většině technických dokumentací
                přehozeny.

3faH  Zápis:  *** pouze 16550 *** Řídící registr fronty FIFO
      (FIFO = First in, first out, tedy první dovnitř, první ven)
      ╔7┬6┬5┬4┬3┬2┬1┬0╗
      ║   │0 0 0│ │ │ ║
      ╚─╬─┴─┴─┴─┴╦┴╦┴╦╝ bit
        ║        ║ ║ ╚═ 0: 1=povolení FIFO přijímače a vysílače
        ║        ║ ║        0=výmaz přijímací a vysílací fronty, znakový režim
        ║        ║ ╚═══ 1: 1=reset přijímacích FIFO registrů
        ║        ╚═════ 2: 1=reset vysílacích FIFO registrů
        ╚══════════════ 7-6: Nastavení FIFO registrů 00 = 1 byte
                                                      01 = 4 byty
                                                      10 = 8 bytů
                                                      11 = 14 bytů

3fbH  Čtení/Zápis: registr řízení linky
      ╔7┬6┬5┬4┬3┬2┬1┬0╗
      ║ │ │ │par│s│len║
      ╚╦┴╦┴╦┴─┴─┴╦┴─┴─╝ bit
       ║ ║ ║ ╚╦╝ ║ ╚═╩═ 0-1: délka slova: 00=5, 01=6, 10=7, 11=8
       ║ ║ ║  ║  ╚═════ 2: stop bity: 0=1, 1=2
       ║ ║ ║  ╚════════ 3-4 parita: x0=nic, 01=Lichá, 11=Sudá
       ║ ║ ╚═══════════ 5: zaseklá parita (nepoužito BIOSem)
       ║ ╚═════════════ 6: povolení kontroly break. 1=začne vysílat nulu
       ╚═══════════════ 7: DLAB (Divisor Latch Access Bit) - přístup do
                            registru dělitele na portech 3f8H a 3f9H.
                            1=nastav přenosovou rychlost, 0=normální stav

3fcH  Zápis: Registr řízení modemu
      ╔7┬6┬5┬4┬3┬2┬1┬0╗
      ║0 0 0│ │ │ │ │ ║
      ╚─┴─┴─┴╦┴╦┴╦┴╦┴╦╝ bit
             ║ ║ ║ ║ ╚═ 0: 1=aktivace -DTR (-data terminal ready), 0=deaktivace
             ║ ║ ║ ╚═══ 1: 1=aktivace -RTS (-request to send), 0=deaktivace
             ║ ║ ╚═════ 2: 1=aktivace -OUT1 (uživatelský výstup)
             ║ ╚═══════ 3: 1=aktivace -OUT2
             ╚═════════ 4: 1=aktivace zpětné vazby pro diagnostické účely
      ┌────────────┐
      │  Poznámky  │ Bity OUT1 a OUT2 jsou z hlediska výrobce příslušného
      └────────────┘ integrovaného obvodu vývody na pouzdře, jejichž funkce
      je k dispozici návrháři systému (prostě 2 programem snadno ovladatelné
      bity pro libovolné upotřebení). Ovšem návrháři IBM PC využili bit OUT2,
      který přivedli na vstup povolení přerušení obvodu. Jelikož v klidovém
      stavu je v bitu OUT2 zapsána 0, je přerušení ZAKÁZÁNO (blokováním na
      úrovni hardware), i když správně naprogramujete registr povolení
      přerušení a všechno ostatní. Pokud chcete přerušovat, musíte OUT2
      nastavit na 1.  Tahle zmínečka je jen v některých manuálech drobnými
      písmeny a kdyby se spočítaly všechny hodiny, po které si vývojáři
      lámali hlavy, proč jim to ne a ne přerušovat...  Moje skóre je 14 dní.

3fdH  Čtení: Registr stavu linky
      ╔7┬6┬5┬4┬3┬2┬1┬0╗
      ║0│ │ │ │ │ │ │ ║ Pozn.:bity 1-4 způsobují přerušení, je-li povoleno(3f9H)
      ╚─┴╦┴╦┴╦┴╦┴╦┴╦┴╦╝ bit
         ║ ║ ║ ║ ║ ║ ╚═ 0: 1=data ready (DR). Reset čtením přijatého znaku
         ║ ║ ║ ║ ║ ╚═══ 1: 1=overrun error (OE). Předchozí znak je ztracen
         ║ ║ ║ ║ ╚═════ 2: 1=parity error (PE). Reset čtením statusu linky
         ║ ║ ║ ╚═══════ 3: 1=framing error (FE). Špatný stop bit ve znaku
         ║ ║ ╚═════════ 4: 1=break indicated (BI). Přijata trvalá nula.
         ║ ╚═══════════ 5: 1=vyprázdnění registru vysílače. Může se vysílat
         ╚═════════════ 6: 1=vysílač je prázdný. Žádná data nejsou zpracovávána


3feH  Čtení: Registr stavu modemu
      ╔7┬6┬5┬4┬3┬2┬1┬0╗
      ║ │ │ │ │ │ │ │ ║ Pozn.:bity 0-3 způsobují přerušení, je-li povoleno(3f9H)
      ╚╦┴╦┴╦┴╦┴╦┴╦┴╦┴╦╝ bit
       ║ ║ ║ ║ ║ ║ ║ ╚═ 0: 1=Delta Clear To Send (DCTS) - CTS změnil stav
       ║ ║ ║ ║ ║ ║ ╚═══ 1: 1=Delta Data Set Ready (DDSR) - DSR změnil stav
       ║ ║ ║ ║ ║ ╚═════ 2: 1=Trailing Edge Ring Indicator (TERI)
       ║ ║ ║ ║ ║            vzestupná hrana indikátoru vyzvánění
       ║ ║ ║ ║ ╚═══════ 3: 1=Delta Data Carrier Detect (DDCD) - DCD změnil stav
       ║ ║ ║ ╚═════════ 4: 1=Clear To Send (CTS) je aktivní
       ║ ║ ╚═══════════ 5: 1=Data Set Ready (DSR) je aktivní
       ║ ╚═════════════ 6: 1=Ring Indicator (RI) je aktivní
       ╚═══════════════ 7: 1=Data Carrier Detect (DCD) je aktivní
```

4
Zapojení konektoru RS-232C
---

```
        Canon 25 pin - samec           Canon 9 pin - samec
                ┌───┐                           ┌───┐
             ┌──┘ 1o│                       ┌───┘ 1o│  DCD
             │o14   │                  DSR  │o6     │
             │    2o│  TxD                  │     2o│  RxD
             │o     │                  RTS  │o7     │
             │    3o│  RxD                  │     3o│  TxD
             │o     │                  CTS  │o8     │
             │    4o│  RTS                  │     4o│  DTR
             │o     │                  RI   │o9     │
             │    5o│  CTS                  └───┐ 5o│  GND
             │o     │                           └───┘
             │    6o│  DSR
             │o     │               POZOR: Piny 2 a 3 mají na 9 a 25-pino-
             │    7o│  GND                 vých konektorech přehozené
        DTR  │o20   │                      významy (svádí to k chybě, která
             │    8o│  DCD                 se dost špatně hledá).
             │o     │
             │     o│
        RI   │o22   │
             │     o│
             │o     │
             │     o│
             │o     │
             │     o│
             │o25   │
             └──┐13o│
                └───┘

```


                                     5
Protokol DISTEL
---
Základní složení tohoto protokolu je následující:

```
           ┌────────┬───────┬────────┬────────────────────┬────────┐
  Velikost │  1 B   │  1 B  │  1 B   │      [LN] B        │  1 B   │
  Popis    │ Adresa │ Délka │ Příkaz │                    │ Součet │
  Označení │  ADR   │  LN   │  CMD   │       DATA         │  CRC   │
           └────────┴───────┴────────┴────────────────────┴────────┘
```

"Adresa" - Adresa zařízení
"Délka" - Délka dat - začíná bytem CMD a končí bytem CRC
"Příkaz" - Operační příkaz pro zařízení
"DATA" - Přenášená data
"Součet" - Kontrolní součet bytů ADR + LN + CMD + DATA

Byte Adresa je jedničkové parity, ostatní byty jsou nulové parity.


