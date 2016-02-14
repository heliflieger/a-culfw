my $s = 0xff;
my @s = ($ARGV[0] =~ m/../g);
for(my $i=0; $i<@s; $i++) {
  $s ^= hex($s[$i]);
}
printf("%02x\n", $s);
