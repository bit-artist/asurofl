/* See LICENSE file for copyright and license details. */

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "util.h"


#define ASURO_CONNECT_RETRIES    100
#define ASURO_CONNECT_USLEEP    (100*1000)
#define ASURO_SEND_PAGE_RETRIES   10
#define ASURO_SEND_PAGE_USLEEP  (100*1000)

/*
 * Writes buffer to file descriptor. Returns error if not all
 * bytes could be written, else 0 on success.
 */
static int asuro_send(int fd, const uint8_t *buf, size_t buf_size)
{
	int ret;
	ssize_t bytes_written;

	if (fd<0||!buf||buf_size==0)
	{
		fprintf(stderr,"Error: send: Invalid argument!\n");
		return -1;
	}
	ret=0;
	bytes_written=write(fd,buf,buf_size);
	if (bytes_written<0)
	{
		perror("write");
		ret=bytes_written;
	}
	else if (bytes_written!=buf_size)
	{
		fprintf(stderr,"Error: write\n");
		ret=-2;
	}

	/* wait until written data has been transmitted */
	if (tcdrain(fd) != 0)
	{
		perror("tcdrain");
	}
	return ret;
}

/*
 * Non blocking read 1 byte from file descriptor. Return EOF if nothing was read.
 */
static int asuro_recv(int fd)
{
	ssize_t n;
	uint8_t c;

	n=read(fd,&c,sizeof(c));
	if (n<0)
	{
		perror("read");
		n=EOF;
	}
	else if (n==1)
	{
		n=c;
	}
	else
	{
		/* n==0 timeout or nothing to read */
		n=EOF;
	}
	return n;
}

/*
 * 2400 Baud, 8 data bits, 1 stop bit, No Parity,
 * No XON/XOFF SW flow control, No RTS/CTS HW flow control
 * Receiver on, Non blocking (poll), ignore modem lines
 * Non canonical mode (don't interpret control characters)
 */
static int init_port(const char *device)
{
	int fd;
	int res;
	struct termios t;

	fd=open(device,O_RDWR|O_NOCTTY);
	if (fd<0)
	{
		perror("open");
		return -1;
	}
	memset(&t,0,sizeof(t));
	res=tcgetattr(fd,&t);
	if (res != 0)
	{
		perror("tcgetattr");
		goto cleanup;
	}
	/* Enable the receiver and ignore modem control lines */
	t.c_cflag |= (CREAD | CLOCAL);
	/* Set character size */
	t.c_cflag &= ~CSIZE;
	t.c_cflag |= CS8;
	/* No parity */
	t.c_cflag &= ~PARENB;
	/* 1 stop bit */
	t.c_cflag &= ~CSTOPB;
	/* Disable hardware flow control */
	t.c_cflag &= ~CRTSCTS;
	/* Selecting raw input mode (non-canonical)
	 * characters are passed through unprocessed
	 */
	t.c_lflag &= ~(ISIG | ICANON | ECHO | ECHOE);
	/* Disable software flow control */
	t.c_iflag &= ~(IXON | IXOFF | IXANY);
	/* Selecting raw output, all other output flags are ignored */
	t.c_oflag &= ~OPOST;

	/* Polling, read will not block */
	t.c_cc[VMIN]=0;
	t.c_cc[VTIME]=0;

	/* Set baud rate */
	res=cfsetispeed(&t,B2400);
	if (res!=0)
	{
		perror("cfsetispeed");
		goto cleanup;
	}
	res=cfsetospeed(&t,B2400);
	if (res!=0)
	{
		perror("cfsetospeed");
		goto cleanup;
	}
	/* Discard all buffered data */
	if (tcflush(fd,TCIOFLUSH) != 0)
	{
		perror("tcflush");
	}
	res=tcsetattr(fd,TCSAFLUSH,&t);
	if (res != 0)
	{
		perror("tcsetattr");
		goto cleanup;
	}
	return fd;

cleanup:
	close(fd);
	return -1;
}

/*
 * This function sends a page that is wrapped in a frame to the
 * programmer and waits for the response message. The process
 * is repeated until receiving a response or reaching the retry
 * limit.
 *
 * fd           file descriptor
 * frame        frame consisting of page number, data and CRC
 *
 * Returns
 * 0 on success "OK"
 * 1 checksum error "CK"
 * 2 program memory error "ER"
 * 3 timeout
 * <0 for underlying function errors.
 */
static int asuro_send_frame(int fd, const uint8_t *frame)
{
	int status;
	int retries;
	int recv_buf[3];
	const char *err;

	retries=ASURO_SEND_PAGE_RETRIES;
	do
	{
		status=asuro_send(fd,frame,ASURO_FRAME_SIZE);
		if (status!=0)
			return status;
		usleep(ASURO_SEND_PAGE_USLEEP);

		/* Detect "OK", "CK" or "ER" followed by EOF */
		recv_buf[1]=EOF;
		recv_buf[2]=EOF;
		do
		{
			/* shift receive buffer */
			recv_buf[0]=recv_buf[1];
			recv_buf[1]=recv_buf[2];
			recv_buf[2]=asuro_recv(fd);
		}
		while (recv_buf[2]!=EOF);

		if (recv_buf[0]=='O'&&recv_buf[1]=='K')
		{
			/* Page Successfully Programmed (0): Done */
			status=0;
			err=NULL;
		}
		else if (recv_buf[0]=='C'&&recv_buf[1]=='K')
		{
			/* Checksum Error (1) */
			status=1;
			err="CRC Error";
		}
		else if (recv_buf[0]=='E'&&recv_buf[1]=='R')
		{
			/* Program Memory Error (2) */
			status=2;
			err="Memory Error";
		}
		else
		{
			/* Timeout (3) */
			status=3;
			err="Timeout";
		}

		if (err)
		{
			fprintf(stderr, "%s%s\n", err, retries?". Retrying":"");
		}

	}
	while (retries-- && status!=0);
	return status;
}

/*
 * Sending a final page with all data set to 0xff seems to be required by
 * the bootloader to exit programming mode. LED switches from Red to Green.
 *
 * fd           file descriptor
 */
static int asuro_send_final_page(int fd)
{
	uint8_t send_buf[ASURO_FRAME_SIZE];

	memset(send_buf,0xff,sizeof(send_buf));
	return asuro_send(fd,send_buf,sizeof(send_buf));
}

/*
 * This function sends out the ASCII string "Flash" until it receives
 * "ASURO" or hits the retry limit. Apparently, this brings ASURO's
 * bootloader into programming mode.
 *
 * fd           file descriptor
 *
 * Returns 0 if the connection could be established.
 */
static int asuro_connect(int fd)
{
	uint8_t flash_cmd[]={'F','l','a','s','h'};
	int status;
	int c;
	int retries;

	retries=ASURO_CONNECT_RETRIES;
	do
	{
		status=asuro_send(fd,flash_cmd,sizeof(flash_cmd));
		if (status!=0)
			return status;
		usleep(ASURO_CONNECT_USLEEP);

		/* state machine that reads "ASURO" */
		status='x';
		while ((c=asuro_recv(fd))!=EOF)
		{
			if (c=='A')
			{
				status='A';
			}
			else
			{
				switch (status)
				{
				case 'A':
					if (c=='S')
						status='S';
					else
						status='x';
					break;
				case 'S':
					if (c=='U')
						status='U';
					else
						status='x';
					break;
				case 'U':
					if (c=='R')
						status='R';
					else
						status='x';
					break;
				case 'R':
					if (c=='O')
						status='O';
					else
						status='x';
					break;
				case 'x':
				default:
					break;
				}
			}
		}
	}
	while (retries-- && status!='O');
	status=(status=='O')?0:-1;
	return status;
}

static void print_help(int err)
{
	fputs(
	"asurofl(ash) v1.0\n"
	"Usage: asurofl [-i <binary>] [-d <device>]\n"
	"\n"
	"  [-i <binary>]   Name of the raw binary image to flash.\n"
	"                  If not specified, read binary from stdin.\n"
	"\n"
	"  [-d <device>]   Device name where the IR-transceiver is connected to.\n"
	"                  Example: /dev/ttyS0\n"
	"                  If not specified, write string with hex digits to\n"
	"                  stdout.\n"
	, err?stderr:stdout);
}

int main(int argc, char *argv[])
{
	int status;
	int c;
	char *infile_name;
	char *device_name;
	FILE *fp_in;
	int fd_out;
	int nbytes;
	uint8_t frame[ASURO_FRAME_SIZE];

	infile_name=NULL;
	device_name=NULL;
	fp_in=NULL;
	fd_out=-1;
	while ((c=getopt(argc,argv,"hi:d:"))!=-1)
	{
		switch (c)
		{
		case 'i':
			infile_name=optarg;
			break;
		case 'd':
			device_name=optarg;
			break;
		case 'h':
			/* Output help to stdout instead of stderr. Help was requested. */
			print_help(0);
			return 0;
		case '?':
		default:
			print_help(1);
			return 1;
		}
	}

	if (infile_name)
	{
		fp_in=fopen(infile_name,"r");
		if (fp_in==NULL)
		{
			perror("fopen");
			fprintf(stderr,"Failed to open file: `%s'\n", infile_name);
			return 1;
		}
	}

	if (device_name)
	{
		fd_out=init_port(device_name);
		if (fd_out<0)
		{
			fprintf(stderr,"Failed to open device: `%s'\n", device_name);
			if (fp_in)
			{
				fclose(fp_in);
			}
			return 1;
		}
	}

	status=0;
	if (fp_in==NULL)
	{
		fp_in=stdin;
	}
	if (fd_out<0)
	{
		fd_out=STDOUT_FILENO;
	}
	else
	{
		fprintf(stderr, "Connecting..\n");
		status=asuro_connect(fd_out);
	}

	while (status==0 && (nbytes=get_frame(fp_in,frame))>0)
	{
		if (fd_out!=STDOUT_FILENO)
		{
			fprintf(stderr, "Sending page %3u\n", frame[0]);
			status=asuro_send_frame(fd_out,frame);
		}
		else
		{
			/* Send to stdout. Maybe teletype or file. */
			print_frame(frame);
		}
	}

	if (fd_out!=STDOUT_FILENO)
	{
		/* If successful and no input/output error so far,
		 * exit programming mode by sending the final page.
		 */
		if (status>=0)
		{
			(void)asuro_send_final_page(fd_out);
		}
		close(fd_out);
	}
	if (fp_in!=stdin)
	{
		fclose(fp_in);
	}
	if (status==0)
	{
		fprintf(stderr, "Success.\n");
	}
	else
	{
		fprintf(stderr, "Failed to program page. Error=%d\n", status);
		status=1;
	}
	return status;
}
