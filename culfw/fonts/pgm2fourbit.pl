#!/usr/bin/perl

use strict;
use warnings;

if(@ARGV != 2 || !($ARGV[0] eq "raw" || $ARGV[0] eq "hex") ) {
  die("Usage: pgm2fourbit.pl [raw|hex] file.ppm/file.pgm > bin\n");
}
my $raw = ($ARGV[0] eq "raw");
open(FH, $ARGV[1]) || die("$ARGV[1]: $!\n");

my $mode = <FH>;
chomp($mode);
if($mode !~ m/^P[56]/) {
  die("Need P5/P6 pgm file\n");
}


my ($w, $h, $depth);
while(my $l = <FH>) {
  chomp($l);
  next if($l =~ m/^#/);
  if(!defined($w)) {
    $l =~ m/^(\d+) (\d+)$/;
    $w = $1; $h = $2;
    next;
  }
  die("Unknown format: need 256 or 16 color values, got $l\n")
        if($l != 255 && $l != 15);
  $depth = $l;
  last;
}


my $shift = ($depth == 255 ? 4 : 0);

if($mode eq "P5") {

  my $buf = join("", <FH>);
  if(length($buf) != $w*$h) {
    die("Not enough data, need: " . ($w*$h) ."\n");
  }

  my ($d, $p, $cnt) = (0, 0, 0);

  foreach my $b (split("", $buf)) {

    if($p) {
      if($raw) {
        printf("%c", ($d<<4)|(ord($b)>>$shift));
      } else {
        printf("0x%02x, ", ($d<<4)|(ord($b)>>$shift));
        if(++$cnt == 20) {
          printf("\n");
          $cnt = 0;
        }
      }
      $p = 0;
    } else {
      $d = (ord($b)>>$shift);
      $p = 1;
    }
    
  }
  printf("\n") if(!$raw);

} else {

  my $buf = join("", <FH>);
  if(length($buf) != $w*$h*3) {
    die("Not enough data, need: " . ($w*$h*3) ."\n");
  }

  my ($p, $cnt) = (0, 0, 0);
  my @d;

  foreach my $b (split("", $buf)) {

    $d[$p++] = ord($b)>>$shift;
    if($p == 6) {
      if($raw) {
        printf("%c%c%c",
          ($d[0]<<4)|$d[1], ($d[2]<<4)|$d[3], ($d[4]<<4)|$d[5]);
      } else {
        printf("0x%02x, 0x%02x, 0x%02x, ",
          ($d[0]<<4)|$d[1], ($d[2]<<4)|$d[3], ($d[4]<<4)|$d[5]);
        if(++$cnt == 3) {
          printf("\n");
          $cnt = 0;
        }
      }
      $p = 0;
    }
    
  }
  printf("\n") if(!$raw);

}
