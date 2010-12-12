# makefile, written by guido socher
#MCU=atmega168
#DUDECPUTYPE=m168
MCU=atmega328p
DUDECPUTYPE=m328p
#MCU=atmega644
#DUDECPUTYPE=m644
#
# === Edit this and enter the correct device/com-port:
# linux (plug in the avrusb500 and type dmesg to see which device it is):
LOADCMD=avrdude -P /dev/ttyUSB0

# mac (plug in the programer and use ls /dev/tty.usbserial* to get the name):
#LOADCMD=avrdude -P /dev/tty.usbserial-A9006MOb

# windows (check which com-port you get when you plugin the avrusb500):
#LOADCMD=avrdude -P COM4

# All operating systems: if you have set the default_serial paramter 
# in your avrdude.conf file correctly then you can just use this
# and you don't need the above -P option:
#LOADCMD=avrdude
# === end edit this
#
LOADARG=-p $(DUDECPUTYPE) -c stk500v2 -e -U flash:w:
#
CC=avr-gcc
OBJCOPY=avr-objcopy
# optimize for size:
CFLAGS=-g -mmcu=$(MCU) -Wall -W -Os -mcall-prologues
#-------------------
.PHONY: all
#
all: main.hex 
	@echo "done"
#
main: main.hex
	@echo "done"
#
size: 
	avr-size *.elf
#-------------------
help: 
	@echo "Usage: make all|load|main|rdfuses|size"
	@echo "or"
	@echo "Usage: make clean"
	@echo " "
	@echo "For boards with clock from enc38j60 (all new boards): make fuse"
#-------------------
main.hex: main.elf 
	$(OBJCOPY) -R .eeprom -O ihex main.elf main.hex 
	avr-size main.elf
	@echo " "
	@echo "Expl.: data=initialized data, bss=uninitialized data, text=code"
	@echo " "

main.elf: main.o ip_arp_udp_tcp.o enc28j60.o websrv_help_functions.o sensirion_protocol.o crc8.o onewire.o ds18x20.o
	$(CC) $(CFLAGS) -o main.elf -Wl,-Map,main.map main.o ip_arp_udp_tcp.o enc28j60.o websrv_help_functions.o sensirion_protocol.o crc8.o onewire.o ds18x20.o
websrv_help_functions.o: websrv_help_functions.c websrv_help_functions.h ip_config.h 
	$(CC) $(CFLAGS) -Os -c websrv_help_functions.c
enc28j60.o: enc28j60.c timeout.h enc28j60.h
	$(CC) $(CFLAGS) -Os -c enc28j60.c
ip_arp_udp_tcp.o: ip_arp_udp_tcp.c net.h enc28j60.h ip_config.h
	$(CC) $(CFLAGS) -Os -c ip_arp_udp_tcp.c
main.o: main.c ip_arp_udp_tcp.h enc28j60.h timeout.h net.h websrv_help_functions.h ip_config.h sensirion_protocol.h  onewire.h ds18x20.h
	$(CC) $(CFLAGS) -Os -c main.c
sensirion_protocol.o: sensirion_protocol.c sensirion_protocol.h 
	$(CC) $(CFLAGS) -Os -c sensirion_protocol.c
onewire.o: onewire.c onewire.h 
	$(CC) $(CFLAGS) -Os -c onewire.c
ds18x20.o: ds18x20.c ds18x20.h crc8.h
	$(CC) $(CFLAGS) -Os -c ds18x20.c
crc8.o: crc8.c crc8.h 
	$(CC) $(CFLAGS) -Os -c crc8.c
#------------------
#pre: main.hex
#	cp main.hex main_pre168.hex
#
load: main.hex
	$(LOADCMD) $(LOADARG)main.hex
#
#-------------------
# Check this with make rdfuses
rdfuses:
	$(LOADCMD) -p $(DUDECPUTYPE) -c stk500v2 -v -q
#
fuse:
	$(LOADCMD) -p  $(DUDECPUTYPE) -c stk500v2 -u -v -U lfuse:w:0x60:m
fuses:
	$(LOADCMD) -p  $(DUDECPUTYPE) -c stk500v2 -u -v -U lfuse:w:0x60:m
#-------------------
clean:
	rm -f *.o *.map *.elf main.hex
#-------------------
