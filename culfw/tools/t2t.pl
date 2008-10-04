#!/usr/bin/perl

# Convert time from timer.c into readable version

my @a=localtime($ARGV[0]);
printf("%02d:%02d:%02d\n", $a[2],$a[1],$a[0]);

