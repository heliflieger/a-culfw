#!/usr/bin/perl

use strict;
use warnings;

my %fnmaxstack;
my %fncomputed;
my %incomputing;
my %fnstack;
my %fncall;
my %interrupt;
my %missing;
my $fname;
my @stack;
sub checkstack($);

die("Usage: perl tools/stack.pl `find . -name \*.lst`\n") if(!@ARGV);

while(@ARGV) {
  my $fname = shift @ARGV;
  open(FH, $fname) || die("$fname: $!\n");
  $fname = "";
  while(my $l = <FH>) {
    chomp($l);
    if($l =~ m/^[^\t]*\t([A-Za-z].*):$/ ||
       $l =~ m/^[^\t]*\t(__vector_[0-9]*):$/) {
      $fname = $1;
      $fnstack{$fname} = 2;
      $interrupt{$fname} = 1 if($l =~ m/^[^\t]*\t(__vector_[0-9]*):$/);
    }
    if($fname) {
      $fnstack{$fname} += $1  if($l =~ m/frame size = ([0-9]*) /);
      $fnstack{$fname}++ if($l =~ m/push r[0-9]/);
      $fncall{$fname}{$1}++ if($l =~ m/^[^\t]*\t\tcall (.*)$/);
    }
    $fncall{analyze_ttydata}{$1}++ if($l =~ m/\t\t.word\tgs\((.*)\)$/);
  }
  close(FH);
}

$fncall{CDC_Task}{analyze_ttydata}++;
delete($fncall{display_char}{CDC_Task});

sub
checkstack($)
{
  my $fn = shift;
  if($incomputing{$fn}) {
    $fncomputed{$fn} = 999999;
    print("Recursion for $fn: @stack\n");
  }
  return $fncomputed{$fn} if($fncomputed{$fn});
  push(@stack, $fn);
  $incomputing{$fn} = 1;
  if($fnstack{$fn}) {
    $fnmaxstack{$fn} = "$fn($fnstack{$fn})";
  } else {
    $fnmaxstack{$fn} = "$fn(2?)";
  }

  my $max = 0;
  foreach my $c (sort keys %{ $fncall{$fn} }) {
    checkstack($c);
    if($fncomputed{$c} > $max) {
      $max = $fncomputed{$c};
      $fnmaxstack{$fn} = "$fn($fnstack{$fn}) $fnmaxstack{$c}";
    }
  }

  if($fnstack{$fn}) {
    $max += $fnstack{$fn};
  } else {
    $max += 2;
    if(!$missing{$fn}) {
      print "Missing stacksize for $fn\n";
      $missing{$fn} = 1;
    }
  }

  $fncomputed{$fn} = $max;
  pop(@stack);
  delete($incomputing{$fn});
}


checkstack("main");
foreach my $f (sort keys %interrupt) {
  checkstack($f);
}
print "\n";

my $maxintsize = 0;
my $maxintname = "";
foreach my $f (sort keys %interrupt) {
  if($maxintsize < $fncomputed{$f}) {
      $maxintsize = $fncomputed{$f};
      $maxintname = $f;
  }
}

print "Maximum stack size: main($fncomputed{main}) + INT($maxintsize) = " .
        ($fncomputed{main} +$maxintsize) . "\n";
print "Stack:\n";
print "$fnmaxstack{main}\n";
print "INTERRUPT: $fnmaxstack{$maxintname}\n";

print "\n";
foreach my $f (sort keys %fnstack) {
  my $fs = ($fnstack{$f} ? $fnstack{$f} : "");
  my $fms = ($fnmaxstack{$f} ? $fnmaxstack{$f} : "");
  print "$f: $fs $fms\n";
}
