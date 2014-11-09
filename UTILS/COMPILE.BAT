@ECHO OFF
REM       Univerzalni davkovy soubor pro radkouvou kompilaci
REM  pokud v adresari kompilovaneho zdrojaku neexistuje, zkopiruje se
REM  tam, jinak se preda rizeni batchi se stejnym jmenem v cilovem adresari
REM ------------------------------------------------------------------------
REM        1. Parametr - Cesta k tomuto souboru (napr. "C:\PRGJAZ\MODEM")
REM        2. Parametr - Jmeno souboru bez pripony (napr. "MODEM")
REM        3. Parametr - Pripona souboru (napr. ".A51")
REM        4. Parametr - RUN_ISP_LOADER ... Po kompilaci spusti ISP loader
REM                      INDIVIDUAL ... Spusti se batch v cilovem adresari
REM ------------------------------------------------------------------------

IF "%1"=="" GOTO HELPEXIT
IF "%4"=="INDIVIDUAL" GOTO RUNBATCH
IF EXIST %1COMPILE.BAT GOTO NEXT
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
COPY %0 %1COMPILE.BAT
ECHO Vytvoren individualni davkovy soubor v adresari %1
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

:NEXT
CD %1
ECHO %1
COMPILE.BAT %1 %2 %3 INDIVIDUAL

:RUNBATCH
REM Automaticke urceni diskoveho oddilu
IF EXIST F:\PRGJAZ\Utils\compile.bat SET %PRGDRV=F
IF EXIST E:\PRGJAZ\Utils\compile.bat SET %PRGDRV=E
IF EXIST D:\PRGJAZ\Utils\compile.bat SET %PRGDRV=D
IF EXIST C:\PRGJAZ\Utils\compile.bat SET %PRGDRV=C


REM --------------- Rozliseni kompilatoru podle pripony --------------------
IF %3==.PAS GOTO PAS_PREKLAD
IF %3==.A51 GOTO A51_4_PREKLAD
IF %3==.C51 GOTO C51_4_PREKLAD
IF %3==.C   GOTO CPlus_PREKLAD
IF %3==.C32 GOTO C32_PREKLAD
IF %3==.BAT GOTO BAT_ECHO
GOTO UNSIGNED_EXTENSION
REM ------------------------------------------------------------------------



REM ------------------------ Preklad pro PAS -------------------------------
:PAS_PREKLAD
SET TPCBIN=:\PRGJAZ\PASCAL\BP7
ECHO Kompilace Turbo Pascal
%1%:
CD %1
IF NOT EXIST %2.PAS ECHO Soubor nenalezen "%2.PAS"
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
%PRGDRV%%TPC%\TPC.EXE %2.PAS
ECHO.
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
PAUSE > NUL
%2.EXE
PAUSE > NUL
GOTO QUIT
REM ------------------------------------------------------------------------


REM ------------------------ Preklad pro C51 v4.01 -------------------------
:C51_4_PREKLAD
SET C51LIB=%PRGDRV%:\PRGJAZ\CPU51\C51_v401\LIB
SET C51INC=%PRGDRV%:\PRGJAZ\CPU51\C51_v401\INC
SET C51PATH=%PRGDRV%:\PRGJAZ\CPU51\C51_v401\BIN
ECHO Kompilace C51 v4.01
REM %PRGDRV%:\PRGJAZ\CPU51\LOADER\ENABLE.EXE 378h
%1%:
CD %1
IF NOT EXIST %2.C51 ECHO Soubor nenalezen "%2.C51"
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
ECHO LIB=%C51LIB%
ECHO INC=%C51INC%
ECHO BIN=%C51PATH%
ECHO C51: "%2.C51" TO "%2.obj"
%C51PATH%\C51.EXE %2.C51 DEBUG CODE LISTINCLUDE |MORE
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

PAUSE >nul
CLS

IF NOT EXIST %2.OBJ ECHO Soubor nenalezen "%2.obj"
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
ECHO L51: "%2.obj" TO "%2.omf"

IF EXIST %2.CMD GOTO CX4
IF EXIST LINKER.CMD GOTO CLINKER4
ECHO.
ECHO Soubor LINKER.CMD nenalezen !
GOTO CNOT4

:CLINKER4
 %C51PATH%\L51.EXE @LINKER.CMD | more
GOTO CNOT4

:CX4
%C51PATH%\L51.EXE @%2.CMD | more

:CNOT4
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

PAUSE >nul
CLS

IF NOT EXIST %2.OMF ECHO Soubor nenalezen "%2.omf"
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
ECHO OHS51: "%2.omf" TO "%2.hex"
%C51PATH%\OH51.EXE %2.OMF
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

PAUSE >NUL
IF %%4==RUN_ISP_LOADER GOTO RUN_ISP_LOADER
GOTO QUIT
REM ------------------------------------------------------------------------


REM ------------------------ Preklad pro C51 v6.12 -------------------------
:C51_6_PREKLAD
SET C51LIB=%PRGDRV%:\PRGJAZ\CPU51\C51_v612\C51\LIB
SET C51INC=%PRGDRV%:\PRGJAZ\CPU51\C51_v612\C51\INC
SET C51PATH=%PRGDRV%:\PRGJAZ\CPU51\C51_v612\C51\BIN
ECHO Kompilace C51 v6.12
CD %1
IF NOT EXIST %2.C51 ECHO Soubor nenalezen "%2.C51"
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
ECHO C51: "%2.C51" TO "%2.obj"
%C51PATH%\C51.EXE %2.C51 DEBUG CODE LISTINCLUDE |MORE
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

PAUSE >nul
CLS

ECHO Linker v4.13
IF NOT EXIST %2.OBJ ECHO Soubor nenalezen "%2.obj"
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
ECHO L51: "%2.obj" TO "%2.omf"
IF EXIST %2.CMD GOTO CX6
IF EXIST LINKER.CMD GOTO CLINKER6
ECHO.
ECHO Soubor LINKER.CMD nenalezen !
GOTO CNOT6

:CLINKER6
%C51PATH%\BL51.EXE @LINKER.CMD | more
GOTO CNOT

:CX6
%C51PATH%\BL51.EXE @%2.CMD | more

:CNOT6
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
PAUSE >nul
CLS
IF NOT EXIST %2.OMF ECHO Soubor nenalezen "%2.omf"
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
ECHO OHS51: "%2.omf" TO "%2.hex"
%C51PATH%\OH51.EXE %2.OMF
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

PAUSE >NUL
IF %%4==RUN_ISP_LOADER GOTO RUN_ISP_LOADER
GOTO QUIT
REM ------------------------------------------------------------------------




REM ------------------------ Preklad pro A51 v4.4 -------------------------
:A51_4_PREKLAD
SET A51BIN=:\PRGJAZ\CPU51\A51
CLS
ECHO Kompilace A51 V4.4
CD %1
REM IF NOT EXIST REG51.INC COPY %PRGDRV%:\PRGJAZ\CPU51\A51\INC\REG51.INC >NUL
REM IF NOT EXIST REG52.INC COPY %PRGDRV%:\PRGJAZ\CPU51\A51\INC\REG52.INC >NUL
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
ECHO A51: "%2%3" TO "%2.obj"
%PRGDRV%%A51BIN%\A51.EXE %2%3 DEBUG SYMBOLS XREF ERRORPRINT |MORE
IF ERRORLEVEL==1 ECHO ERROR IN "%2%3"
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
PAUSE >NUL
IF %%4==RUN_ISP_LOADER GOTO RUN_ISP_LOADER
GOTO QUIT

REM ------------------------ Preklad pro A51 v6.12 -------------------------
:A51_6_PREKLAD
SET A51INC=:\PRGJAZ\CPU51\C51_V612\INC
SET A51BIN=:\PRGJAZ\CPU51\C51_V612
CLS
ECHO Kompilace A51
CD %1
IF NOT EXIST REG51.H COPY %PRGDRV%%A51INC%\REG51.INC >NUL
IF NOT EXIST REG52.H COPY %PRGDRV%%A51INC%\REG52.INC >NUL
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
ECHO A51: "%2%3" TO "%2.obj"
%PRGDRV%%A51BIN%\A51.EXE %2%3 NODEBUG SYMBOLS XREF NOMOD51 ERRORPRINT |MORE
IF ERRORLEVEL==1 ECHO ERROR IN "%2%3"
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
PAUSE >NUL
IF %%4==RUN_ISP_LOADER GOTO RUN_ISP_LOADER
GOTO QUIT
REM ------------------------------------------------------------------------


REM ------------------------ ISP Loader ------------------------------------
:RUN_ISP_LOADER
%PRGDRV%:\PRGJAZ\CPU51\FLASHER\rd2f.exe -f %3.hex -c COM1 -b 19200 -m 0 -p 0 -t 2
GOTO QUIT
REM ------------------------------------------------------------------------


REM ------------------------ Preklad pro C++ -------------------------------
:CPlus_PREKLAD
SET CBIN=:\PrgJaz\C\BC\BIN
CLS
ECHO Kompilace C++
CD %1
ECHO %1%2%3
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
%PRGDRV%%CBIN%\BC.EXE /b /p %2%3
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
IF ERRORLEVEL==1 GOTO CPLUSERR
ECHO Press to run EXE...
PAUSE >NUL
%2.EXE
GOTO QUIT

:CPLUSERR
ECHO ERROR IN "%2%3"                     Press to Exit...
PAUSE >NUL
GOTO QUIT
REM ------------------------------------------------------------------------


REM ------------------------ Preklad pro C++ 32 -------------------------------
:C32_PREKLAD
SET C32BIN=:\PrgJaz\C\BC5\BIN
SET C32INC=:\PRGJAZ\C\BC5\Include
CLS
ECHO Kompilace C++ 32
SET PATH=%PATH%; %PRGDRV%%C32BIN%;
ECHO %1%2%3
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
%PRGDRV%%C32BIN%\BCC32.EXE -n%1 -I%PRGDRV%%C32INC% %2%3 | more
IF ERRORLEVEL==1 ECHO ERROR IN "%2%3"
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
PAUSE >NUL
rem %2.EXE
GOTO QUIT
REM ------------------------------------------------------------------------

:BAT_ECHO
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
ECHO     Soubory BAT nejsou soucasti netopyra a neni tudiz treba je prekladat!
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
PAUSE >NUL
GOTO QUIT
REM ------------------------------------------------------------------------

REM ------------------- Pro priponu neni definovan preklad -----------------
:UNSIGNED_EXTENSION
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
ECHO       Pro soubor %2%3 neni definovan zadny prekladac !
ECHO 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
PAUSE >NUL
GOTO QUIT
REM ------------------------------------------------------------------------

:HELPEXIT
ECHO              Univerzalni davkovy soubor pro radkouvou kompilaci
ECHO  ------------------------------------------------------------------------
ECHO   Pokud v adresari zdrojoveho kodu neexistuje, zkopiruje se tam,
ECHO   jinak se preda rizeni batchi se stejnym jmenem v cilovem adresari.
ECHO   Je tak mozne davkovy soubor u kazdeho zdrojoveho kodu individualne
ECHO   prizpusobit.
ECHO  ------------------------------------------------------------------------
ECHO.
ECHO.
ECHO                        Chybi pozadovane parametry !
ECHO.
ECHO.
ECHO  ------------------------------------------------------------------------
ECHO         1. Parametr - Cesta k tomuto souboru (napr. "C:\PRGJAZ\MODEM")
ECHO         2. Parametr - Jmeno souboru bez pripony (napr. "MODEM")
ECHO         3. Parametr - Pripona souboru (napr. ".A51")
ECHO         4. Parametr - RUN_ISP_LOADER ... Po kompilaci spusti ISP loader
ECHO                       INDIVIDUAL ... Spusti se batch v cilovem adresari
ECHO  ------------------------------------------------------------------------

PAUSE > NUL



:QUIT



