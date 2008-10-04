#!/usr/bin/perl

my %d;
my ($lf, $lbw) = ("N/A", "N/A");

sub
doPrint()
{
  my $f = ($d{f} ?sprintf("%2d/%3d",$d{f}{num},$d{f}{sum}/$d{f}{num}):"  0   ");
  my $k = ($d{k} ?sprintf("%2d/%3d",$d{k}{num},$d{k}{sum}/$d{k}{num}):"  0   ");
  my $s = ($d{s} ?sprintf("%2d/%3d",$d{s}{num},$d{s}{sum}/$d{s}{num}):"  0   ");
  my $e = ($d{e} ?sprintf("%2d/%3d",$d{e}{num},$d{e}{sum}/$d{e}{num}):"  0   ");
  printf("%s  %s    %s   %s     %s    %s\n", $lf, $lbw, $f, $k, $s, $e);
  %d = ();
}


printf("Freq   Bwidth   #FHT:15 #KS300:7  #S300:15    #EM:6\n");
while(my $l = <>) {
  chomp($l);
  my @a = split(" ", $l);

  if($a[3] eq "Setting") {
    if($a[4] eq "MDMCFG4") {
      $lbw = sprintf("%3d", $a[9]);
    } else {
      doPrint() if($lf);
      $lf = $a[11];
    }
  }

  if($a[3] eq "set" && $a[5] eq "eeprom") {
    if($a[6] =~ m/^.D/) {
      $lbw = $a[6];
    } else {
      doPrint() if($lf);
      $lf = $a[6];
    }
  }

  if($a[4] =~ m/T....00....$/)          { $d{f}{num}++; $d{f}{sum} += $a[5]; }
  if($a[4] =~ m/^K........$/)           { $d{s}{num}++; $d{s}{sum} += $a[5]; }
  if($a[4] =~ m/^K..............$/)     { $d{k}{num}++; $d{k}{sum} += $a[5]; }
  if($a[4] =~ m/^E..................$/) { $d{e}{num}++; $d{e}{sum} += $a[5]; }

}
doPrint();
exit(0);
