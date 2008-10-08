#!/usr/bin/perl

use strict;
use warnings;

my (%h, $n, %r);

if(int(@ARGV) != 2) {
  print "Usage: xmllist2curmenu.pl xmllist template > menu\n";
  exit(1);
}

##################
# xml file first
open(FH, $ARGV[0]) || die("$ARGV[0]: $!\n");
while(my $l = <FH>) {
  if($l =~ m+^\t\t<FS20 name="([^"]*)".*sets="([^"]*)"+) {
    $n=$1;
    $h{$n}{sets} = $2;
  }
  next if(!$n);
  $h{$n}{def}  = $1 if($l =~ m+^\t\t\t<INT key="DEF" value="([^"]*)"+);
  $h{$n}{room} = $1 if($l =~ m+^\t\t\t<ATTR key="room" value="([^"]*)"+);
  undef($n) if($l =~ m+^\t\t</FS20+);
}
close(FH);

foreach my $k (keys %h) {
  $h{$k}{room} = "UNDEF" if(!$h{$k}{room});
  $r{$h{$k}{room}}{$k} = 1;
}

##################
# Template file, first run
open(FH, $ARGV[1]) || die("$ARGV[1]: $!\n");
my %defs;
$n = 0;
while(my $l = <FH>) {
  chomp($l);
  next if($l eq "");
  my $c = substr($l,1,1);
  my @a = split($c, $l);

  $defs{$a[1]} = sprintf("%02x", $n++) if($a[0] eq "M");
  $n += int(keys %r)  if($l =~ m/^<xmllist>/);
}
close(FH);


##################
# Template file, second run
open(FH, $ARGV[1]) || die("$ARGV[1]: $!\n");
printf "#MENU %02x\n\n", $n;
$n = 0;
while(my $l = <FH>) {
  chomp($l);
  if($l eq "") {
    print "\n";
    next;
  }
  my $c = substr($l,1,1);
  my @a = split($c, $l);

  if($a[0] eq "M") {

    printf("M$c$a[1]$c%02d\n", $n++);

  } elsif($a[0] eq "S") {

    shift(@a);
    my $t = shift(@a);
    my $m = shift(@a);
    printf("S$c$t$c%s%s\n", $defs{$m}, join($c, @a));

  } elsif($l =~ m/^<xmllist>/) {
    my $on = $n;
    foreach my $rn (sort keys %r) {
      printf "S $rn %02x\n", $on++;
    }

    foreach my $rn (sort keys %r) {
      printf "\nM $rn %02x\n", $n++;
      foreach my $k (sort keys %{$r{$rn}}) {
        my $f = ($h{$k}{sets} =~ m/dim/) ?
                $defs{"FS20-Dim"} : $defs{"FS20-Plain"};
        my $d = $h{$k}{def};
        $d =~ s/ //g;
        print "S $k $f $d\n";
      }
    }


  } else {

    printf("$l\n");

  }

}
