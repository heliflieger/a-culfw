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
  if($l =~ m+^\t\t<(.*) name="([^"]*)"+) {
    my $type=$1;
    next if(!($type eq "FHT" ||
              $type eq "CUL_WS" ||
              $type eq "CUL_EM"));

    $n=$2;
    $h{$n}{TYPE} = $type;
  }
  if($l =~ m+^\t\t<FHT name="([^"]*)"+) {
    $n=$1;
    $h{$n}{TYPE} = "FHT";
  }

  next if(!$n);
  $h{$n}{def}  = $1 if($l =~ m+^\t\t\t<INT key="DEF" value="([^"]*)"+);
  $h{$n}{room} = $1 if($l =~ m+^\t\t\t<ATTR key="room" value="([^"]*)"+);
  undef($n) if($l =~ m+^\t\t</+);
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

printf STDERR "Total menu/macro definitions: $n (max 64)\n";


##################
# Template file, second run
open(FH, $ARGV[1]) || die("$ARGV[1]: $!\n");
my $line = 0;
my $nitems = 0;
my $maxnitems = 0;
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
        my ($f,$d,$t) = ("","",$h{$k}{TYPE});
        if($t eq "FS20") {
          $f = ($h{$k}{sets} =~ m/dim/) ?
                $defs{"FS20-Dim"} : $defs{"FS20-Plain"};
          $d = $h{$k}{def};
        } else {
          $f = $defs{$t};
        }
        $d = $h{$k}{def} if($t eq "FS20" || $t eq "FHT");
        $d =~ s/ //g;
        $d = substr($d,0,6);  # Cut off group definitions

        chkprint("S$c$k$c$f$c$d");
      }
      chkprint "";
    }

  } else {

    chkprint("$l");

  }

  $nitems++;
  $maxnitems = $nitems if($maxnitems < $nitems);
  printf STDERR "WARNING: Too many subitems in line $line, 32 allowed\n"
               if($nitems > 32);
}

printf STDERR "Maximum number of subitems $maxnitems (max 32)\n";

sub
chkprint($)
{
  my $str = shift;
  if(length($str) > 32) {
    print STDERR "Expression $str is too long, maxlen 32\n";
  }
  print "$str\n";
}
