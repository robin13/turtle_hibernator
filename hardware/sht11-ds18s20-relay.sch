v 20080127 1
T 13100 26300 9 16 1 0 0 0 1
Connection of relay, SHT11 and DS18s20 sensors to the tuxgraphics ethernet board
T 22700 17000 9 12 1 0 0 0 2
(C) Guido Socher, tuxgraphics.org
http://tuxgraphics.org/electronics/
C 23600 19100 1 0 0 ATMEGA-168DIP_ph.sym
{
T 25900 25000 5 10 1 1 0 6 1
refdes=U1
T 23800 18800 5 10 1 1 0 0 1
value=ATMEGA168
T 23600 19100 5 10 0 1 0 0 1
footprint=DIP28N
}
C 21500 25400 1 0 0 vccarrow-1.sym
{
T 20800 25600 5 10 1 1 0 0 1
value=3V3
}
C 21700 25400 1 90 1 resistor-2.sym
{
T 21300 24600 5 10 1 1 0 6 1
refdes=R1
T 21300 24900 5 10 1 1 0 6 1
value=10K
T 21700 25400 5 10 0 1 0 2 1
footprint=R0w4
}
C 26200 21400 1 270 0 vccarrow-1.sym
C 26500 22000 1 90 0 gnd-1.sym
C 23300 21800 1 270 0 gnd-1.sym
C 23600 22000 1 90 0 vccarrow-1.sym
{
T 23400 22400 5 10 1 1 180 0 1
value=3V3
}
C 20100 21000 1 0 0 gnd-1.sym
C 20100 17600 1 0 0 gnd-1.sym
C 21800 18300 1 0 0 resistor-2.sym
{
T 23000 18200 5 10 1 1 0 8 1
refdes=R10
T 22500 18200 5 10 1 1 180 0 1
value=1K
T 21800 18300 5 10 0 1 270 0 1
footprint=R0w4
}
N 22700 18400 22800 18400 4
N 22800 18400 22800 19700 4
N 22800 19700 23600 19700 4
N 20800 18400 21800 18400 4
C 20300 20600 1 270 0 diode-3.sym
{
T 19900 20300 5 10 1 1 0 0 1
refdes=D1
T 20300 20600 5 10 0 1 270 0 1
footprint=R0w8
T 19500 20000 5 10 1 1 0 0 1
value=BAY69
}
C 20400 20600 1 0 0 vddarrow-1.sym
{
T 19600 20700 5 10 1 1 0 0 1
value=RAWDC
}
C 23900 18100 1 90 1 vccarrow-1.sym
{
T 23700 18600 5 10 1 1 270 2 1
value=3V3
}
C 25200 18100 1 180 1 led-2.sym
{
T 25500 18300 5 10 1 1 180 6 1
refdes=D2
T 25200 18100 5 10 0 1 180 6 1
footprint=LED 3
}
C 24300 17900 1 0 0 resistor-2.sym
{
T 24500 17800 5 10 1 1 0 8 1
refdes=R13
T 25000 17800 5 10 1 1 180 0 1
value=270
T 24300 17900 5 10 0 1 270 0 1
footprint=R0w4
}
N 26200 19300 26200 18000 4
N 26200 18000 26100 18000 4
N 23900 18000 24300 18000 4
C 18500 19300 1 0 0 connector2-1.sym
{
T 18500 20100 5 10 1 1 0 0 1
refdes=CONN3
T 18500 19300 5 10 0 1 0 0 1
footprint=CONNECTOR 2 1
}
N 23600 20100 22300 20100 4
N 22300 20100 22300 21900 4
N 20200 21900 22300 21900 4
N 23600 20500 22600 20500 4
N 22600 20500 22600 22200 4
N 22600 22200 20200 22200 4
N 20200 22500 23600 22500 4
N 23600 22900 22600 22900 4
N 22600 22900 22600 22800 4
N 22600 22800 20200 22800 4
C 20800 17900 1 0 1 BC547-1.sym
{
T 19900 18400 5 10 1 1 0 6 1
refdes=Q1
T 20800 17900 5 10 0 1 0 0 1
footprint=TO92_BC5x7
T 19300 17900 5 10 1 1 0 0 1
value=or BC548 
}
T 18400 18800 9 10 1 0 0 0 2
external
relay
C 20800 20600 1 270 0 capacitor-1.sym
{
T 21400 20500 5 10 1 1 180 0 1
refdes=C11
T 21500 20800 5 10 1 1 180 0 1
value=100nF
T 20800 20600 5 10 0 1 90 0 1
footprint=C100
}
N 20200 20600 21000 20600 4
N 20200 20600 20200 19800 4
N 20200 19500 21000 19500 4
N 21000 19500 21000 19700 4
N 20200 18900 20200 19500 4
N 20200 21600 22000 21600 4
N 22000 21600 22000 19300 4
N 22000 19300 23600 19300 4
N 23600 24500 21600 24500 4
N 20200 23100 22400 23100 4
N 22400 23100 22400 23300 4
N 22400 23300 23600 23300 4
N 20200 23400 22100 23400 4
N 22100 23400 22100 23700 4
N 22100 23700 23600 23700 4
N 21800 24100 21800 23700 4
N 21800 23700 20200 23700 4
N 21800 24100 23600 24100 4
C 20200 23900 1 270 1 vccarrow-1.sym
{
T 20400 24300 5 10 1 1 180 6 1
value=3V3
}
C 18500 21100 1 0 0 connector10-1.sym
{
T 18500 24400 5 10 1 1 0 0 1
refdes=CONN5
T 18500 21100 5 10 0 1 0 0 1
footprint=CONNECTOR 10 1
}
N 20500 19700 20500 19500 4
C 15800 22400 1 180 1 connector3-1.sym
{
T 15500 21300 5 10 1 1 180 6 1
refdes=DS18s20
}
B 19200 16700 7800 9300 3 0 0 0 -1 -1 0 -1 -1 -1 -1 -1
T 25700 25700 9 12 1 0 0 0 1
eth. board
C 17400 17300 1 0 1 relay-1.sym
L 16500 19400 16500 20100 3 0 0 0 -1 -1
V 16400 19300 100 3 0 0 0 -1 -1 0 -1 -1 -1 -1 -1
N 17100 20100 18000 20100 4
N 18000 20100 18000 19800 4
N 18000 19800 19200 19800 4
N 17100 17300 18000 17300 4
N 18000 17300 18000 19500 4
N 18000 19500 19200 19500 4
T 22200 25300 9 10 1 0 0 0 1
Not all components of the eth. board are shown here.
T 19100 16000 9 12 1 0 0 6 3
The rating of the relay depends on the power supply  voltage (Vdd) you use. 
Use a 6V relay for power supply  voltages between 5V and 9V otherwise a 
12V relay. Pay attention to proper insulation.
N 17500 24000 17500 22200 4
N 17500 21900 19200 21900 4
N 17500 20600 17500 21600 4
N 14300 24000 19200 24000 4
T 15700 22600 9 12 1 0 0 0 1
Outdoor
C 12600 23200 1 180 1 connector4-1.sym
{
T 14400 22300 5 10 0 0 180 6 1
device=CONNECTOR_4
T 12600 21800 5 10 1 1 180 6 1
refdes=SHT11
}
T 13300 22200 9 10 1 0 0 0 1
GND
T 13300 22500 9 10 1 0 0 0 1
DATA
T 16700 22000 9 10 1 0 0 0 1
DATA
T 16700 21700 9 10 1 0 0 0 1
GND
T 16700 22300 9 10 1 0 0 0 1
Vcc
T 13300 23100 9 10 1 0 0 0 1
Vcc
T 13300 22800 9 10 1 0 0 0 1
SCK
N 14300 24000 14300 23000 4
N 14300 20600 14300 22100 4
N 14300 22400 15400 22400 4
N 15400 22400 15400 23700 4
N 15400 23700 19200 23700 4
N 14300 22700 15100 22700 4
N 15100 20900 15100 22700 4
N 15100 20900 18000 20900 4
N 18000 20900 18000 21600 4
N 18000 21600 19200 21600 4
N 19200 21300 18300 21300 4
N 18300 20600 18300 21300 4
N 14300 20600 18300 20600 4
T 12500 23400 9 12 1 0 0 0 1
Indoor
T 18000 23800 9 10 1 0 0 0 1
PD0
T 17500 24100 9 10 1 0 0 0 1
Vcc +3.3V
T 18000 22000 9 10 1 0 0 0 1
PD6
T 18000 21700 9 10 1 0 0 0 1
PB0
C 17900 23400 1 90 1 resistor-2.sym
{
T 18300 22600 5 10 1 1 0 6 1
refdes=Rp
T 17400 23100 5 10 1 1 0 6 1
value=10K or 4K7
T 17900 23400 5 10 0 1 0 2 1
footprint=R0w4
}
N 17800 23400 17500 23400 4
N 17800 22500 17800 21900 4
T 19100 25000 9 12 1 0 0 6 2
The pull-up resistor Rp for the ds18s20 sensor
is only needed for longer cables.
