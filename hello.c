/*
 * Userspace program that communicates with the vga_ball device driver
 * through ioctls
 *
 * Stephen A. Edwards
 * Columbia University
 */

#include <stdio.h>
#include "vga_ball.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int vga_ball_fd;

/* Read and print the background color */
void print_background_color() {
  vga_ball_arg_t vla;
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_READ_BACKGROUND) failed");
      return;
  }
//  printf("%02x %02x %02x\n",
	// vla.background.var1, vla.background.var2, vla.background.var3);
}

/* Set the background color */
void set_background_color(const vga_ball_color_t *c)
{
  vga_ball_arg_t vla;
  vla.background = *c;
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_SET_BACKGROUND) failed");
      return;
  }
}

static vga_ball_color_t hardware_position(uint hor, uint ver)
{
   vga_ball_color_t position;
   position.var1 = (unsigned char)(hor & 255);
   position.var2 = (unsigned char)(((hor >> 8) & 7) | ((ver << 3) & 248));
   position.var3 = (unsigned char)((ver >> 5) & 63);
   	
   printf("%02x %02x %02x\n",
	 position.var1, position.var2, position.var3);
   
   return position;
}

void print_position() {
  vga_ball_arg_t vla;
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_BALL, &vla)) {
      perror("ioctl(VGA_BALL_READ_BALL) failed");
      return;
  }
//  printf("%02x %02x %02x\n",
	 //vla.background.var3, vla.background.var2, vla.background.var1);
}

void set_position(uint hor, uint ver)
{
  vga_ball_arg_t vla;
  vla.background = hardware_position(hor, ver);
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_BALL, &vla)) {
      perror("ioctl(VGA_BALL_WRITE_BALL) failed");
      return;
  }
}


int main()
{
  vga_ball_arg_t vla;
  int i = 0;
	
  uint hor = 640;
  uint ver = 240;
  
  uint hormax = 1280-32;
  uint vermax = 480-16;
	
  int directx = 1;
  int directy = 1;

  static const char filename[] = "/dev/vga_ball";

  static const vga_ball_color_t colors[] = {
    { 0xff, 0x00, 0x00 }, /* Red */
    { 0x00, 0xff, 0x00 }, /* Green */
    { 0x00, 0x00, 0xff }, /* Blue */
    { 0xff, 0xff, 0x00 }, /* Yellow */
    { 0x00, 0xff, 0xff }, /* Cyan */
    { 0xff, 0x00, 0xff }, /* Magenta */
    { 0x80, 0x80, 0x80 }, /* Gray */
    { 0x00, 0x00, 0x00 }, /* Black */
  };

# define COLORS 8

  printf("VGA ball Userspace program started\n");

  if ( (vga_ball_fd = open(filename, O_RDWR)) == -1) {
    fprintf(stderr, "could not open %s\n", filename);
    return -1;
  }

  printf("initial state: ");
  print_background_color();

  while(1) {
    
    if (i % 100 == 0) 
    {
       int r = rand();
       int g = rand();
       int b = rand();
       vga_ball_color_t newcolor = {(unsigned char)r, (unsigned char)g, (unsigned char)b};
       set_background_color(&newcolor);
    }
    i++;
    //set_background_color(&colors[i % COLORS ]);
    //print_background_color();
    if (directx == 1){
	hor = hor + 2;
    } else {
	hor = hor - 2;
    }
    if (directy == 1){
	ver = ver + 1;
    } else {
	ver = ver - 1;
    }
    if (hor >= hormax ){
	directx = 0;
	//i++;
    } else if (hor <= 32) {
	directx = 1;
	//i++;
    }
    if (ver >= vermax ){
     	directy = 0;
	//i++;
    } else if (ver <= 16) {
	directy = 1;
	//i++;
    } 
    set_position(hor, ver);
    printf("%d %d\n", hor, ver);
    //print_position();
    usleep(8000);
  }
  printf("VGA BALL Userspace program terminating\n");
  return 0;
}
