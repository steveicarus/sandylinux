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

# include  <cstdio>
# include  <sys/types.h>
# include  <fcntl.h>
# include  <unistd.h>
# include  <slf_fpga.h>

int main(int argc, char*argv[])
{
      const char*dev_path = "/dev/slf_fpga0";

      int dev = open(dev_path, O_RDWR, 0);
      if (dev < 0) {
	    fprintf(stderr, "%s: Unable to open device\n", dev_path);
	    return -1;
      }
      struct slf_fpga_wait_s status;
      status.user_in_value = 0;
      status.user_in_exp = 0;
      status.timeout_ms = 500;

      for (;;) {
	    int rc = ioctl(dev, SLF_FPGA_WAIT, &status);
	    if (rc < 0)
		  break;

	    uint32_t changes = status.user_in_value ^ status.user_in_exp;

	    if (changes == 0) printf("????? No changes?\n");
	    if (changes&0x01) printf("PB0    : %s\n", status.user_in_value&0x01? "ON" : "OFF");
	    if (changes&0x02) printf("PB1    : %s\n", status.user_in_value&0x02? "ON" : "OFF");
	    if (changes&0x04) printf("PB2    : %s\n", status.user_in_value&0x04? "ON" : "OFF");
	    if (changes&0x08) printf("PB3    : %s\n", status.user_in_value&0x08? "ON" : "OFF");
	    if (changes&0x10) printf("DIP_SW0: %s\n", status.user_in_value&0x10? "ON" : "OFF");
	    if (changes&0x20) printf("DIP_SW1: %s\n", status.user_in_value&0x20? "ON" : "OFF");
	    if (changes&0x40) printf("DIP_SW2: %s\n", status.user_in_value&0x40? "ON" : "OFF");
	    if (changes&0x80) printf("DIP_SW3: %s\n", status.user_in_value&0x80? "ON" : "OFF");

	    status.user_in_exp = status.user_in_value;
      }

      close(dev);
      return 0;
}
