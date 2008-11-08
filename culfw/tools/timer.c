#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int
main(int ac, char *av[])
{
  int fd;
  unsigned char data;
  struct timeval tv;


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

  while(read(fd, &data, 1) == 1) {

    if((data>='f'&&data<='i') ||
       (data>='r'&&data<='u')) {

      int t;
      unsigned char t1, t2;
      gettimeofday(&tv, 0);
      read(fd, &t1, 1);
      read(fd, &t2, 1);
      t = ((t1<<8) | t2);
      printf("%u.%06d %c %c %5d\n", tv.tv_sec, tv.tv_usec, 
        data >= 'r' ? '.' : ' ', data, t);

    } else {

      if(data=='\r' ||
         data=='\n' ||
         data==' '  ||
         data=='p' ||
         (data>='0' && data<='9') ||
         (data>='A' && data<='Z')) {

        putchar(data);

      } else {

        printf("? %x\n", data);

      }

    }

    fflush(stdout);
  }
  return 0;
}
