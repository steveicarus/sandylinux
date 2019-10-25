#ifndef __slf_fpga_H
#define __slf_fpga_H
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

/*
 * This defines the user mode interface to the slf_fpga device driver.
 */
#ifdef __KERNEL__
# include  <linux/ioctl.h>
#else
# include  <sys/ioctl.h>
# include  <stdint.h>
#endif

/*
 * Set the status of the LEDs. There are 8 LEDs, with 4 bits that
 * make up 16 levels of brightness (0==off, 15==full on) mashed into a
 * 32bit word.
 */
struct slf_fpga_leds_s {
      uint32_t led_value;
};
# define SLF_FPGA_LEDS _IOW('F',0x10,struct slf_fpga_leds_s)

/*
 * The user input is broken up this way:
 *     PB0        - user_in_value[0]
 *     PB1        - user_in_value[1]
 *     PB2        - user_in_value[2]
 *     PB3        - user_in_value[3]
 *     DIP_SW0    - user_in_value[4]
 *     DIP_SW1    - user_in_value[5]
 *     DIP_SW2    - user_in_value[6]
 *     DIP_SW3    - user_in_value[7]
 *     <reserved> - user_in_value[31:8]
 */
struct slf_fpga_UserIn_s {
      uint32_t user_in_value;
};
# define SLF_FPGA_UserIn _IOR('F',0x11,struct slf_fpga_UserIn_s)

/*
 * Wait for one of the user inputs to change. The user_in_exp value is
 * the expected current state of the user inputs. The ioctl will wait
 * until the actual value differs from the expected value. This ioctl
 * then updates the user_in_value with the actual current state. The
 * assignment of bits is the same as for the UserIn ioctl.
 */
struct slf_fpga_wait_s {
      uint32_t user_in_value;
      uint32_t user_in_exp;
      uint32_t timeout_ms;
};
# define SLF_FPGA_WAIT _IOWR('F',0x12,struct slf_fpga_wait_s)

#endif
