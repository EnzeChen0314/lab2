/*
 *
 * CSEE 4840 Lab 2 for 2019
 *
 * Name/UNI: Enze Chen/ Youfeng Chen
 */
#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>
#include <stdbool.h>

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* arthur.cs.columbia.edu */
#define SERVER_HOST "128.59.19.114"
#define SERVER_PORT 42000

#define BUFFER_SIZE 128

#define MAX_COL 64
#define MAX_ROW_R 20
#define MAX_ROW_S 23

/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
void *network_thread_f(void *);

void initial();
void memRclear();
//void memSclear();
void gonext();
void golast();
void cursorshow();
void ramshow();
void ramclear();
void del();
int cursor2ram();

int rowr = 1;
int cursor1 = MAX_ROW_R + 1, cursor2 = 0;
char sendram[2 * MAX_COL];
bool sendfull, receivefull;
int pos1 = 0;


int main()
{
  int err;
	
  struct sockaddr_in serv_addr;

  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[12];


  // initial the screen display
  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
	}
  initial();

  /* Open the keyboard */
  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
    fprintf(stderr, "Did not find a keyboard\n");
    exit(1);
  }
    
  /* Create a TCP communications socket */
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

  /* Get the server address */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  if ( inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
    exit(1);
  }

  /* Connect the socket to the server */
  if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
    exit(1);
  }

  /* Start the network thread */
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  /* Look for and handle keypresses */
  char send0, send1;
  int pos = 0;
  int nxtready0 = 1;
  int nxtready1 = 1;
  for (;;) {
    libusb_interrupt_transfer(keyboard, endpoint_address, (unsigned char *) &packet, sizeof(packet), &transferred, 0);
    if (transferred == sizeof(packet)) {
      sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0], packet.keycode[1]);
      printf("%s\n", keystate);
      
      send0 = keystateconvert(packet.modifiers, packet.keycode[0]);
      send1 = keystateconvert(packet.modifiers, packet.keycode[1]);
      if ((int)send0 == 178) nxtready0 = 1;
      if ((int)send1 == 178) nxtready1 = 1;   
      if (!sendfull) {
	  if (nxtready0){
	    if ((int)send1 == 178) {     
              if ((int)send0 != 177) {
                if ((int)send0 != 178) {
	          if ((int)send0 != 180) {
	            if ((int)send0 != 181) {
	              if ((int)send0 != 179) {
	                pos = cursor2ram(cursor1, cursor2);	
		        sendram[pos] = send0;
			pos1++;
	                gonext();
			nxtready0 = 0;
		      }
		      else del();
	            }
	            else gonext();
	          }
	          else golast();
	        }
      	      }
	    }   
	  }
        if (nxtready1){
	  if ((int)send1 != 177) {
	    if ((int)send1 != 178) {
	      if ((int)send1 != 180) {
	        if ((int)send1 != 181) {
	          if ((int)send1 != 179) {
	  	    pos = cursor2ram();
	  	    sendram[pos] = send1;
	            pos1++;
	            gonext();
		    nxtready1 = 0;
		    }
		  else del();
	        }
	        else gonext();
	      }
	      else golast();
	    }
	  }
        }
      }
      else {
      if ((int)send0 == 179) {sendfull = 0; del();}
      if ((int)send0 == 180) {sendfull = 0; golast();}
      }
	    
      if ((int)send0 == 177) { write(sockfd, sendram, pos1); pos1 = 0; ramclear();} 
      
			      
      if (packet.keycode[0] == 0x29) { /* ESC pressed? */
	      break;
      }
    }
  }

  /* Terminate the network thread */
  pthread_cancel(network_thread);

  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}

void *network_thread_f(void *ignore)
{
	
  char recvBuf[BUFFER_SIZE];
  int n;
	
  /* Receive data */
  while ( (n = read(sockfd, &recvBuf, BUFFER_SIZE - 1)) > 0 ) {
    recvBuf[n] = '\0';
    printf("%s", recvBuf);
    if (!receivefull) {
      if (rowr >= MAX_ROW_R - 1) receivefull = 1;
    }	
    else {
      rowr = 1;
      memRclear();
    }
    rowr = fbputswrap(recvBuf, rowr, 0, MAX_ROW_R, MAX_COL);
    for (int i = 0; i < n; i++) recvBuf[i] = ' ';
    rowr++;
  }

  return NULL;
}

void initial()
{
  /* Draw rows of asterisks across the top and bottom of the screen */
  // Draw the horizontal line that split into receive region and send region
  for (int col = 0 ; col < MAX_COL ; col++) {
    fbputchar('*', 0, col);
    fbputchar('-', MAX_ROW_R, col);
    fbputchar('*', MAX_ROW_S, col);
  }

  // Screen Clean
  memRclear();
  ramclear();
}

void memRclear()
{
  for (int col = 0 ; col < MAX_COL ; col++) {
    for (int row = 1 ; row < MAX_ROW_R ; row++) fbputchar(' ', row, col);
  }
  receivefull = 0;
}

/*void memSclear()
{
  for (int col = 0 ; col < MAX_COL ; col++) {
    for (int row = MAX_ROW_R+1 ; row < MAX_ROW_S ; row++) fbputchar(' ', row, col);
  }
  cursor1 = MAX_ROW_R + 1; cursor2 = 0;
  cursorshow();
}*/

void ramclear()
{
  for (int i = 0; i < MAX_COL * 2; i++) sendram[i] = ' ';
  cursor1 = MAX_ROW_R + 1; cursor2 = 0; sendfull = 0;
	ramshow();
	cursorshow();
}

void gonext()
{
  if (!sendfull) {
    if (cursor2 == MAX_COL - 1) {
      if (cursor1 == MAX_ROW_S - 1) {sendfull = 1; cursor2++;}
      else if (cursor2ram() <= pos1){
        cursor1++;
        cursor2 = 0;
      }
    }
    else if (cursor2ram() <= pos1) cursor2++;
  }
  ramshow();
  cursorshow();
}

void golast()
{
  if (!sendfull) {
    if (cursor2 == 0) {
      if (cursor1 == MAX_ROW_S - 1) {
        cursor2 = MAX_COL - 1;
        cursor1 = MAX_ROW_R + 1;
      }
    }
    else cursor2--;
  }
  if (cursor2 >= MAX_COL) 
  {
	cursor2 = MAX_COL - 1;
        cursor1 = MAX_ROW_S - 1;  
  }
	ramshow();
	cursorshow();
}

int cursor2ram()
{
    return (cursor1 - MAX_ROW_R - 1) * MAX_COL + cursor2;
}

void cursorshow()
{
  if (cursor2 < MAX_COL) fbputchar('_', cursor1, cursor2);
}

void ramshow()
{
	fbputswrap(sendram, MAX_ROW_R + 1, 0, MAX_ROW_S, MAX_COL);
}

void del()
{
	golast();
	int tem = cursor2ram();
	for (int i = tem; i < 2 * MAX_COL - 1; i++) sendram[i] = sendram[i + 1];
	sendram[2 * MAX_COL - 1] = ' ';
	if (pos1 > 0) pos1 --;
	ramshow();
	cursorshow();
}
