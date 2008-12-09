#!/usr/bin/perl

use strict;
use warnings;

my (%h, $n, %r);

sub chkprint($);

if(int(@ARGV) != 2) {
  print "Usage: xmllist2curmenu.pl fhem.xmllist CUR.menu.template > MENU\n";
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

  if($a[0] eq "M" || $a[0] eq "D") {
    $defs{$a[1]} = sprintf("%02x", $n++);
  }
  $n += int(keys %r)  if($l =~ m/^<xmllist>/);
}
close(FH);

printf STDERR "WARNING: Too many menu/macro definitions, 64 allowed\n"
        if($n > 64);


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
    chkprint(sprintf("M$c$a[1]$c%s$c%s",
        ($a[2] ? $a[2] : ""),
        ($a[3] ? $a[3] : "fffffffff")));
    $n++;

  } elsif($a[0] eq "S" || $a[0] eq "m") {       # Submenu/Macro-Call

    my $type = shift(@a);
    my $name = shift(@a);
    my $menu = shift(@a);
    die("Unknown menu/macro $menu referenced in line $line ($l)\n")
        if(!$defs{$menu});
    chkprint(sprintf("$type$c$name$c%s$c%s", $defs{$menu}, join($c, @a)));

  } elsif($l =~ m/^<xmllist>/) {
    my $on = $n;

    foreach my $rn (sort keys %r) {
      chkprint(sprintf "S:$rn:%02x", $on++);
    }
    chkprint "";

    $c = ":";
    @a = split($c, $l);

    foreach my $rn (sort keys %r) {
      chkprint "M$c$rn$c$c$a[1]";
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
        chkprint("S$c$k$c$f$c$d");
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
    print STDERR "Expression $str is too long, maxlen 32\n";
  }
  print "$str\n";
}
