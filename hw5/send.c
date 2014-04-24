/*
   udp4-s3.c: version 3 of sender
	 - uses symbolic name for address
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>

typedef struct packet {
	int rseqno;
	char rdata[100];
} packet;

int reliable_sendto(int seqno, int sock, void *data, int len, int flags, struct sockaddr *dest_addr, int dest_len)
{
	//copy seqno and message into new message struct
	packet msg;
	int ACK;
	msg.rseqno = seqno;
	strcpy(msg.rdata, data);
	//send message
	sendto(sock, &msg, sizeof(msg), 0 /* flags */, dest_addr, dest_len);
	printf("sent (%s) with seq=%u\n", msg.rdata, msg.rseqno);
	//prepare read_fds and timeout value for select()
	int read_fds = 0;
	struct timeval timeout;
	read_fds |= (1 << sock);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	int success = 0;
	//wait for ack
	while(!success){
		if(select(32, (fd_set *)&read_fds, NULL, NULL, &timeout) == 0){
			//timeout! send message again
			sendto(sock, &msg, sizeof(msg), 0 /* flags */, dest_addr, dest_len);
			printf("timeout. sent (%s) with seq=%u\n", msg.rdata, msg.rseqno);
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
		}
		else {
			//receive message
			recvfrom(sock, &ACK, 100, 0 /* flags */, dest_addr, &dest_len);
			printf("received ACK=%u\n", ACK);
			if(ACK == seqno){
				//ACK number matches, break out of loop
				success = 1;
				break;
			}
			else {
				//wrong ACK, prepare to receive again
				/*read_fds = 0;
				read_fds |= (1 << sock);
				timeout.tv_sec = 0;
				timeout.tv_usec = 1000;*/
			}
		}
	}
}

int main(int argc, char *argv[])
{
   int s;				/* s = socket */
   struct sockaddr_in in_addr;		/* Structure used for bind() */
   struct sockaddr_in sock_addr;	/* Output structure from getsockname */
   struct sockaddr_in dest_addr;	/* Destination socket */
   char line[100];
   char **p;
   int len;
	int seqno;			/* sequence number for stop-and-wait */

   struct hostent *host_entry_ptr;     /* Structure to receive output */

   if (argc < 3)
    { printf("Usage: %s <symbolic-dest-addr> <dest-port>\n", argv[0]);
      printf("   Program sends messages to <symbolic-dest-addr> <dest-port>\n");
      exit(1);
    }

   /* -------
      Fill in destination socket - this will identify IP-host + (UDP) port
      ------- */
   dest_addr.sin_family = AF_INET;		 /* Internet domain */

   host_entry_ptr = gethostbyname(argv[1]);   /* E.g.: cssun.mathcs.emory.edu */

   if (host_entry_ptr == NULL)
    { printf("Host `%s' not found...\n", argv[1]);     /* Just for safety.... */
      exit(1);
    }

   /**********************************************************************
    * NOTE: DO NOT use htonl on the h_addr_list[0] returned by 
    * gethostbyname() !!!
    **********************************************************************/
   memcpy((char *) &(dest_addr.sin_addr.s_addr), 
		   host_entry_ptr->h_addr_list[0], 
		   host_entry_ptr->h_length);

   /**********************************************************************/

   dest_addr.sin_port = htons(atoi(argv[2]));    /* Destination (UDP) port # */

   /* ---
      Create a socket
      --- */
   s = socket(AF_INET, SOCK_DGRAM, 0);

   /* ---
      Set up socket end-point info for binding
      --- */
   memset((char *)&in_addr, 0, sizeof(in_addr));
   in_addr.sin_family = AF_INET;                   /* Protocol domain */
   in_addr.sin_addr.s_addr = htonl(INADDR_ANY);    /* Use wildcard IP address */
   in_addr.sin_port = 0;	           	   /* Use any UDP port */

   /* ---
      Here goes the binding...
      --- */
   if (bind(s, (struct sockaddr *)&in_addr, sizeof(in_addr)) == -1)
    { printf("Error: bind FAILED\n");
    }
   else
    { printf("OK: bind SUCCESS\n");
    }

   /* ----
      Check what port I got
      ---- */
   len = sizeof(sock_addr);
   getsockname(s, (struct sockaddr *) &sock_addr, &len);
   printf("Socket s is bind to:\n");
   printf("  addr = %u\n", sock_addr.sin_addr.s_addr);
   printf("  port = %d\n", sock_addr.sin_port);

	seqno = 0;
   while(1)
    { printf("Enter a line: ");
      scanf("%[^\n]", &line);
      getchar();

      /* ----
	 sendto() will automatically use UDP layer
	 ---- */
      reliable_sendto(seqno, s, line, strlen(line)+1, 0 /* flags */, 
	     (struct sockaddr *)&dest_addr, sizeof(dest_addr));
		seqno++;
    }
}
