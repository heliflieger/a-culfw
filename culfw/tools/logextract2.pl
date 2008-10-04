#!/usr/bin/perl

sub
doPrint()
{
  foreach my $k (sort keys %d) {
    printf("$k %2d / %3d\n", $d{$k}{num},$d{$k}{sum}/$d{$k}{num});
  }
}


while(my $l = <>) {
  chomp($l);
  my @a = split(" ", $l);

  if($a[4] =~ m/^(T....)00....$/)        { $d{$1}{num}++; $d{$1}{sum} += $a[5];}
  if($a[4] =~ m/^(K.).......$/)          { $d{$1}{num}++; $d{$1}{sum} += $a[5];}
  if($a[4] =~ m/^(K.).............$/)    { $d{$1}{num}++; $d{$1}{sum} += $a[5];}
  if($a[4] =~ m/^(E....)..............$/){ $d{$1}{num}++; $d{$1}{sum} += $a[5];}

}
doPrint();
exit(0);
