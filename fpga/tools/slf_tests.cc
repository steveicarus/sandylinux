/*
 * Copyright (c) 2019 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

# include  <slf_fpga.h>
# include  <cstdio>
# include  <cstdlib>
# include  <cstring>
# include  <sys/types.h>
# include  <fcntl.h>
# include  <unistd.h>

int main(int argc, char*argv[])
{
      const char*dev_path = "/dev/slf_fpga0";
      struct slf_fpga_leds_s setting;

      setting.led_value = 0x00000000;

      for (int arg_idx = 1 ; arg_idx < argc ; arg_idx += 1) {
	    if (strncmp(argv[arg_idx],"--path=",7) == 0) {
		  dev_path = argv[arg_idx]+7;
		  
	    } else if (strncmp(argv[arg_idx],"--leds=",7) == 0) {
		  setting.led_value = strtoul(argv[arg_idx]+7,0,0);

	    } else {
	    }
      }

      int dev = open(dev_path, O_RDWR, 0);
      if (dev < 0) {
	    fprintf(stderr, "%s: Unable to open device\n", dev_path);
	    return -1;
      }

      int rc = ioctl(dev, SLF_FPGA_LEDS, &setting);

      struct slf_fpga_UserIn_s user_in;
      rc = ioctl(dev, SLF_FPGA_UserIn, &user_in);
      printf("PB0: %s\n", user_in.user_in_value&0x01? "PRESSED" : "");
      printf("PB1: %s\n", user_in.user_in_value&0x02? "PRESSED" : "");
      printf("PB2: %s\n", user_in.user_in_value&0x04? "PRESSED" : "");
      printf("PB3: %s\n", user_in.user_in_value&0x08? "PRESSED" : "");
      printf("DIP_SW0: %s\n", user_in.user_in_value&0x10? "ON" : "off");
      printf("DIP_SW1: %s\n", user_in.user_in_value&0x20? "ON" : "off");
      printf("DIP_SW2: %s\n", user_in.user_in_value&0x40? "ON" : "off");
      printf("DIP_SW3: %s\n", user_in.user_in_value&0x80? "ON" : "off");

      close(dev);

      return 0;
}
