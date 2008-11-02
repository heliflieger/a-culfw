#!/usr/bin/perl

use strict;
use warnings;

my (%h, $n, %r);

sub chkprint($);

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
    $h{$n}{TYPE} = "FS20";
  }
  if($l =~ m+^\t\t<FHT name="([^"]*)"+) {
    $n=$1;
    $h{$n}{TYPE} = "FHT";
  }

  next if(!$n);
  $h{$n}{def}  = $1 if($l =~ m+^\t\t\t<INT key="DEF" value="([^"]*)"+);
  $h{$n}{room} = $1 if($l =~ m+^\t\t\t<ATTR key="room" value="([^"]*)"+);
  undef($n) if($l =~ m+^\t\t</FS20+);
  undef($n) if($l =~ m+^\t\t</FHT+);
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

  if($a[0] eq "M") {
    $defs{$a[1]} = sprintf("%02x", $n++);
  }
  $n += int(keys %r)  if($l =~ m/^<xmllist>/);
}
close(FH);

printf STDERR "WARNING: Too many menu definitions, 32 allowed\n" if($n > 32);


##################
# Template file, second run
open(FH, $ARGV[1]) || die("$ARGV[1]: $!\n");
my $line = 0;
my $nitems = 0;
$n = 0;
while(my $l = <FH>) {
  $line++;
  chomp($l);
  if($l eq "") {
    print "\n";
    next;
  }
  my $c = substr($l,1,1);
  my @a = split($c, $l);

  if($a[0] eq "M") {

    $nitems = 0;
    chkprint(sprintf("M$c$a[1]$c%s", ($a[2] ? $a[2] : "")));
    $n++;

  } elsif($a[0] eq "S") {

    shift(@a);
    my $t = shift(@a);
    my $m = shift(@a);
    die("Unknown menu $m referenced in line $line ($l)\n") if(!$defs{$m});
    chkprint(sprintf("S$c$t$c%s$c%s", $defs{$m}, join($c, @a)));

  } elsif($l =~ m/^<xmllist>/) {
    my $on = $n;

    foreach my $rn (sort keys %r) {
      chkprint(sprintf "S $rn %02x", $on++);
    }
    chkprint "";

    foreach my $rn (sort keys %r) {
      chkprint "M $rn";
      $n++;
      foreach my $k (sort keys %{$r{$rn}}) {
        my $f;
        if($h{$k}{TYPE} eq "FS20") {
          $f = ($h{$k}{sets} =~ m/dim/) ?
                $defs{"FS20-Dim"} : $defs{"FS20-Plain"};
        } elsif($h{$k}{TYPE} eq "FHT") {
          $f = $defs{"FHT"};
        }
        my $d = $h{$k}{def};
        $d =~ s/ //g;
        chkprint("S $k $f $d");
      }
      chkprint "";
    }


  } else {

    chkprint("$l");

  }

  $nitems++;
  printf STDERR "WARNING: Too many subitems in line $line, 32 allowed\n"
               if($nitems > 32);
}

sub
chkprint($)
{
  my $str = shift;
  if(length($str) > 32) {
    print STDERR "Expression $str is too long\n";
  }
  print "$str\n";
}
