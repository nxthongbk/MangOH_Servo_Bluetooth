/**
 * Description: This file is migrated from https:github.com/Seeed-Studio/Seeed_RGB_LED_Matrix
 * 		Some functions have been renamed but its code still be the same
 * Author: tqkieu@tma.com.vn
 * 
 **/

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/io.h>
#include <stdio.h>
#include <errno.h>
#include "legato.h"
#include "interfaces.h"

#define PWMDATAHUB_DATAHUB_SERVO	"pwdDataHub/servo"

int open_uart1(const char *dev) {
	int     fd;
	fd = open (dev, O_RDWR | O_NOCTTY | O_NDELAY);
	struct termios options;
	// The old way. Let's not change baud settings
	fcntl (fd, F_SETFL, 0);
	// get the parameters
	tcgetattr (fd, &options);
	// Set the baud rates to 115200...
	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);
	// Enable the receiver and set local mode...
	options.c_cflag |= (CLOCAL | CREAD);
	// No parity (8N1):
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
 	// enable hardware flow control (CNEW_RTCCTS)
	// options.c_cflag |= CRTSCTS;
	// if(hw_handshake)
	// Disable the hardware flow control for use with mangOH RED
	options.c_cflag &= ~CRTSCTS;

	// set raw input
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_iflag &= ~(INLCR | ICRNL | IGNCR);

	// set raw output
	options.c_oflag &= ~OPOST;
	options.c_oflag &= ~OLCUC;
	options.c_oflag &= ~ONLRET;
	options.c_oflag &= ~ONOCR;
	options.c_oflag &= ~OCRNL;

	// Set the new options for the port...
	tcsetattr (fd, TCSANOW, &options);

	return fd;
}

void read_uart1(int fd)
{
	char read_buffer[32];   /* Buffer to store the data received              */
	int  bytes_read = 0;    /* Number of bytes read by the read() system call */

	bytes_read = read(fd,&read_buffer,10); /* Read the data                   */

	LE_INFO("%d",bytes_read);
	LE_INFO("%c",read_buffer[0]);
	LE_INFO("\n----------------------------------\n");
}

void write_uart1 (int fd, char *cmd)
{
	int     wrote = 0;
	LE_INFO("MSG: \"%s\"", cmd);
	wrote = write (fd, cmd, strlen (cmd));
	LE_INFO("wrote  %d",wrote);
}

static void uartThread(void)
{
	#define BUFF_SIZE 32

	int fetched_bytes = 0;
	char fetch_buffer[BUFF_SIZE];
	int uart_fd = 0;
	char json_str[IO_MAX_STRING_VALUE_LEN];

	uart_fd = open_uart1("/dev/ttyHS0");
	if (uart_fd == -1) {
		char buffer[256];
		strerror_r(errno, buffer, 256);
		LE_INFO("Failed to open UART1 '%s'", buffer);
		return;
	}
	
	LE_INFO("Bluetooth service started");
	while(true) {
		fetched_bytes = read(uart_fd, fetch_buffer, BUFF_SIZE-1);
		if (fetched_bytes > 0) {
			fetch_buffer[fetched_bytes] = 0;
			LE_INFO("buffer '%s'", fetch_buffer);
			LE_INFO("size '%d'", fetched_bytes);

			if (fetch_buffer[fetched_bytes - 1] == '\r' ||
			    fetch_buffer[fetched_bytes - 1] == '\n') {
				fetch_buffer[fetched_bytes - 1] = 0;
			}
			if (fetch_buffer[fetched_bytes - 2] == '\r' ||
			    fetch_buffer[fetched_bytes - 2] == '\n') {
				fetch_buffer[fetched_bytes - 2] = 0;
			}

			// push servo info
			memset(json_str, 0 , IO_MAX_STRING_VALUE_LEN);
			sprintf(json_str,
				"{\"interface\":1,\"angle\":%s}",
				fetch_buffer);
			io_PushJson(PWMDATAHUB_DATAHUB_SERVO, IO_NOW, (const char *)json_str);
		}
		sleep(0.1);
	}
}

COMPONENT_INIT
{
	le_result_t result;

	// This will be provided to the Data Hub. --> send to ledmatrix
	result = io_CreateInput(PWMDATAHUB_DATAHUB_SERVO,
				IO_DATA_TYPE_JSON,
				"servo");
	LE_ASSERT(result == LE_OK);

	result = admin_SetSource("/app/pwmDataHub/" PWMDATAHUB_DATAHUB_SERVO,
				 "/app/bluetoothDataHub/" PWMDATAHUB_DATAHUB_SERVO);
	LE_ASSERT(result == LE_OK);

	uartThread();
}
