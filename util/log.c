// compile this with: gcc -Wall -lm -o log log.c
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


int main(){
	int i=1;
	printf("x;log(x);log10_approx;err\n");
	while(i<101){
		printf("%d;%6.1f;%d;%5.2f;\n",i,100*log10(i),log10_approx(i),100*log10(i)-log10_approx(i));
		i++;
	}
	return(0);
	
}
