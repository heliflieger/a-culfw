#!/usr/bin/perl

# Convert time from timer.c into readable version

my @a=localtime($ARGV[0]/1000);
printf("%04d.%02d.%02d %02d:%02d:%02d\n", $a[5]+1900, $a[4]+1, $a[3], $a[2],$a[1],$a[0]);

