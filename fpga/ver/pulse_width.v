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

`default_nettype none
`timescale 1ps/1ps

module pulse_width
  #(parameter CLOCK_DIVIDER = 50000
    /* */)
   (input wire  CLOCK,
    input wire 	RESET,
    // Width of the pulse
    input wire [3:0] WIDTH,
    // The generated pulse
    output reg 	PULSE
    /* */);

   // The clock_counter divides down the system clock to a pulse
   // clock that is much lower frequency, so that we don't have
   // high-frequency signals leaving the chip. The clock_phase
   // counter then counts out the phase position within the pulse
   // width of the generated signal.
   localparam             CLOCK_DIV_BITS = $clog2(CLOCK_DIVIDER);
   reg [CLOCK_DIV_BITS:0] clock_counter;
   reg [3:0] 		  clock_phase;
   always @(posedge CLOCK)
     if (RESET) begin
	clock_counter <= CLOCK_DIVIDER - 1;
	clock_phase   <= 1'b0;

     end else if (clock_counter[CLOCK_DIV_BITS]) begin
	clock_phase   <= clock_phase + 1;
	clock_counter <= CLOCK_DIVIDER - 1;

     end else begin
	clock_counter <= clock_counter - 1;
     end

   always @(posedge CLOCK)
     if (clock_phase < WIDTH)
       PULSE <= 1'b1;
     else
       PULSE <= 1'b0;

endmodule // pulse_width
