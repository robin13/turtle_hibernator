/*********************************************
 * vim:sw=8:ts=8:si:et
 * This is the header file the sensirion temperature and
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
//@{
#ifndef SENSIRION_PROTOCOL_H
#define SENSIRION_PROTOCOL_H

// mode can be one of:
#define MEASURE_TEMP 0x03 //000 0001 1
#define MEASURE_HUMI 0x05 //000 0010 1
//
// note: hardware connection definitions are in sensirion_protocol.c at
// the beginning of the inensirion_protocol.c file. You need to change
// the coded there if you connect the sensor to other pins.
// The MAX_SENSORS in main.c has to match the hardware connections
//
extern void s_connectionreset(uint8_t sensor);
extern char s_softreset(uint8_t sensor);
// start a measurement
extern char s_measure(uint8_t mode, uint8_t sensor);
// When the measurement is read then read it.
// Do not change sensor number, you can only start and read one
// sensor measurement at a time:
extern char s_readmeasurement(unsigned int *p_value, uint8_t sensor);
// return 1 if previously started measurement is ready
//
// Note that you can not wait too long. It takes between
// 60ms to 220ms for the measurement to complete and the
// sht11 pulls unfortunately the line only to gnd for a
// moment.  You should pull this function more or less
// constantly after the start of the measurement.
extern uint8_t s_resultready(uint8_t sensor);

extern int calc_sth11_temp(unsigned int t);
extern uint8_t rhcalc_int(unsigned int s);
extern uint8_t calc_sth11_humi(unsigned int h, int t);
extern int calc_dewpoint(uint8_t rh,int t);
extern int log10_approx(uint8_t x);

#endif /* SENSIRION_PROTOCOL_H */
//@}
