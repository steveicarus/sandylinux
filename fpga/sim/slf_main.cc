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
 * This is the simulation engine for the SandyLinux sandbox AXI device.
 */

# define _STDC_FORMAT_MACROS
# include  <simbus_axi4.h>
# include  <cassert>

const char*port_string = "pipe:slf_master.pipe";
const unsigned slf_addr_width = 24;

/*
 * These are addresses on the AXI4 bus
 */
const uint32_t SLF_BUILD = 0x000000;
const uint32_t SLF_LEDs  = 0x000004;
const uint32_t SLF_UserIn     = 0x000008;
const uint32_t SLF_UserInExp  = 0x00000c;
const uint32_t SLF_UserInIEN  = 0x000010;


uint32_t slf_read32(simbus_axi4_t bus, uint64_t addr)
{
      uint32_t val;
      simbus_axi4_resp_t axi4_rc = simbus_axi4_read32(bus, addr, 0x00, &val);
      return val;
}

void slf_write32(simbus_axi4_t bus, uint64_t addr, uint32_t data)
{
      simbus_axi4_resp_t axi4_rc = simbus_axi4_write32(bus, addr, 0x00, data);
}


int main(int argc, char*argv[])
{
      int rc;
      simbus_axi4_t bus = simbus_axi4_connect(port_string, "master",
					      32, slf_addr_width, 4, 4, 1);
      assert(bus);

      printf("Wait 4 clocks...\n");
      fflush(stdout);
      simbus_axi4_wait(bus, 4, 0);

      printf("Reset bus...\n");
      fflush(stdout);
      simbus_axi4_reset(bus, 8, 8);

      uint32_t build_id = slf_read32(bus, SLF_BUILD);
      printf("FPGA BUILD = %" PRIu32 "\n", build_id);

      printf("Wait 4 clocks...\n");
      fflush(stdout);
      simbus_axi4_wait(bus, 4, 0);

      uint32_t LEDs = slf_read32(bus, SLF_LEDs);
      printf("LEDs = 0x%08" PRIx32 "\n", LEDs);

      printf("Wait 4 clocks...\n");
      fflush(stdout);
      simbus_axi4_wait(bus, 4, 0);

      slf_write32(bus, SLF_LEDs, 0x12345678);

      LEDs = slf_read32(bus, SLF_LEDs);
      printf("LEDs = 0x%08" PRIx32 " (s.b. 012345678)\n", LEDs);

      printf("Wait 4 clocks...\n");
      fflush(stdout);
      simbus_axi4_wait(bus, 4, 0);

      uint32_t user_in = slf_read32(bus, SLF_UserIn);
      uint32_t user_in_exp = slf_read32(bus, SLF_UserInExp);
      uint32_t user_in_ien = slf_read32(bus, SLF_UserInIEN);
      printf("UserIn   : 0x%08" PRIx32 "\n", user_in);
      printf("UserInExp: 0x%08" PRIx32 "\n", user_in_exp);
      printf("UserInIEN: 0x%08" PRIx32 "\n", user_in_ien);

	// Force an interrupt to happen by setting InExp different
	// from In, and enabling interrupts.
      slf_write32(bus, SLF_UserInIEN, 0x000000ff);
      slf_write32(bus, SLF_UserInExp, 0x00000011);
      simbus_axi4_wait(bus, 4, 0);

      user_in_exp = slf_read32(bus, SLF_UserInExp);
      user_in_ien = slf_read32(bus, SLF_UserInIEN);
      printf("UserInExp: 0x%08" PRIx32 " (s.b. 0x00000011)\n", user_in_exp);
      printf("UserInIEN: 0x%08" PRIx32 " (s.b. 0x000000ff)\n", user_in_ien);

      uint32_t irq_mask = 1;
      rc = simbus_axi4_wait(bus, 8, &irq_mask);
      printf("irq_mask after wait: 0x%08" PRIx32 " (expect an interrupt)\n", irq_mask);

	// Now clear that interrupt.
      slf_write32(bus, SLF_UserInExp, user_in);

      irq_mask = 1;
      rc = simbus_axi4_wait(bus, 8, &irq_mask);
      printf("irq_mask after wait: 0x%08" PRIx32 " (expect no interrupt)\n", irq_mask);

      printf("Wait 8 clocks...\n");
      fflush(stdout);
      simbus_axi4_wait(bus, 8, 0);

      simbus_axi4_end_simulation(bus);
      return 0;
}
