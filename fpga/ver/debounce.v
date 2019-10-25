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

module debounce
  #(parameter bounce_filter = 100000
    /* */)
   (input wire CLOCK,
    input wire RESET,
    input wire SIGNAL_IN,
    output reg SIGNAL_OUT
    /* */);

   // This just buffers the input signal, so that it is at least clean,
   // if not free of pulses.
   reg 	       signal_in2;
   always @(posedge CLOCK)
     signal_in2 <= SIGNAL_IN;

   // Now filter out pulses. The input needs to be stable for at least
   // bounce_filter clocks before we let the input propagate to the output.
   localparam BOUNCE_FILTER_BITS = $clog2(bounce_filter);
   reg [BOUNCE_FILTER_BITS : 0] filter;

   always @(posedge CLOCK)
     if (RESET) begin
	SIGNAL_OUT <= 1'b0;
	filter     <= bounce_filter;

     end else if (signal_in2 == SIGNAL_OUT) begin
	filter     <= bounce_filter;

     end else if (filter[BOUNCE_FILTER_BITS]) begin
	SIGNAL_OUT <= signal_in2;
	filter     <= bounce_filter;

     end else begin
	filter     <= filter - 1;
     end

endmodule // debounce
