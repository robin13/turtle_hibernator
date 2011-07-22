REM *** you need to edit this file and adapt it to your WinAVR 
REM *** installation. E.g replace c:\avrgcc by c:\WinAVR-20090313
@echo -------- begin --------

set AVR=c:\avrgcc

set CC=avr-gcc

set PATH=c:\avrgcc\bin;c:\avrgcc\utils\bin

make -f Makefile

@echo --------  end  --------
pause
