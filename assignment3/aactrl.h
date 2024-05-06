#define OUTPUT_SERIAL 0
#define MAX_PKT_LEN 35 // One more than msg length sent from Android

enum {
    FALSE,
    TRUE
};

/* Serial control type */
typedef struct sctrl {
  int connected; /* if not true, connect() must be run */
  int fd; /* File descriptor for the port */
} sctrl_t;

typedef struct {
  int sd; 			/* Socket */
  char *buf; 			/* Buffer */
  int bufsz;			/* Buffer length */

  int baud;			/* Serial device baud rate */
  int serial_port;		/* 0..4 for tty[USB][0..4] */
  sctrl_t s;			/* Serial control type */
} callback_obj_t;

/* Control [Arduino] device connection request */
extern int ctrlConn (sctrl_t *s, int baud, int port_num, int output);

/* Disconnect serial communication and reset serial control object */
extern void ctrlDisconnect (sctrl_t *s);

