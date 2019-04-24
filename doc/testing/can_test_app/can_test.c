/* A simple SocketCAN example */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include <errno.h>

int soc;
int read_can_port;

int open_port(const char *port)
{
	struct ifreq ifr;
	struct sockaddr_can addr;

	/* open socket */
	soc = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(soc < 0)
	{
		return (-1);
	}

	addr.can_family = AF_CAN;
	strcpy(ifr.ifr_name, port);

	if (ioctl(soc, SIOCGIFINDEX, &ifr) < 0)
	{
		return (-1);
	}

	addr.can_ifindex = ifr.ifr_ifindex;

	fcntl(soc, F_SETFL, O_NONBLOCK);

	if (bind(soc, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		return (-1);
	}

	return 0;
}

int send_port(struct can_frame *frame)
{
	int retval;
	retval = write(soc, frame, sizeof(struct can_frame));
	if (retval != sizeof(struct can_frame))
	{
		return (-1);
	}
	else
	{
		return (0);
	}
}

/* this is just an example, run in a thread */
void read_port()
{
	struct can_frame frame_rd;
	int recvbytes = 0;

	read_can_port = 1;
	while(read_can_port)
	{
		struct timeval timeout = {1, 0};
		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(soc, &readSet);

		if (select((soc + 1), &readSet, NULL, NULL, &timeout) >= 0)
		{
			if (!read_can_port)
			{
				break;
			}
			if (FD_ISSET(soc, &readSet))
			{
				recvbytes = read(soc, &frame_rd, sizeof(struct can_frame));
				if(recvbytes)
				{
					printf("dlc = %d, data = %s\n", frame_rd.can_dlc,frame_rd.data);
				}
			}
		}
	}
}

int close_port()
{
	close(soc);
	return 0;
}

int main(void)
{
	struct ifreq ifr;
	struct sockaddr_can addr;
	struct can_frame frame;
	int ret;
	int s;
	ssize_t len;

	memset(&ifr, 0x0, sizeof(ifr));
	memset(&addr, 0x0, sizeof(addr));
	memset(&frame, 0x0, sizeof(frame));

	printf("CAN test\n");

	/* open CAN_RAW socket */
	s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	printf("socket() return %d%d\n", s);
	if (s == -1 )
		return -1;

	/* Convert interface string "can0" into interface index */
	strcpy(ifr.ifr_name, "can0");
	ret = ioctl(s, SIOCGIFINDEX, &ifr);
	printf("ioctl(SIOCGIFINDEX) returned %d\n", ret);
	if (ret)
		return -1;

	/* setup address for bind */
	addr.can_ifindex = ifr.ifr_ifindex;
	addr.can_family	= PF_CAN;

	/* bind socket to the can0 interface */
	ret = bind(s, (struct sockaddr *)&addr, sizeof(addr));
	printf("bind() returned %d\n", ret);
	if (ret)
		return -1;

	/* first fill, then send the CAN frame */
	frame.can_id = 0x23;
	strcpy((char *)frame.data, "hello");
	frame.can_dlc = 5;
	len = write(s, &frame, sizeof(frame));
	printf("write() returned %d\n", len);
	if( len == -1 )
	{
		printf("write error: %s\n", strerror(errno));
		return -1;
	}

	/* first fill, then send the CAN frame */
	frame.can_id = 0x23;
	strcpy((char *)frame.data, "iCC2012");
	frame.can_dlc = 7;
	len = write(s, &frame, sizeof(frame));
	printf("write() returned %d\n", len);
	if( len == -1 )
	{
		printf("write error: %s\n", strerror(errno));
		return -1;
	}

	close(s);

	return 0;
}

