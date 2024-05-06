/* Copyright Rich West (Boston University)
 * aactrl -- [A]ndroid-[A]rduino Controller
 *
 * The basic logic reads data from an android device
 * via a TCP socket. This is then used to control
 * an Arduino (e.g., servo pair) via serial communication.
 *
 * Basic control logic:
 * Android events --> (TCP socket) --> PC --> (Serial port) --> Arduino
 * (1) Establish server connection [servConn]
 * (2) Establish (arduino) controller connection [ctrlConn]
 * (3) The ctrlConn connection can be established in a callback for the
 *     controller device [ctrlDevice]
 */

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "aactrl.h"

static sctrl_t sc;

void readSocket(int sd, char *buf, int bufsz);

/* This manages the connection with the controller [Arduino] device */
int ctrlConn(sctrl_t *s, int baud, int port_num, int output)
{
  struct termios options;
  int count, ret;

  s->connected = 0; /* reset in case we fail to connect */

  switch (port_num)
  {
  case 1:
    s->fd = open("/dev/ttyUSB1", O_RDWR | O_NOCTTY);
    break;
  case 2:
    s->fd = open("/dev/ttyUSB2", O_RDWR | O_NOCTTY);
    break;
  case 3:
    s->fd = open("/dev/ttyUSB3", O_RDWR | O_NOCTTY);
    break;
  case 4:
    s->fd = open("/dev/ttyUSB4", O_RDWR | O_NOCTTY);
    break;
  default:
    s->fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
    break;
  }

  if (s->fd == -1)
    return 0;
  else
  {
    if (!output)
      fcntl(s->fd, F_SETFL, 0); /* Blocking fd */
    else
      fcntl(s->fd, F_SETFL, 0 /*O_NONBLOCK*/);
  }

  tcgetattr(s->fd, &options);

  /* Set baud rate */
  switch (baud)
  {
  case 4800:
    cfsetispeed(&options, B4800);
    cfsetospeed(&options, B4800);
    break;
  case 9600:
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
    break;
  case 19200:
    cfsetispeed(&options, B19200);
    cfsetospeed(&options, B19200);
    break;
  case 38400:
    cfsetispeed(&options, B38400);
    cfsetospeed(&options, B38400);
    break;
  case 57600:
    cfsetispeed(&options, B57600);
    cfsetospeed(&options, B57600);
    break;
  case 115200:
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    break;
  default:
    fprintf(stderr, "Invalid serial baud rate!\n");
    exit(1);
  }

  /* Control options */
  options.c_cflag |= (CLOCAL | CREAD | HUPCL); /* enable */

  options.c_cflag &= ~PARENB; /* 8N1 */
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;

  if (!output)
  {

    /* Disable h/w flow control */
    options.c_cflag &= ~CRTSCTS;

    /* Input options */
    options.c_iflag &= ~(IXON | IXOFF | IXANY | IMAXBEL | IUTF8 |
                         IUCLC | ICRNL | IGNCR | INLCR | ISTRIP |
                         INPCK | PARMRK | IGNPAR | BRKINT);
    options.c_iflag |= IGNBRK;

    /* Local options... */
    /* Select raw input as opposed to canonical (line-oriented) input. */
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN |
                         ECHOK | ECHONL | NOFLSH | XCASE |
                         TOSTOP | ECHOPRT | ECHOCTL | ECHOKE);

    /* Output options */
    options.c_oflag &= ~(OPOST | OLCUC | OCRNL | ONLCR | ONOCR |
                         ONLRET | OFILL | OFDEL);
    options.c_oflag |= (NL0 | CR0 | TAB0 | BS0 | VT0 | FF0);

    /* Specify the raw input mininum number of characters to read in one
     * "packet" and timeout for data reception in 1/10ths of seconds. */
    options.c_cc[VMIN] = 1;
    options.c_cc[VTIME] = 5;
  }

  /* set all of the options */
  tcsetattr(s->fd, TCSANOW, &options);

  s->connected = 1;

  return 1;
}

/* This manages the connection between the server [PC] and the input Android
   device */
void servConn(int port,
              void (*callback)(callback_obj_t *obj),
              callback_obj_t *arg)
{

  int sd, new_sd;
  struct sockaddr_in name, cli_name;
  int sock_opt_val = 1;
  int cli_len;
  static char data[MAX_PKT_LEN]; /* Our receive data buffer. */

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("(servConn): socket() error");
    exit(-1);
  }

  if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&sock_opt_val,
                 sizeof(sock_opt_val)) < 0)
  {
    perror("(servConn): Failed to set SO_REUSEADDR on INET socket");
    exit(-1);
  }

  name.sin_family = AF_INET;
  name.sin_port = htons(port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sd, (struct sockaddr *)&name, sizeof(name)) < 0)
  {
    perror("(servConn): bind() error");
    exit(-1);
  }

  listen(sd, 5);

  for (;;)
  {
    cli_len = sizeof(cli_name);
    new_sd = accept(sd, (struct sockaddr *)&cli_name, &cli_len);
    printf("Assigning new socket descriptor:  %d\n", new_sd);

    if (new_sd < 0)
    {
      perror("(servConn): accept() error");
      exit(-1);
    }

    if (fork() == 0)
    { /* Child process */
      close(sd);

      arg->sd = new_sd;
      arg->buf = data;
      arg->bufsz = sizeof(data);

      (*callback)(arg); /* Invoke callback */
      exit(0);
    }
  }
}

void readSocket(int sd, char *buf, int bufsz)
{

  read(sd, buf, bufsz);
  printf("%s\n", buf);
}

/* Callback function for forwarding Android input to Arduino */
void ctrlDevice(callback_obj_t *o)
{

  char *token;
  int value;

  /* Setup serial connection to control [Arduino] device */
  ctrlConn(&(o->s), o->baud, o->serial_port, TRUE);

  while (1)
  {
    /* Get input from Android device */
    printf("Reading from socket...\n");
    readSocket(o->sd, o->buf, o->bufsz);

    // Use strtok to tokenize buf and extract speed/dir values,
    // which can then be sent to the arduino for managing servo control
    token = strtok(o->buf, " "); // 1st token is speed
    token = strtok(NULL, " ");   // 1st token is speed

    value = atoi(token);
    printf("Speed: %d\n", value);
    write((o->s).fd, &value, 1);

    token = strtok(NULL, " ");
    token = strtok(NULL, " ");

    value = atoi(token);
    printf("Direction: %d\n", value);
    write((o->s).fd, &value, 1);

    /* Reset buffer */
    memset(o->buf, 0, o->bufsz);
  }

  ctrlDisconnect(&(o->s));
}

int main(int argc, char *argv[])
{

  int option;
  static callback_obj_t obj;

  /* Defaults that can be over-ridden from the command-line */
  int baud = 57600;
  int tcp_port = 5050;             /* Default */
  int output_port = OUTPUT_SERIAL; /* /dev/ttyUSB0 */

  /* Parse command-line options for "b"aud rate, TCP "p"ort, and
   * serial "o"utput port. */
  while ((option = getopt(argc, argv, "b:o:p:h")) != -1)
    switch (option)
    {
    case 'b':
      if (optarg)
      {
        baud = atoi(optarg);
      }
      else
      {
        fprintf(stderr,
                "%s: missing baudrate "
                "[4800|9600|19200|38400|57600|115200]\n",
                argv[0]);
        exit(1);
      }
      break;
    case 'o':
      if (optarg)
      {
        output_port = atoi(optarg);
      }
      else
      {
        fprintf(stderr,
                "%s: missing ouput-port "
                "[0..4]\n",
                argv[0]);
        exit(1);
      }
      break;
    case 'p':
      if (optarg)
      {
        tcp_port = atoi(optarg);
      }
      else
      {
        fprintf(stderr,
                "%s: missing input-port "
                "[0|1]\n",
                argv[0]);
        exit(1);
      }
      break;
    case 'h':
      fprintf(stderr, "Usage: %s [-b baudrate] "
                      "[-o output-port][-p tcp-port]\n"
                      "baudrate: \t[4800|9600|19200|38400|57600|115200]\n"
                      "output-port: \t[0..4] for /dev/ttyUSB[0..4]\n"
                      "tcp-port: \t[0..65535] \n",
              argv[0]);
      exit(1);
    case '?':
      fprintf(stderr, "Unknown option -%c\n"
                      "Usage: %s [-b baudrate] "
                      "[-o output-port] [-p tcp-port] [-h]\n",
              optopt, argv[0]);
      exit(1);
    }

  /* Establish TCP endpoint for communication with Android device
   * Pass in a callback function to be invoked when connection is established
   */
  obj.baud = baud;
  obj.serial_port = output_port;
  obj.s = sc;
  servConn(tcp_port, ctrlDevice, &obj);

  return 0;
}
