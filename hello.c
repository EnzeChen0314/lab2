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

int vga_ball_fd;

/* Read and print the background color */
void print_background_color() {
  vga_ball_arg_t vla;
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_READ_BACKGROUND) failed");
      return;
  }
  printf("%02x %02x %02x\n",
	 vla.background.var1, vla.background.var2, vla.background.var3);
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
   unsigned char horhw = (unsigned char) hor;
   unsigned char verhw = (unsigned char) ver;
   
   position.var1 = (horhw & (unsigned char)255);
   position.var2 = (((horhw >> 8) & (unsigned char)7) | ((verhw << 3) & (unsigned char)248));
   position.var3 = (unsigned char)((verhw >> 5) & (unsigned char)63);
   
   
   return position;
}

void print_position() {
  vga_ball_arg_t vla;
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_BALL, &vla)) {
      perror("ioctl(VGA_BALL_READ_BACKGROUND) failed");
      return;
  }
  printf("%02x %02x %02x\n",
	 vla.background.var3, vla.background.var2, vla.background.var1);
}

void set_position(uint hor, uint ver)
{
  vga_ball_arg_t vla;
  vla.background = hardware_position(hor, ver);
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_BALL, &vla)) {
      perror("ioctl(VGA_BALL_SET_BACKGROUND) failed");
      return;
  }
}


int main()
{
  vga_ball_arg_t vla;
  int i;
	
  uint hor = 200;
  uint ver = 300;
  
  uint hormax = 1280-32;
  uint vermax = 480-32;
	
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
    set_background_color(&colors[i % COLORS ]);
    print_background_color();
    if (directx = 1){
	hor = hor + 1;
    } else {
	hor = hor - 1;
    }
    if (directy = 1){
	ver = ver + 1;
    } else {
	ver = ver - 1;
    }
    if (hor >= hormax ){
	directx = 0;
    } else if (hor <= 32) {
	directx = 1;
    }
    if (ver >= vermax ){
     	directx = 0;
    } else if (hor <= 32) {
	directx = 1;
    } 
    set_position(hor, ver);
    print_position();
    usleep(400000);
  }
  
  printf("VGA BALL Userspace program terminating\n");
  return 0;
}
