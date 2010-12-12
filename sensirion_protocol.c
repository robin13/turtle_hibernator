/*********************************************
 * vim:sw=8:ts=8:si:et
 * This is the driver code for the sensirion temperature and
 * humidity sensor. 
 *
 * Based on ideas from the sensirion application note and modified
 * for atmega88/168. 
 * A major part of the code was optimized and as a result the compiled 
 * code size was reduced to 50%.
 *
 * Modifications by: Guido Socher 
 *
 * Note: the sensirion SHTxx sensor does _not_ use the industry standard
 * I2C. The sensirion protocol looks similar but the start/stop
 * sequence is different. You can not use the avr TWI module.
 *
 *********************************************/
#include <avr/io.h>
#define F_CPU 12500000UL  // 12.5 MHz
#include <util/delay.h>
                          //adr command r/w
#define STATUS_REG_W 0x06 //000 0011 0
#define STATUS_REG_R 0x07 //000 0011 1
#define MEASURE_TEMP 0x03 //000 0001 1
#define MEASURE_HUMI 0x05 //000 0010 1
#define RESET 0x1e        //000 1111 0

// uncommend this to use crc checks, about 250 bytes more code
#define USESHT11CRC 1
// physical connection:
// All sensors use the same SCK pin.
#define SETSCK1 PORTB|=(1<<PORTB0)
#define SETSCK0 PORTB&=~(1<<PORTB0)
#define SCKOUTP DDRB|=(1<<DDB0)
//
// Define where the DATA pin of each sensor is connected.
// Everything must be connected to port D
static uint8_t gSensor2dataDDR[4] = {DDD0,DDD1,DDD4,DDD5};
static uint8_t gSensor2dataPORT[4] = {PORTD0,PORTD1,PORTD4,PORTD5};
static uint8_t gSensor2dataPIN[4] = {PIND0,PIND1,PIND4,PIND5};
// the current measurement mode, storage area for each sensor is needed here:
static uint8_t gSensor2mode[4];
//
#define SETDAT1(x) PORTD|=(1<<gSensor2dataPORT[x])
#define SETDAT0(x) PORTD&=~(1<<gSensor2dataPORT[x])
#define GETDATA(x) (PIND&(1<<gSensor2dataPIN[x]))
//
#define DMODEIN(x) DDRD&=~(1<<gSensor2dataDDR[x])
#define PULLUP1(x) PORTD|=(1<<gSensor2dataPIN[x])
#define DMODEOU(x) DDRD|=(1<<gSensor2dataDDR[x])

//pulswith long 3us
#define S_PULSLONG _delay_loop_1(15)
//pulswith short 1us
#define S_PULSSHORT _delay_loop_1(6)

#ifdef USESHT11CRC
// Compute the CRC8 value of a data set.
//
//  This function will compute the CRC8 of inData using seed
//  as inital value for the CRC.
//
//  This function was copied from Atmel avr318 example files.
//  It is more suitable for microcontroller than the code example
//  in the sensirion CRC application note.
//
//  inData  One byte of data to compute CRC from.
//
//  seed    The starting value of the CRC.
//
//  return The CRC8 of inData with seed as initial value.
//
//  note   Setting seed to 0 computes the crc8 of the inData.
//
//  note   Constantly passing the return value of this function 
//         As the seed argument computes the CRC8 value of a
//         longer string of data.
//
uint8_t computeCRC8(uint8_t inData, uint8_t seed)
{
    uint8_t bitsLeft;
    uint8_t tmp;

    for (bitsLeft = 8; bitsLeft > 0; bitsLeft--)
    {
        tmp = ((seed ^ inData) & 0x01);
        if (tmp == 0)
        {
            seed >>= 1;
        }
        else
        {
            seed ^= 0x18;
            seed >>= 1;
            seed |= 0x80;
        }
        inData >>= 1;
    }
    return seed;    
}

// sensirion has implemented the CRC the wrong way round. We
// need to swap everything.
// bit-swap a byte (bit7->bit0, bit6->bit1 ...)
uint8_t bitswapbyte(uint8_t byte)
{
        uint8_t i=8;
        uint8_t result=0;
        while(i){ 
		result=(result<<1);
                if (1 & byte) {
			result=result | 1;
                }
		i--;
		byte=(byte>>1);
        }
	return(result);
}
#endif // USESHT11CRC

// writes a byte on the Sensibus and checks the acknowledge
char s_write_byte(uint8_t value,uint8_t sensor)
{
        uint8_t i=0x80;
        uint8_t error=0;
        DMODEOU(sensor); 
        while(i){ //shift bit for masking
                if (i & value) {
                        SETDAT1(sensor); //masking value with i , write to SENSI-BUS
                }else{ 
                        SETDAT0(sensor);
                }
                S_PULSSHORT; // improve cable length
                SETSCK1; //clk for SENSI-BUS
                S_PULSLONG;
                SETSCK0;
                S_PULSSHORT;
                i=(i>>1);
        }
        DMODEIN(sensor); //release DATA-line
        PULLUP1(sensor);
        SETSCK1; //clk #9 for ack
        S_PULSLONG;
        if (GETDATA(sensor)){ //check ack (DATA will be pulled down by SHT11)
                error=1;
        }
        S_PULSSHORT;
        SETSCK0;
        return(error); //error=1 in case of no acknowledge
}

// reads a byte form the Sensibus and gives an acknowledge in case of "ack=1"
// reversebits=1 caused the bits to be reversed (bit0=bit7, bit1=bit6,...)
uint8_t s_read_byte(uint8_t ack,uint8_t sensor)
{
        uint8_t i=0x80;
        uint8_t val=0;
        DMODEIN(sensor); //release DATA-line
        PULLUP1(sensor);
        S_PULSSHORT; // improve cable length
        while(i){ //shift bit for masking
                SETSCK1; //clk for SENSI-BUS
                S_PULSSHORT;
                if (GETDATA(sensor)){
                        val=(val | i); //read bit
                }
                SETSCK0;
                S_PULSSHORT;
                i=(i>>1);
        }
        DMODEOU(sensor); 
        if (ack){
                //in case of "ack==1" pull down DATA-Line
                SETDAT0(sensor);
        }else{
                SETDAT1(sensor);
        }
        SETSCK1; //clk #9 for ack
        S_PULSLONG;
        SETSCK0;
        S_PULSSHORT;
        DMODEIN(sensor); //release DATA-line
        PULLUP1(sensor);
        return (val);
}

// generates a sensirion specific transmission start
// This is the point where sensirion is not I2C standard conform and the
// main reason why the AVR TWI hardware support can not be used.
//       _____         ________
// DATA:      |_______|
//           ___     ___
// SCK : ___|   |___|   |______
void s_transstart(uint8_t sensor)
{
        //Initial state
        SCKOUTP;
        SETSCK0;
        DMODEOU(sensor); 
        SETDAT1(sensor);
        //
        S_PULSSHORT;
        SETSCK1;
        S_PULSSHORT;
        SETDAT0(sensor);
        S_PULSSHORT;
        SETSCK0;
        S_PULSLONG;
        SETSCK1;
        S_PULSSHORT;
        SETDAT1(sensor);
        S_PULSSHORT;
        SETSCK0;
        S_PULSSHORT;
        //
        DMODEIN(sensor); //release DATA-line
        PULLUP1(sensor);
}

// communication reset: DATA-line=1 and at least 9 SCK cycles followed by transstart
//      _____________________________________________________         ________
// DATA:                                                     |_______|
//          _    _    _    _    _    _    _    _    _        ___    ___
// SCK : __| |__| |__| |__| |__| |__| |__| |__| |__| |______|  |___|   |______
void s_connectionreset(uint8_t sensor)
{
        uint8_t i;
        //Initial state
        SCKOUTP;
        SETSCK0;
        DMODEOU(sensor); 
        SETDAT1(sensor);
        S_PULSSHORT;
        for(i=0;i<9;i++){ //9 SCK cycles
                SETSCK1;
                S_PULSLONG;
                SETSCK0;
                S_PULSLONG;
        }
        s_transstart(sensor); //transmission start
}

// resets the sensor by a softreset
char s_softreset(uint8_t sensor)
{
        s_connectionreset(sensor); //reset communication
        //send RESET-command to sensor:
        return (s_write_byte(RESET,sensor)); //return=1 in case of no response form the sensor
}


// makes a measurement (humidity/temperature) 
// return value: 1=write error, 0=ok wait for data
// if it returns OK then you must  call s_readmeasurement immediately 
// afterwards for the same sensor (sck pin must not change)
char s_measure(uint8_t mode, uint8_t sensor)
{
        s_transstart(sensor); //transmission start
        gSensor2mode[sensor]=mode;
        if (s_write_byte(mode,sensor)){
                return(1);
        }
        return(0);
}

// return 1 if previously started measurement is ready
//
// Note that you can not wait too long. It takes between
// 60ms to 220ms for the measurement to complete and the
// sht11 pulls unfortunately the line only to gnd for a
// moment.  You should pull this function more or less
// constantly after the start of the measurement.
uint8_t s_resultready(uint8_t sensor)
{
        if (GETDATA(sensor)!=0){
                return(0);
        }
        return(1);
}

// read previously initiated measurement 
// p_value returns 2 bytes
// mode: 1=humidity  0=temperature
// return value: 2=crc error
//
char s_readmeasurement(unsigned int *p_value, uint8_t sensor)
{
        uint8_t msb,lsb;
        uint8_t checksum;
#ifdef USESHT11CRC
        uint8_t crc_state=0; 
        // the crc8 is computed over the entire communication from command to response data:
        crc_state=computeCRC8(bitswapbyte(gSensor2mode[sensor]),crc_state);
#endif
        msb=s_read_byte(1,sensor); //read the first byte (MSB)
#ifdef USESHT11CRC
        crc_state=computeCRC8(bitswapbyte(msb),crc_state);
#endif
        lsb=s_read_byte(1,sensor); //read the second byte (LSB)
        *p_value=(msb<<8)|(lsb);
#ifdef USESHT11CRC
        crc_state=computeCRC8(bitswapbyte(lsb),crc_state);
#endif
        checksum =s_read_byte(0,sensor); //read checksum
#ifdef USESHT11CRC
        if (crc_state != checksum ) {
                return(2);
        }
#endif
        return(0);
}

// calculates temperature [C] 
// input : temp [Ticks] (14 bit)
// output: temp [C] times 10 (e.g 253 = 25.3'C)
// Sensor supply voltage: about 3.3V
//
int calc_sth11_temp(unsigned int t)
{
        //t = 10*(t *0.01 - 39.7);
        //or
        //t = t *0.1 - 397;
        t/=10;
        t-=397;
        return(t);
}

// calculates humidity [%RH]
// The relative humitidy is: -0.0000028*s*s + 0.0405*s - 4.0
// but this is very difficult to compute with integer math. 
// We use a simpler approach. See sht10_Non-Linearity_Compensation_Humidity_Sensors_E.pdf
//
// output: humi [%RH] (=integer value from 0 to 100)
uint8_t rhcalc_int(unsigned int s)
{
	// s is in the range from 100 to 3340
	unsigned int rh;
	//for s less than 1712: (143*s - 8192)/4096
	//for s greater than 1712: (111*s + 46288)/4096
	//s range: 100<s<3350
        //
	//rh = rel humi * 10
	if (s<1712){
                // div by 4:
		rh=(36*s - 2048)/102;
	}else{
                // div by 8:
		rh=(14*s + 5790)/51;
	}
	// round up as we will cut the last digit to full percent
	rh+=5;
	rh/=10;
        //
	if (rh>98){
		rh=100;
	}
	return((uint8_t)(rh));
}

// calculates humidity [%RH] with temperature compensation
// input : humi [Ticks] (12 bit), temperature in 'C * 100 (e.g 253 for 25.3'C)
// output: humi [%RH] (=integer value from 0 to 100)
uint8_t calc_sth11_humi(unsigned int h, int t)
{
        int rh;
        rh=rhcalc_int(h);
        // now calc. Temperature compensated humidity [%RH]
        // the correct formula is:
        // rh_true=(t/10-25)*(0.01+0.00008*(sensor_val))+rh; 
        // sensor_val ~= rh*30 
        // we use:
        // rh_true=(t/10-25) * 1/8;
        rh=rh + (t/80 - 3);
        return((uint8_t)rh);
}

// this is an approximation of 100*log10(x) and does not need the math
// library. The error is less than 5% in most cases.
// compared to the real log10 function for 2<x<100.
// input: x=2<x<100 
// output: 100*log10(x) 
// Idea by Guido Socher
int log10_approx(uint8_t x)
{
	int l,log;
	if (x==1){
		return(0);
	}
	if (x<8){
		return(11*x+11);
	}
	//
	log=980-980/x;
	log/=10;
	if (x<31){
		l=19*x;
		l=l/10;
		log+=l-4;
	}else{
		l=67*x;
		l=l/100;
		if (x>51 && x<81){
			log+=l +42;
		}else{
			log+=l +39;
		}
	}
	if (log>200) log=200;
	return(log);
}

// calculates dew point
// input: humidity [in %RH], temperature [in C times 10]
// output: dew point [in C times 10]
int calc_dewpoint(uint8_t rh,int t)
{ 
        // we use integer math and everything times 100
        int k,tmp;
        k = (100*log10_approx(rh)-20000)/43;
	// we do the calculations in many steps otherwise the compiler will try
	// to optimize and that creates nonsence as the numbers
	// can get too big or too small.
        tmp=t/10;
        tmp=881*tmp;
        tmp=tmp/(243+t/10);
        k+=tmp*2;
        tmp=1762-k;
        tmp=24310/tmp;
        tmp*=k;
        // dew point temp rounded:
	if (tmp<0){
		tmp-=51;
	}else{
		tmp+=51;
	}
        return (tmp/10);
}

// --- end of sensirion_protocol.c

