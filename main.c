/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you add "set modeline" in your .vimrc
 * Author: Guido Socher
 * Copyright: GPL V2
 * See http://www.gnu.org/licenses/gpl.html
 *
 * Ethernet interface to a combined SHT11 and ds18s20 sensor station
 * 
 * See http://tuxgraphics.org/electronics/
 *
 * Chip type           : Atmega168 or Atmega328 with ENC28J60
 *********************************************/
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include "ip_arp_udp_tcp.h"
#include "enc28j60.h"
#include "timeout.h"
#include "net.h"
#include "websrv_help_functions.h"
#include "sensirion_protocol.h"
#include "onewire.h"
#include "ds18x20.h"

// set output to VCC, red LED off
#define LEDOFF PORTB|=(1<<PORTB1)
// set output to GND, red LED on
#define LEDON PORTB&=~(1<<PORTB1)
// to test the state of the LED
#define LEDISOFF PORTB&(1<<PORTB1)

// please modify the following lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices. The IP address may be changed at run-time but the
// MAC address is hardcoded:

static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x29};
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX
static uint8_t myip[4] = {192,168,178,120};

// listen port for tcp/www:
static uint16_t mywwwport=80;
// the password string (only characters: a-z,0-9,_,.,-,# ), max len 8 char:
static char password[9]="vEUA4GoP";
// -------------- do not change anything below this line ----------
// The buffer is the packet size we can handle and its upper limit is
// given by the amount of RAM that the atmega chip has. 
#define BUFFER_SIZE 640
static uint8_t buf[BUFFER_SIZE+1];

// global string buffer
#define STR_BUFFER_SIZE 20
static char gStrbuf[STR_BUFFER_SIZE+1];
static uint16_t gPlen;

// --- sht11 sensors
// uncomment to see raw sensor data:
#define SHOW_RAW 1
// this software does only support one sensor (due to memory 
// restrictions we can not record more data):
#define MAX_SENSORS 1
unsigned int gHumival_raw[MAX_SENSORS],gTempval_raw[MAX_SENSORS];
uint8_t gSensorErrors[MAX_SENSORS];
uint8_t gAllSensorsReadOnce=0;

// --- onewire sensors
#define MAXOWSENSORS 2
static uint8_t gOWSensorIDs[MAXOWSENSORS][OW_ROMCODE_SIZE];
static int16_t gOWTempdata[MAXOWSENSORS]; // temperature times 10
static int8_t gOWsensors=0;
static uint8_t gOWTemp_measurementstatus=0; // 0=ok,1=erro

// set this to 1 to have the graph recorded in farenheit
static uint8_t graph_in_f=0;
//
// how often (in minutes times 10, 24=240min) to record the data to a graph (max value=255 ):
static uint8_t rec_interval=24;
static uint8_t gTemphistnextptr=0;
//
static volatile uint8_t cnt2step=0;
static volatile uint8_t gSec=0;
static volatile uint8_t gMeasurementTimer=0; // sec
static uint16_t gRecMin=0; // miniutes counter for recording of history graphics
//

//RCL Turtle control
int limitTempTop       = 110;
int limitTempBottom    = 100;
int switchStateTarget = 0;


// how many values to store (watch out for eeprom overflow):
#define HIST_BUFFER_SIZE 72
// position at which the EEPROM is used for storage of temp. data:
#define EEPROM_TEMP_POS_OFFSET 40 
// sensor=0..2
int8_t read_dat(uint8_t pos,uint8_t sensor)
{
        return ((int8_t)eeprom_read_byte((uint8_t *)((uint16_t)pos+EEPROM_TEMP_POS_OFFSET+(HIST_BUFFER_SIZE*(uint16_t)sensor))));
}

// no overflow protection here make sure you stay within the available eeprom size:
void store_dat(int8_t val, uint8_t pos,uint8_t sensor)
{
        eeprom_write_byte((uint8_t *)((uint16_t)pos+EEPROM_TEMP_POS_OFFSET+(HIST_BUFFER_SIZE*(uint16_t)sensor)),(uint8_t)val); 
}

void init_store_dat(void)
{
        uint8_t i=0;
        while(i<HIST_BUFFER_SIZE){
                store_dat(0,i,0);
                store_dat(0,i,1);
                store_dat(0,i,2);
                i++;
        }
}
// 
uint8_t verify_password(char *str)
{
        // the first characters of the received string are
        // a simple password/cookie:
        if (strncmp(password,str,strlen(password))==0){
                return(1);
        }
        return(0);
}

// insert a '.' before the last digit/character.
// "100" will become "10.0"
// This function writes to gStrbuf
// The user is responsible for allocating enough space.
void adddecimalpoint2(int16_t num){
        uint8_t i;
        itoa(num/10,gStrbuf,10); 
        i=strlen(gStrbuf);
        gStrbuf[i]='.';
        itoa(abs(num)%10,&(gStrbuf[i+1]),10); 
}

uint16_t http200ok(void)
{
        return(fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}

uint16_t http200okjs(void)
{
        return(fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: application/x-javascript\r\n\r\n")));
}

uint16_t fill_tcp_data_int(uint8_t *buf,uint16_t plen,int16_t i)
{
        itoa(i,gStrbuf,10); // convert integer to string
        return(fill_tcp_data(buf,plen,gStrbuf));
}

// convert celsius times 10 values to Farenheit
int8_t c2f(int16_t celsiustimes10){
        return((int8_t)((int16_t)(celsiustimes10 * 18)/100) + 32);
}

uint16_t print_webpage_config(void)
{
        uint16_t plen;
        plen=http200ok();
        plen=fill_tcp_data_p(buf,plen,PSTR("<a href=./>[home]</a>\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("<h2>Graph recording config</h2>\n<pre>"));
        plen=fill_tcp_data_p(buf,plen,PSTR("\n<form action=/ method=get>"));
        plen=fill_tcp_data_p(buf,plen,PSTR("interval:<input type=text name=ri size=3 value="));
        plen=fill_tcp_data_int(buf,plen,rec_interval);
        
        plen=fill_tcp_data_p(buf,plen,PSTR(">[*10 min, 6=1h]\npassw.  :<input type=password size=10 name=pw>\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("<input type=submit value=\"change\"></form>\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("</pre><hr>\n"));
        return(plen);
}

void print_webpage_relay(uint8_t on)
{
        uint16_t pl; // a local var results in less code than access to global
        pl=http200ok();
        pl=fill_tcp_data_p(buf,pl,PSTR("<a href=./>[home]</a> <a href=./?sw=1>[refresh]</a>\n"));
        pl=fill_tcp_data_p(buf,pl,PSTR("<h2>Switch</h2>\n<pre>state: "));
        if (on){
                pl=fill_tcp_data_p(buf,pl,PSTR("<font color=#00FF00>ON</font>"));
        }else{
                pl=fill_tcp_data_p(buf,pl,PSTR("OFF"));
        }
        pl=fill_tcp_data_p(buf,pl,PSTR("\n<form action=/ method=get>"));
        pl=fill_tcp_data_p(buf,pl,PSTR("<input type=hidden name=sc value="));
        if (on){
                pl=fill_tcp_data_p(buf,pl,PSTR("0>\npassw: <input type=password size=10 name=pw>"));
                pl=fill_tcp_data_p(buf,pl,PSTR("<input type=submit value=\"switch off\"></form>\n"));
        }else{
                pl=fill_tcp_data_p(buf,pl,PSTR("1>\npassw: <input type=password size=10 name=pw>"));
                pl=fill_tcp_data_p(buf,pl,PSTR("<input type=submit value=\"switch on\"></form>\n"));
        }
        pl=fill_tcp_data_p(buf,pl,PSTR("</pre><hr>\n"));
        gPlen=pl;
}

// p1.js
uint16_t print_p1_js(void)
{
        // I am sorry that the javascript is a bit unreadable 
        // but we absolutely need to keep the amount of code
        // in bytes small
        uint16_t plen;
        plen=http200okjs();
        // g = graphdata array
        // m = minutes since last data, global variable
        // d = recording interval in min, global variable
        // t = new Date(), global variable
        plen=fill_tcp_data_p(buf,plen,PSTR("\
function pad(n){\
var s=n.toString(10);\
if (s.length<2){return(\"0\"+s)}\
return(s);\
}\n\
function dw(s){document.writeln(s)}\n\
function scvs(g1,g2,g3){\
document.open();\
dw(\"<pre>#Copy/paste as txt file:\");\
dw(\"#date;i_temp;i_hum;o_temp;\");\
var i,ms,pt;\
ms=t.getTime();\
var l=new Date();\
for(i=0;i<g1.length;i++){\
pt=ms-((m+d*i)*60*1000);\
l.setTime(pt);\
dw(l.getFullYear()+\"-\"+pad(l.getMonth())+\"-\"+pad(l.getDate())+\" \"+pad(l.getHours())+\":\"+pad(l.getMinutes())+\";\"+g1[i]+\";\"+g2[i]+\";\"+g3[i]+\";\");\
}\
document.close();\
}\n\
"));
        return(plen);
}

// p2.js
uint16_t print_p2_js(void)
{
        // I am sorry that the javascript is a bit unreadable 
        // but we absolutely need to keep the amount of code
        // in bytes small
        uint16_t plen;
        plen=http200okjs();
        // gd = graphdata array
        // c = color
        // a = amplification factor
        // h = heading
        // m = minutes since last data, global variable
        // d = recording interval in min, global variable
        // t = new Date(), global variable
        //
        // pbar(w,c,t) w=width c=color t=time
        plen=fill_tcp_data_p(buf,plen,PSTR("\
function pbar(w,c,t){\
if(w<5){w=5;}\
dw(\"<p style=\\\"width:\"+w+\"px;background-color:#\"+c+\";margin:2px;border:1px #999 solid;white-space:pre;\\\">\"+t+\"</p>\");\
}\n\
function bpt(gd,c,a,h){\
dw(\"<h2>\"+h+\"</h2>\");\
var i,n,ms,pt;\
ms=t.getTime();\
var l=new Date();\
for(i=0;i<gd.length;i++){\
 pt=ms-((m+d*i)*60*1000);\
 l.setTime(pt);\
 n=parseInt(gd[i]);\
 pbar((a*n+120),c,pad(l.getDate())+\"-\"+pad(l.getHours())+\":\"+pad(l.getMinutes())+\"=\"+n);\
}\n\
dw(\"<br>TZ diff GMT: \"+t.getTimezoneOffset()/60+\"h. Format: day-hh:mm=val<br>\");\
}\n\
"));
        return(plen);
}

// sensor = 0 or 1
uint16_t print_webpage_graph_array(uint8_t sensor)
{
        int8_t i;
        uint8_t nptr;
        uint16_t plen;
        plen=http200okjs();
        nptr=gTemphistnextptr; // gTemphistnextptr may change so we make a snapshot here
        i=nptr;
        plen=fill_tcp_data_p(buf,plen,PSTR("gdat"));
        plen=fill_tcp_data_int(buf,plen,sensor);
        plen=fill_tcp_data_p(buf,plen,PSTR("=Array("));
        if (!eeprom_is_ready()){
                // don't sit and wait 
                plen=fill_tcp_data_p(buf,plen,PSTR("0);\n"));
                return(plen);
        }
        while(1){
                if (i!=nptr){ // not first time
                        plen=fill_tcp_data_p(buf,plen,PSTR(","));
                }
                i--;
                if (i<0){
                        i=HIST_BUFFER_SIZE-1;
                }
                // sensor can be 0..2
                if (sensor<3){ 
                        // in temp=0, humidity=1, out temp=2
                        plen=fill_tcp_data_int(buf,plen,read_dat(i,sensor));
                }
                if (i==nptr){
                        break; // end loop
                }
        }
        plen=fill_tcp_data_p(buf,plen,PSTR(");\n"));
        return(plen);
}

uint16_t print_webpage_graph(void)
{
        uint16_t plen;
        plen=http200ok();
        // global javascript variables:
        plen=fill_tcp_data_p(buf,plen,PSTR("<script>var t=new Date();var m="));
        plen=fill_tcp_data_int(buf,plen,gRecMin);
        plen=fill_tcp_data_p(buf,plen,PSTR(";var d="));
        plen=fill_tcp_data_int(buf,plen,rec_interval);
        plen=fill_tcp_data_p(buf,plen,PSTR("0;</script>\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("<script src=p1.js></script><script src=p2.js></script>\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("<script src=gdat0.js></script>"));
        plen=fill_tcp_data_p(buf,plen,PSTR("<script src=gdat1.js></script>"));
        plen=fill_tcp_data_p(buf,plen,PSTR("<script src=gdat2.js></script>\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("<a href=./>[home]</a> "));
        plen=fill_tcp_data_p(buf,plen,PSTR("<a href=javascript:scvs(gdat0,gdat1,gdat2)>[CSV data]</a>\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("<script>\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("dw(\"<table border=1><tr><td>\");\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("bpt(gdat0,\"eb0\",2,\"In Temp\");\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("dw(\"</td><td>\");\n"));
        // orange: eb0 green: ac2 blue: 78c orange2: f72
        plen=fill_tcp_data_p(buf,plen,PSTR("bpt(gdat1,\"78c\",1.5,\"In Humidity %\");\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("dw(\"</td><td>\");\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("bpt(gdat2,\"f72\",2,\"Out Temp\");\n"));
        plen=fill_tcp_data_p(buf,plen,PSTR("dw(\"</td></tr></table>\");\n</script>\n<br><hr>\n"));
        return(plen);
}

// this the top level main web page
uint16_t print_webpage_sensordetails(void)
{
        int8_t i,rh;
        int dew,temp;
        uint8_t error;
        uint16_t plen;
        plen=http200ok();
        plen=fill_tcp_data_p(buf,plen,PSTR("<a href=./?sw=1>[switch]</a> <a href=./?c=1>[cfg]</a> <a href=./?g=0>[graph]</a> "));
        plen=fill_tcp_data_p(buf,plen,PSTR("<a href=./>[refresh]</a>\n<h2>Current conditions</h2>"));
        plen=fill_tcp_data_p(buf,plen,PSTR("<pre>"));
        if (gAllSensorsReadOnce==0){
                plen=fill_tcp_data_p(buf,plen,PSTR("wait...\n"));
                goto END_OF_WEBPAGE;
        }
        // sht11:
        plen=fill_tcp_data_p(buf,plen,PSTR("<b>Indoor SHT11 Sensor</b>\n"));
                //
        error=gSensorErrors[0];
        if(error!=0){ //in case of an error
                if (error==1){
                        plen=fill_tcp_data_p(buf,plen,PSTR("no sensor\n"));
                }
                if (error==2){
                        plen=fill_tcp_data_p(buf,plen,PSTR("sensor crc error\n"));
                }
                if (error==3){
                        plen=fill_tcp_data_p(buf,plen,PSTR("timeout error\n"));
                }
                plen=fill_tcp_data_p(buf,plen,PSTR("\n"));
        }else{
#ifdef SHOW_RAW
                plen=fill_tcp_data_p(buf,plen,PSTR("DEBUG: raw temp humi:"));
                itoa(gTempval_raw[0],gStrbuf,10);
                plen=fill_tcp_data(buf,plen,gStrbuf);
                plen=fill_tcp_data_p(buf,plen,PSTR(" "));
                itoa(gHumival_raw[0],gStrbuf,10);
                plen=fill_tcp_data(buf,plen,gStrbuf);
                plen=fill_tcp_data_p(buf,plen,PSTR("\n"));
#endif
                //
                plen=fill_tcp_data_p(buf,plen,PSTR("temp.    : "));
                temp=calc_sth11_temp(gTempval_raw[0]);
                adddecimalpoint2(temp);
                plen=fill_tcp_data(buf,plen,gStrbuf);
                plen=fill_tcp_data_p(buf,plen,PSTR("'C ["));
                itoa(c2f(temp),gStrbuf,10);
                plen=fill_tcp_data(buf,plen,gStrbuf);
                plen=fill_tcp_data_p(buf,plen,PSTR("'F]"));
                plen=fill_tcp_data_p(buf,plen,PSTR("\n"));
#ifdef SHOW_RAW
                plen=fill_tcp_data_p(buf,plen,PSTR("humi lin : "));
                rh=rhcalc_int(gHumival_raw[0]);
                itoa(rh,gStrbuf,10);
                plen=fill_tcp_data(buf,plen,gStrbuf);
                plen=fill_tcp_data_p(buf,plen,PSTR("\n"));
#endif
                plen=fill_tcp_data_p(buf,plen,PSTR("humidity : "));
                rh=calc_sth11_humi(gHumival_raw[0],temp);
                itoa(rh,gStrbuf,10);
                plen=fill_tcp_data(buf,plen,gStrbuf);
                plen=fill_tcp_data_p(buf,plen,PSTR("%\n"));
                plen=fill_tcp_data_p(buf,plen,PSTR("dew point: "));
                dew=calc_dewpoint(rh,temp);
                adddecimalpoint2(dew);
                plen=fill_tcp_data(buf,plen,gStrbuf);
                plen=fill_tcp_data_p(buf,plen,PSTR("'C ["));
                itoa(c2f(dew),gStrbuf,10);
                plen=fill_tcp_data(buf,plen,gStrbuf);
                plen=fill_tcp_data_p(buf,plen,PSTR("'F]\n\n"));
                //
        }
        plen=fill_tcp_data_p(buf,plen,PSTR("<b>DS18S20 Sensors</b>\n"));
        // onewire ds18x20 sensors:
        i=0;
        if (gOWsensors<1){
                plen=fill_tcp_data_p(buf,plen,PSTR("no sensor\n"));
        }

        while(i<gOWsensors) {
                plen=fill_tcp_data_p(buf,plen,PSTR("temp S"));
                plen=fill_tcp_data_int(buf,plen,i);
                plen=fill_tcp_data_p(buf,plen,PSTR("  : "));
                adddecimalpoint2(gOWTempdata[i]);
                plen=fill_tcp_data(buf,plen,gStrbuf);
                plen=fill_tcp_data_p(buf,plen,PSTR("'C ["));
                plen=fill_tcp_data_int(buf,plen,c2f(gOWTempdata[i]));
                plen=fill_tcp_data_p(buf,plen,PSTR("'F]\n"));
                i++;
        }
        if (gOWTemp_measurementstatus==1){
                plen=fill_tcp_data_p(buf,plen,PSTR("ds18x20 error\n"));
        }

        //RCL
        plen=fill_tcp_data_p(buf,plen,PSTR("SwitchState: "));
        plen=fill_tcp_data_int(buf,plen,switchStateTarget);
        plen=fill_tcp_data_p(buf,plen,PSTR("\n"));

        plen=fill_tcp_data_p(buf,plen,PSTR("limitTempTop: "));
        plen=fill_tcp_data_int(buf,plen,limitTempTop);
        plen=fill_tcp_data_p(buf,plen,PSTR("\n"));

        plen=fill_tcp_data_p(buf,plen,PSTR("limitTempBottom: "));
        plen=fill_tcp_data_int(buf,plen,limitTempBottom);
        plen=fill_tcp_data_p(buf,plen,PSTR("\n"));

        plen=fill_tcp_data_p(buf,plen,PSTR("S Raw: "));
        plen=fill_tcp_data_int(buf,plen,gOWTempdata[0]);
        plen=fill_tcp_data_p(buf,plen,PSTR(" : "));
        plen=fill_tcp_data_int(buf,plen,gOWTempdata[1]);
        plen=fill_tcp_data_p(buf,plen,PSTR("\n"));

        

END_OF_WEBPAGE:
        plen=fill_tcp_data_p(buf,plen,PSTR("\n</pre><hr>(c)robinclarke.net\n"));
        return(plen);
}


// takes a string of the form command/Number and analyse it (e.g "?sw=pd7&a=1 HTTP/1.1")
// The first char of the url ('/') is already removed.
int8_t analyse_get_url(char *str)
{
        uint8_t i;
        //uint8_t j;
        if (str[0]==' '){ // end of url, main page
                return(1); 
        }
        // --------
        if (strncmp("favicon.ico",str,11)==0){
                gPlen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 301 Moved Permanently\r\nLocation: "));
                gPlen=fill_tcp_data_p(buf,gPlen,PSTR("http://tuxgraphics.org/ico/therm.ico"));
                gPlen=fill_tcp_data_p(buf,gPlen,PSTR("\r\n\r\nContent-Type: text/html\r\n\r\n"));
                gPlen=fill_tcp_data_p(buf,gPlen,PSTR("<h1>301 Moved Permanently</h1>\n"));
                return(10);
        }
        // --------
        i=0; // switch on or off 
        if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"sc")){
                if (gStrbuf[0]=='1'){
                        i=1;
                }
                if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"pw")){
                        urldecode(gStrbuf);
                        if (verify_password(gStrbuf)){
                                if (i){
                                        PORTD|= (1<<PORTD7);// transistor on
                                }else{
                                        PORTD &= ~(1<<PORTD7);// transistor off
                                }
                                print_webpage_relay(PORTD&(1<<PORTD7));
                                return(10);
                        }
                }
                return(-1);
        }
        //RCL
        // Set the Bottom temperature
        if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"s_bot")){
            urldecode(gStrbuf);
            unsigned int newTemp = atoi( gStrbuf );
            if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"pw")){
                urldecode(gStrbuf);
                if (verify_password(gStrbuf)){
                    limitTempBottom = newTemp;
                    eeprom_write_byte((uint8_t *)14,26 );
                    eeprom_write_byte((uint8_t *)15,limitTempBottom );
                    return(1);
                }
            }
            return( -1 );
        }
        // Set the Top temperature
        if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"s_top")){
            urldecode(gStrbuf);
            unsigned int newTemp = atoi( gStrbuf );
            if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"pw")){
                urldecode(gStrbuf);
                if (verify_password(gStrbuf)){
                    limitTempTop = newTemp;
                    eeprom_write_byte((uint8_t *)16,26 );
                    eeprom_write_byte((uint8_t *)17,limitTempTop );
                    return(1);
                }
            }
            return( -1 );
        }
         //
        if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"sw")){
                i=0;
                // state of the transistor/relay
                if (PORTD&(1<<PORTD7)){
                        i=1;
                }
                print_webpage_relay(i);
                return(10);
        }
        if (strncmp("p1.js",str,5)==0){
                gPlen=print_p1_js();
                return(10);
        }
        if (strncmp("p2.js",str,5)==0){
                gPlen=print_p2_js();
                return(10);
        }
        if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"g")){
                gPlen=print_webpage_graph();
                return(10);
        }
        if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"c")){
                gPlen=print_webpage_config();
                return(10);
        }
        if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"ri")){
                gStrbuf[4]='\0';
                i=atoi(gStrbuf);
                if (i<3){
                        i=3;
                }
                if (find_key_val(str,gStrbuf,STR_BUFFER_SIZE,"pw")){
                        urldecode(gStrbuf);
                        if (verify_password(gStrbuf)){
                                init_store_dat();
                                gRecMin=1;
                                rec_interval=i;
                                gAllSensorsReadOnce=0;
                                gRecMin=0;
                                eeprom_write_byte((uint8_t *)18,26); // magic number
                                eeprom_write_byte((uint8_t *)19,rec_interval);
                                return(1);
                        }
                }
                return(-1);
        }
        // --------
        if (strncmp("gdat",str,4)==0){
                if (strncmp(".js",(str+5),3)==0){
                        if (*(str+4)=='0'){
                                gPlen=print_webpage_graph_array(0);
                                return(10);
                        }
                        if (*(str+4)=='1'){
                                gPlen=print_webpage_graph_array(1);
                                return(10);
                        }
                        if (*(str+4)=='2'){
                                gPlen=print_webpage_graph_array(2);
                                return(10);
                        }
                }
        }
        return(0);
}

// called when TCNT2==OCR2A
// that is in about 50Hz intervals
// This is used as a "clock" to store the data
ISR(TIMER2_COMPA_vect){
        cnt2step++;
        if (cnt2step>49){ 
                gSec++;
                gMeasurementTimer++;
                cnt2step=0;
        }
}

/* setup timer T2 as an interrupt generating time base.
* You must call once sei() in the main program */
void init_cnt2(void)
{
        cnt2step=0;
        PRR&=~(1<<PRTIM2); // write power reduction register to zero
        TIMSK2=(1<<OCIE2A); // compare match on OCR2A
        TCNT2=0;  // init counter
        OCR2A=244; // value to compare against
        TCCR2A=(1<<WGM21); // do not change any output pin, clear at compare match
        // divide clock by 1024: 12.5MHz/1024=12207.0313 Hz
        TCCR2B=(1<<CS22)|(1<<CS21)|(1<<CS20); // clock divider, start counter
        // OCR2A=244 is a division factor of 245
        //         // 12207.0313 / 245= 49.82461
}

// read the latest measurement off the scratchpad of the ds18x20 sensor
// and store it in gOWTempdata
void onewire_read_temp_meas(void){
        uint8_t i;
        uint8_t subzero, cel, cel_frac_bits;
        for ( i=0; i<gOWsensors; i++ ) {
        
                if ( DS18X20_read_meas( &gOWSensorIDs[i][0], &subzero,
                                &cel, &cel_frac_bits) == DS18X20_OK ) {
                        gOWTempdata[i]=cel*10;
                        gOWTempdata[i]+=DS18X20_frac_bits_decimal(cel_frac_bits);
                        if (subzero){
                                gOWTempdata[i]=-gOWTempdata[i];
                        }
                }else{
                        gOWTempdata[i]=0;
                }
        }
}

// start a measurement for all sensors on the bus:
void onewire_start_temp_meas(void){
        gOWTemp_measurementstatus=0;
        if ( DS18X20_start_meas(NULL) != DS18X20_OK) {
                gOWTemp_measurementstatus=1;
        }
}

// writes to global array gOWSensorIDs
int8_t onewire_search_sensors(void)
{
	uint8_t diff;
        uint8_t nSensors=0;
	for( diff = OW_SEARCH_FIRST; 
		diff != OW_LAST_DEVICE && nSensors < MAXOWSENSORS ; )
	{
		DS18X20_find_sensor( &diff, &(gOWSensorIDs[nSensors][0]) );
		
		if( diff == OW_PRESENCE_ERR ) {
			return(-1); //No Sensor found
		}
		if( diff == OW_DATA_ERR ) {
			return(-2); //Bus Error
		}
		nSensors++;
	}
	return nSensors;
}

// convert a value times 10 into value and round it
// Example: -206 becomes -21, 306 becomes 31, 103 becomes 10.
int8_t roundanddivide(int16_t val){
        if (val<0){
                return((val-5)/10);
        }
        return((val+5)/10);
}

// called every minute but data is only stored at rec_interval:
void store_graph_dat(void){
        static uint8_t gCompSecCnt=0; // how often we corrected the clock
        int16_t temp;
        if (gRecMin==0){
                if(gSensorErrors[0]!=0){ //in case of an error
                        temp=0;
                        store_dat(0,gTemphistnextptr,1);
                }else{
                        temp=calc_sth11_temp(gTempval_raw[0]);
                        // store humidity
                        store_dat(calc_sth11_humi(gHumival_raw[0],temp),gTemphistnextptr,1);
                }
                if (graph_in_f){
                        // round up to full 'F is already done in c2f:
                        store_dat(c2f(temp),gTemphistnextptr,0);
                        store_dat(c2f(gOWTempdata[0]),gTemphistnextptr,2);
                }else{
                        // round up to full 'C:
                        store_dat(roundanddivide(temp),gTemphistnextptr,0);
                        store_dat(roundanddivide(gOWTempdata[0]),gTemphistnextptr,2);
                }
                gTemphistnextptr++;
                gTemphistnextptr=gTemphistnextptr%HIST_BUFFER_SIZE;
        }
        // compensate after 17 intervals of 5 min
        if (gCompSecCnt>16){ // executed 1min after gCompSecCnt==17
                gSec+=1;
                gCompSecCnt=0;
        }
        // We calibrate/compensate the clock here a bit.
        // Our clock is a bit behind by 50/49.82461=1.00352
        // We need to compenstate by 3600*1.00352-3600sec=12.672sec every
        // hour or 1sec every minute.
        // The gSec is zero or max one when this function is called.
        if (gRecMin%5==0){
                gSec+=1;
                gCompSecCnt++;
        }
        // we have a remaining compensation of 16.12800sec per day
        // after the above. There are 288 5min units in 24h: 288/16.128=17.85
        // In the next minute we will fix that (see code at "compensate after 17 intervals")
        // - - - -
        gRecMin++;
        if (gRecMin==((uint16_t)rec_interval*10)){
                gRecMin=0;
        }
}

int main(void){
        uint16_t dat_p;
        int8_t i=0;
        uint8_t measurementstate=0;
        uint8_t sensor=0;
        // set the clock speed to "no pre-scaler" (8MHz with internal osc or 
        // full external speed)
        // set the clock prescaler. First write CLKPCE to enable setting of clock the
        // next four instructions.
        CLKPR=(1<<CLKPCE); // change enable
        CLKPR=0; // "no pre-scaler"
        _delay_loop_1(0); // 60us

        /*initialize enc28j60*/
        enc28j60Init(mymac);
        enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
        _delay_loop_1(0); // 60us
        //
        // time keeping
        init_cnt2();
        sei();
        // LED
        DDRB|= (1<<DDB1); // enable PB1, LED as output
        LEDOFF;

        // the transistor on PD7 (relay)
        DDRD|= (1<<DDD7);
        PORTD &= ~(1<<PORTD7);// transistor off
        /* Magjack leds configuration, see enc28j60 datasheet, page 11 */
        // LEDB=yellow LEDA=green
        //
        // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
        // enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
        enc28j60PhyWrite(PHLCON,0x476);

        //init the ethernet/ip layer:
        init_ip_arp_udp_tcp(mymac,myip,mywwwport);

        // initialize:
        init_store_dat();
        while(i<MAX_SENSORS){
                gSensorErrors[i]=1; // init as error
                i++;
        }
        gOWsensors=onewire_search_sensors();
        if (eeprom_read_byte((uint8_t *)18) == 26){
                rec_interval=(uint8_t)eeprom_read_byte((uint8_t *)19);
        }

        //RCL Read the top/bottom target tank temperatures from the eeprom
        if (eeprom_read_byte((uint8_t *)14) == 26){
            // magic byte in eeprom - if 14 is set to 26, we know 15 will have the limitTempBottom
            limitTempBottom=(int)eeprom_read_byte((uint8_t *)15);
        }
        if (eeprom_read_byte((uint8_t *)16) == 26){
            // magic byte in eeprom - if 16 is set to 26, we know 17 will have the limitTempTop
            limitTempBottom=(int)eeprom_read_byte((uint8_t *)17);
        }


        while(1){
                // get the next new packet:
                gPlen = enc28j60PacketReceive(BUFFER_SIZE, buf);
                dat_p=packetloop_icmp_tcp(buf,gPlen);

                // Need two sensors to test with.  Assuming that the fridge is the second sensor
                if( gOWsensors > 1 ) {
                    if(  gOWTempdata[1] > limitTempTop ){
                        // If higher than top, turn on
                        PORTD|= (1<<PORTD7);// transistor on
                        switchStateTarget = 1;
                    }else if(  gOWTempdata[1] < limitTempBottom ){
                        // if lower than bottom, turn off
                        PORTD &= ~(1<<PORTD7);// transistor off
                        switchStateTarget = 0;
                    }
                }
  
                /*dat_p will ne unequal to zero if there is a valid 
                 * packet (without crc error) */
                if(dat_p==0){
                        // no packets, do measurements
                        //
                        // we need the measurementstate otherwise we would
                        // execute the same thing serveral times in that second
                        if (measurementstate==0 && gMeasurementTimer==0){
                                s_connectionreset(sensor);
                                LEDON;
                                measurementstate++;
                        }
                        if (measurementstate==1 && gMeasurementTimer==1){
                                gSensorErrors[sensor]=s_measure(MEASURE_TEMP,sensor); //measure temperature
                                // for onewire DS18X20 sensors we need just
                                // under 1sec between starting and reading
                                // of measurements:
                                onewire_start_temp_meas();
                                measurementstate++;
                                LEDOFF;
                                if (gSensorErrors[sensor]!=0){
                                        measurementstate=5;
                                }
                        }
                        if (measurementstate==2) {
                                // now poll sensor until it is ready
                                // might take up to 220ms
                                if (s_resultready(sensor)){
                                        gSensorErrors[sensor]=s_readmeasurement( &gTempval_raw[sensor],sensor); 
                                        measurementstate++;
                                        if (gSensorErrors[sensor]!=0){
                                                measurementstate=5;
                                        }
                                }
                        }
                        if (gMeasurementTimer==2){
                                onewire_read_temp_meas(); 
                                if (measurementstate==2){
                                        gSensorErrors[sensor]=3; // timeout
                                        measurementstate=5;
                                }
                                if (measurementstate==3){

                                        gSensorErrors[sensor]=s_measure(MEASURE_HUMI,sensor); 
                                        measurementstate++;
                                        if (gSensorErrors[sensor]!=0){
                                                measurementstate=5;
                                        }
                                }
                        }
                        if (measurementstate==4) {
                                // now poll sensor until it is ready
                                // might take up to 220ms
                                if (s_resultready(sensor)){
                                        gSensorErrors[sensor]=s_readmeasurement( &gHumival_raw[sensor],sensor);
                                        measurementstate++;
                                }
                        }
                        // wrap at 3:
                        if (gMeasurementTimer>2){
                                gMeasurementTimer=0;
                                measurementstate=0;
                                sensor++;
                                // start over
                                if (sensor>=MAX_SENSORS){
                                        sensor=0;
                                        if (gAllSensorsReadOnce==0){
                                                // store initial reading
                                                store_graph_dat();
                                                gAllSensorsReadOnce=1;
                                        }
                                }
                        }
                        if (gSec>59){
                                gSec=0;
                                store_graph_dat();
                        }
                        //
                        continue;
                }
                // Cut the size for security reasons. If we are almost at the 
                // end of the buffer then there is a zero but normally there is
                // a lot of room and we can cut down the processing time as 
                // correct URLs should be short in our case. If dat_p is already
                // close to the end then the buffer is terminated already.
                if ((dat_p+5+45) < BUFFER_SIZE){
                        buf[dat_p+5+45]='\0';
                }
                // analyse the url and do possible port changes:
                // move one char ahead:
                i=analyse_get_url((char *)&(buf[dat_p+5]));
                // for possible status codes see:
                // http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
                if (i==-1){
                        gPlen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1><a href=/>continue</a>\n"));
                        goto SENDTCP;
                }
                if (i==0){
                        gPlen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 Not Found</h1>"));
                        goto SENDTCP;
                }
                if (i==10){
                        goto SENDTCP;
                }
                // just display the status:
                // normally i==1
                gPlen=print_webpage_sensordetails();
                //
SENDTCP:
                www_server_reply(buf,gPlen); // send web page data
                // tcp port www end
                //
        }
        return (0);
}
