#!/usr/bin/perl -w
# vim: set sw=4 ts=4 si et:
# Copyright: Guido Socher
#
use strict;
use vars qw($opt_h);
use Getopt::Std;
sub help();
#
getopts("h")||die "ERROR: No such option. -h for help\n";
help() if ($opt_h);
help() unless($ARGV[1]);

my $t=$ARGV[0] *0.01 - 39.7;
printf("temp in 'C=%6.2f\n",$t);
my $rh_lin=$ARGV[1] * $ARGV[1] * -0.0000028 + 0.0405 * $ARGV[1] -4;
printf("humi %% lin=%6.2f\n",$rh_lin);
my $rh_true=($t-25)*(0.01 + 0.00008 * $ARGV[1])+$rh_lin;
printf("humi %%    =%6.2f\n",$rh_true);
my $k= (log($rh_true)/log(10)-2)/0.4343 + (17.62*$t)/(243.12+$t);
printf("k         =%6.2f\n",$k);
my $dp=243.12*$k/(17.62-$k);
printf("dew point =%6.2f\n",$dp);
print "\n";
#
sub help(){
print "raw2val.pl -- convert raw sht11 sensor values to real numbers
hmidity is 12 bit temperature is 14 bit sensor accuracy

USAGE: raw2val.pl raw_temp raw_humi

OPTIONS: -h this help

You can uncomment the define for SHOW_RAW in main.c to see the raw
values.
\n";
exit;
}
__END__ 

