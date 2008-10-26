#include <stdio.h>

int
main(int ac, char *av[])
{
  char buf[256];
  int d, cnt = 0;

  if(!fgets(buf, sizeof(buf), stdin))
    return 0;
  if(strncmp(buf, "P6", 2) && strncmp(buf, "P5", 2)) {
    fprintf(stderr, "Not a PPM/PGM raw input file (P5/P6 needed)\n");
    return 1;
  }
  printf(buf);

  while(cnt < 2) {
    if(!fgets(buf, sizeof(buf), stdin))
      return 0;
    if(buf[0] != '#')
      cnt++;
    printf(buf);
  }
  while((d = getchar()) != EOF) {
    d &= 0xf0;
    putchar(d);
  }
  return 0;
}
