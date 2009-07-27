#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int
main(int ac, char *av[])
{
  int fd, limit;
  unsigned char odata, data, state, otype;

  if(ac != 3) {
    printf("Usage: timer /dev/ttyACM0 limit\n");
    exit(1);
  }
  limit = atoi(av[2]);


  fd = open(av[1], O_RDWR);
  if(fd == -1) {
    perror(av[1]);
    return 1;
  }
  data = 'X' ; write(fd, &data, 1);
  data = '1' ; write(fd, &data, 1);
  data = '8' ; write(fd, &data, 1);
  data = '\r'; write(fd, &data, 1);
  data = '\n'; write(fd, &data, 1);

  state = 0;
  while(read(fd, &data, 1) == 1) {

    if(state == 0) {
      if(data == 'r' || data == 'f') {
        odata = data;
        state++;
      } else if(data == '.') {
        otype = 0;
        printf("------\n");
      } else {
        printf("%x (%d)..\n", data, data<<4);
      }

    } else {
      struct timeval tv;
      gettimeofday(&tv, 0);
      printf("%u.%06d %c %s %5d  %d\n",
        (int)tv.tv_sec, (int)tv.tv_usec, odata,
          otype == odata ? "?" : " ", data << 4, (data<<4) < limit ? 0 : 1);
      otype = odata;
      fflush(stdout);
      state = 0;

    }

  }
  return 0;
}
