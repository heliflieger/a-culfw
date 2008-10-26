#!/usr/bin/perl

use strict;
use warnings;

if(@ARGV != 1) {
  die("Usage: pgm2fourbit.pl file.ppm/file.pgm > bin\n");
}
open(FH, $ARGV[0]) || die("$ARGV[0]: $!\n");

my $mode = <FH>;
chomp($mode);
if($mode !~ m/^P[56]/) {
  die("Need P5/P6 pgm file\n");
}


my ($w, $h);
while(my $l = <FH>) {
  chomp($l);
  next if($l =~ m/^#/);
  if(!defined($w)) {
    $l =~ m/^(\d+) (\d+)$/;
    $w = $1; $h = $2;
    next;
  }
  if($l !~ m/^255$/) {
    die("Unknown format: need 255 color values, got $l\n");
  } else {
    last;
  }
}


if($mode eq "P5") {

  my $buf = join("", <FH>);
  if(length($buf) != $w*$h) {
    die("Not enough data, need: " . ($w*$h) ."\n");
  }

  my ($d, $p, $cnt) = (0, 0, 0);

  foreach my $b (split("", $buf)) {

    if($p) {
      printf("0x%02x, ", ($d<<4)|(ord($b)>>4));
      if(++$cnt == 470) {
        printf("\n");
        $cnt = 0;
      }
      $p = 0;
    } else {
      $d = (ord($b)>>4);
      $p = 1;
    }
    
  }
  printf("\n");

} else {

  my $buf = join("", <FH>);
  if(length($buf) != $w*$h*3) {
    die("Not enough data, need: " . ($w*$h*3) ."\n");
  }

  my ($p, $cnt) = (0, 0, 0);
  my @d;

  foreach my $b (split("", $buf)) {

    $d[$p++] = ord($b)>>4;
    if($p == 6) {
      printf("0x%02x, 0x%02x, 0x%02x, ",
        ($d[0]<<4)|$d[1], ($d[2]<<4)|$d[3], ($d[4]<<4)|$d[5]);
      if(++$cnt == 3) {
        printf("\n");
        $cnt = 0;
      }
      $p = 0;
    }
    
  }
  printf("\n");

}
