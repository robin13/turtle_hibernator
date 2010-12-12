// gcc -Wall -o dewpointcalc -lm dewpointcalc.c
#include <stdio.h>
#include <math.h>

// this is an approximation of 100*log10(x) and does not need the math
// library. The error is less than 5% in most cases.
// compared to the real log10 function for 2<x<100.
// input: x=2<x<100 
// output: 100*log10(x) 
// Idea by Guido Socher
int log10_approx(unsigned char x)
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

// rh=10% t=25
// k=-1/0.4343 + 440.5 / 268.12 =-2.30255 + 1.64292 =-0.65963
// dp=-160.37/18.27963=8.77

// calculates dew point
// input: humidity [in %RH], temperature [in C times 10]
// output: dew point [in C ]
int calc_dewpoint(unsigned char rh,int t)
{ 
        // we use integer math and everything times 100
        int k,tmp;
        k = (100*log10_approx(rh)-20000)/43;
	// do calculations in many steps otherwise the compiler will try
	// to optimize and that creates nonsence as the numbers
	// can get too big.
	tmp=t/10;
	tmp=881*tmp;
	tmp=tmp/(243+t/10);
	//printf("tmp: %d\n",tmp);
	k+=tmp*2;
	//printf("k: %d\n",k);
	tmp=1762-k;
	tmp=24310/tmp;
	tmp*=k;
        // dew point temp rounded (there is only one digit accuracy here
	// due to the fact that the log10_approx is not more accurate):
	if (tmp<0){
		tmp-=51;
	}else{
		tmp+=51;
	}
        return (tmp/100);
}
float calc_dewpoint_exact(unsigned char rh,int t)
{
	float k;
	t=t/10;
	k= (log10((float)rh)-2)/0.4343 + (17.62*(float)t)/(243.12+(float)t);
	return((243.12*k/(17.62-k)));
}

int main()
{
	unsigned char rh;
	int t;
	rh=10;
	t=-50;
	while(t<500){
		printf("\ntemp=%d\n",t);
		printf("rh,temp,log10,log10_approx,dew_approx,dew,error\n");
		while(rh<99){
			printf("%d,%d,%6.2f,%d,%d,%6.2f,%6.2f\n",rh,t,log10(rh),log10_approx(rh),calc_dewpoint(rh,t),calc_dewpoint_exact(rh,t),calc_dewpoint_exact(rh,t)-calc_dewpoint(rh,t));
			rh+=10;
		}
		t+=50;
		rh=10;
	}
	return(0);
}


