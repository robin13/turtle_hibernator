// gcc -Wall -o humicalc humicalc.c
#include <stdio.h>

float rhcalc(float s)
{
	float rh;
	rh=-0.0000028*s*s + 0.0405*s - 4.0;
	return(rh);
}

// calculates humidity [%RH]
// The relative humitidy is: -0.0000028*s*s + 0.0405*s - 4.0
// but this is very difficult to compute with integer math. 
// We use a simpler approach. See sht10_Non-Linearity_Compensation_Humidity_Sensors_E.pdf
//
// output: humi [%RH] (=integer value from 0 to 100)
unsigned char rhcalc_int(unsigned int s)
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
	return((unsigned char)rh);
}

int main()
{
	float sensorval=100;
	float rh;
	printf("raw,rel_humi,int,error,\n");
	while(sensorval<3345){
		rh=rhcalc(sensorval);
		printf("%5.1f,%5.1f,%d,%5.2f\n",sensorval,rh,rhcalc_int(sensorval),rh-rhcalc_int(sensorval));
		sensorval+=5;
	}
	return(0);
}
