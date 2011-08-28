#!/usr/bin/perl -w

#
# FS20 message evaluation
# by Dr. Boris Neubert 2011 omega at online dot de
# 
# This is for use with CUL/CUN/CUNO and similar devices with culfw
# see http://www.koeniglich.de/culfw/culfw.html
#
# Feel free to modify to your needs!
#
# Howto (for use with CUN under Linux):
# 1. Connect CUN to USB port.
# 2. at your terminal run screen /dev/ttyACM0
# 3. 	enter V to check version (optional)
# 4. 	enter X27 to start reception
# 5. terminate screen
# 6. at your terminal run cat /dev/ttyACM0 | F.pl


while(<STDIN>) {
  print $_;
  if(/^FA5CEAA...../) {
    my $d= substr $_, 9, 2;
    my $r= hex $d;
    printf("***** USF (%s) distance= %4dcm\n", $d, $r);
  } elsif(/^FA5CEAB...../) {
    my $h= substr $_, 9, 2;
    my $r= hex $h;
    printf("***** PS (%s) height= %4dcm\n", $h, $r);
  } elsif(/^FA5CEAC...../) {
    my $h= substr $_, 9, 2;
    my $cmd= substr $_, 7, 2;
    my $r= hex $h;
    printf("***** PS command %2s height= %4dcm\n", $cmd, $r);
  } elsif(/^F........../) {
    my $housecode= substr $_, 1, 4;
    my $button= substr $_, 5, 2;
    my $cmd= substr $_, 7, 2;
    my $arg= substr $_, 9, 2;
    printf("***** FS20\nhousecode %4s button %2s command %2s argument %2s\n", $housecode, $button, $cmd, $arg);
  }
}