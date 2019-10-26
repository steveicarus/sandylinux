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
 * This device connects to one of the GP AXI ports of the PS system in
 * the Zynq chip. The address width of that bus is 24bits, and we only
 * implement AXI4-Lite, so an AXI-Interconnect device is needed to match
 * it up with the actual processor.
 * 
 * The design assumes the Zynq is installed on an Avnet MicroZed I/O
 * Carrier Card.
 *
 * LEDs can be set to off (LEDn=0), full on (LEDn=15) or brightness levels
 * in between. The hardware will use pulse width modulation to adjust the
 * brightness of the output.
 *
 * REGISTER MAP
 * The registers are 32bit words. The addresses are given as byte addresses,
 * which are 24 bits in this context.
 *
 *   24'h00_0000   [31: 0]  (ro) BUILD ID
 *   24'h00_0004   [31: 0]  (rw) LEDs (4 bits per LED, 0==off, 7==full on)
 *                                 [ 3: 0] LED0
 *                                 [ 7: 4] LED1
 *                                 [11: 8] LED2
 *                                 [15:12] LED3
 *                                 [19:16] LED4
 *                                 [23:20] LED5
 *                                 [27:24] LED6
 *                                 [31:28] LED7
 *   24'h00_0008   [31: 0]  (ro) UserIn
 *   24'h00_000c   [31: 0]  (rw) UserInExpect
 *   24'h00_0010   [31: 0]  (rw) UserInIEN
 *                                    [ 0] PB0
 *                                    [ 1] PB1
 *                                    [ 2] PB2
 *                                    [ 3] PB3
 *                                    [ 4] DIP_SW0
 *                                    [ 5] DIP_SW1
 *                                    [ 6] DIP_SW2
 *                                    [ 7] DIP_SW3
 *                                 [31: 8] <reserved>
 *
 * Each user input can generate an interrupt if the corresponding bit
 * in the UserInIEN register is enabled. And interrupt is generated
 * when this expression is true: (UserIn ^ UserInExpect) & UserInIEN.
 * In other words, if the interrupt is enabled, and the actual
 * button value differs from the expected value, an interrupt is
 * generated. So interrupts can be cleared by writing the value of
 * UserIn into UserInExpect.
 */
`default_nettype none
`timescale 1ps/1ps

module SLF_FPGA
  #(parameter addr_width = 24,
    parameter BURST_LEN_ORDER = 4
    /* */)
   (// AXI4 port connected to a GP port. This is a slave port, with
    // the processor the master.
    
    // Global signals
    input wire 			AXI_S_ACLK,
    input wire 			AXI_ARESETn,
    // Write address channel
    input wire 			AXI_S_AWVALID,
    output wire 		AXI_S_AWREADY,
    input wire [addr_width-1:0] AXI_S_AWADDR,
    input wire [2:0] 		AXI_S_AWPROT,
    // Write data channel
    input wire 			AXI_S_WVALID,
    output wire 		AXI_S_WREADY,
    input wire [31:0] 		AXI_S_WDATA,
    input wire [3:0] 		AXI_S_WSTRB,
    // Write response channel
    output wire 		AXI_S_BVALID,
    input wire 			AXI_S_BREADY,
    output wire [1:0] 		AXI_S_BRESP,
    // Read address channel
    input wire 			AXI_S_ARVALID,
    output wire 		AXI_S_ARREADY,
    input wire [addr_width-1:0] AXI_S_ARADDR,
    input wire [2:0] 		AXI_S_ARPROT,
    // Read data channel
    output wire 		AXI_S_RVALID,
    input wire 			AXI_S_RREADY,
    output wire [31:0] 		AXI_S_RDATA,
    output wire [1:0] 		AXI_S_RRESP,
    // interrupts
    output wire 		INTERRUPT,

    // LED outputs. (active high)
    output wire 		LED0,
    output wire 		LED1,
    output wire 		LED2,
    output wire 		LED3,
    output wire 		LED4,
    output wire 		LED5,
    output wire 		LED6,
    output wire 		LED7,
    // Push buttons (active/pressed == high)
    input wire 			PB0,
    input wire 			PB1,
    input wire 			PB2,
    input wire 			PB3,
    // DIP switches (ON == high)
    input wire 			DIP_SW0,
    input wire 			DIP_SW1,
    input wire 			DIP_SW2,
    input wire 			DIP_SW3
    /*  */);

   localparam [addr_width-1:0] ADDRESS_BUILD_ID = 'h00_0000;
   localparam [addr_width-1:0] ADDRESS_LEDs     = 'h00_0004;
   localparam [addr_width-1:0] ADDRESS_UserIn   = 'h00_0008;
   localparam [addr_width-1:0] ADDRESS_UserInExp= 'h00_000c;
   localparam [addr_width-1:0] ADDRESS_UserInIEN= 'h00_0010;

   // Make an active-high version of the reset signal. The reset goes
   // to so much stuff, that we want it buffered. We might as well make
   // it active high at the same time.
   reg 				reset_int;
   always @(posedge AXI_S_ACLK) reset_int <= ~AXI_ARESETn;

   // Receive the write address.
   reg [addr_width-1:0] write_address;
   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
	write_address <= 0;
     end else if (AXI_S_AWREADY & AXI_S_AWVALID) begin
	write_address <= AXI_S_AWADDR;
     end

   // Receive the read address
   reg [addr_width-1:0] read_address;
   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
	read_address <= 0;
     end else if (AXI_S_ARREADY & AXI_S_ARVALID) begin
	read_address <= AXI_S_ARADDR;
     end

   // The build id of the device. This module, and the output patterns
   // it emits, is generated at compile time.
   wire [31:0] build_id;
   BUILD build_id_mod(.build_id(build_id));

   // This is the Read/Write register that controls the brightness of
   // the output LEDs.
   reg [31:0]  LEDs_register;
   wire        LEDs_register_hit_w = (write_address == ADDRESS_LEDs);

   // This is the Read-only register that reads the switches.
   wire [31:0] UserIn_register;

   reg [31:0]  UserInExp_register;
   wire        UserInExp_register_hit_w = (write_address == ADDRESS_UserInExp);

   reg [31:0]  UserInIEN_register;
   wire        UserInIEN_register_hit_w = (write_address == ADDRESS_UserInIEN);

   // This state machine drives the read process. Wait for a read
   // address. When we get the read address, mark the read as ready,
   // holding off the next address. When the read completes, then
   // be ready for the next address. To wit:
   //
   //    ARREADY: + + + - - - +
   //    ARVALID: - - + _ . . .
   //    RREADY : . . . . - + -
   //    RVALID : - - - - + + -
   //
   reg 	      reg_s_arready;
   reg 	      reg_s_rvalid;
   reg [31:0] reg_s_rdata;
   assign     AXI_S_ARREADY = reg_s_arready;
   assign     AXI_S_RVALID  = reg_s_rvalid;
   assign     AXI_S_RDATA   = reg_s_rdata;
   assign     AXI_S_RRESP   = 2'b00;

   // Deliver the addressed read register contents to the read data
   // buffer, ready for a read cycle. It is safe and efficient to just
   // track the read address, because the state machine will assure
   // that only one read is in progress, and will only flag the data
   // as valid if it really is valid.
   always @(posedge AXI_S_ACLK)
     case (read_address)
       ADDRESS_BUILD_ID : reg_s_rdata <= build_id;
       ADDRESS_LEDs     : reg_s_rdata <= LEDs_register;
       ADDRESS_UserIn   : reg_s_rdata <= UserIn_register;
       ADDRESS_UserInExp: reg_s_rdata <= UserInExp_register;
       ADDRESS_UserInIEN: reg_s_rdata <= UserInIEN_register;
       default          : reg_s_rdata <= 32'd0;
     endcase

   reg 	      read_address_ready_del;

   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
	reg_s_arready <= 1'b1;
	reg_s_rvalid  <= 1'b0;
	read_address_ready_del <= 1'b0;

     end else if (AXI_S_ARREADY && AXI_S_ARVALID) begin
	// If a read address arrives, then switch to the read step. Put
	// in a delay to give a chance for the read address to collect
	// the read data.
	reg_s_arready <= 1'b0;
	read_address_ready_del <= 1'b1;

     end else if (read_address_ready_del) begin
	// The read address was delivered, and the correct data was clocked
	// into the read_data register. Now set it up to be read by the
	// processor.
	read_address_ready_del <= 1'b0;
	reg_s_rvalid <= 1'b1;

     end else if (AXI_S_RREADY && AXI_S_RVALID) begin
	// The processor accepts the read response, so now we can go back
	// to waiting for the next read address.
	reg_s_rvalid  <= 1'b0;
	reg_s_arready <= 1'b1;

     end else begin
	// Waiting for something to happen.
     end

   // The write cycle involves 3 channels: the write address, the
   // write data, and the write response. The state machine flows
   // the state by waiting for the address channel, then the write
   // data channel, and then sending a response.
   reg 			       reg_s_awready;
   reg 			       reg_s_wready;
   reg 			       reg_s_bvalid;
   assign AXI_S_AWREADY = reg_s_awready;
   assign AXI_S_WREADY  = reg_s_wready;
   assign AXI_S_BVALID  = reg_s_bvalid;
   assign AXI_S_BRESP   = 2'b00;
   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
	reg_s_awready <= 1'b1;
	reg_s_wready  <= 1'b0;
	reg_s_bvalid  <= 1'b0;

     end else if (AXI_S_AWREADY & AXI_S_AWVALID) begin
	// An address has been sent. Get ready to receive data.
	reg_s_awready <= 1'b0;
	reg_s_wready  <= 1'b1;

     end else if (AXI_S_WREADY & AXI_S_WVALID) begin
	// Data has been sent. Now send the response.
	reg_s_wready  <= 1'b0;
	reg_s_bvalid  <= 1'b1;

     end else if (AXI_S_BREADY & AXI_S_BVALID) begin
	// Sender has received the response. Go back to
	// waiting for an address.
	reg_s_awready <= 1'b1;
	reg_s_wready  <= 1'b0;
	reg_s_bvalid  <= 1'b0;

     end else begin
     end

   // Detect and process writes to the LEDs register.
   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
	LEDs_register <= 32'h00000000;
     end else if (LEDs_register_hit_w & AXI_S_WREADY & AXI_S_WVALID) begin
	LEDs_register <= AXI_S_WDATA;
     end

   // Detect and process writes to the UserInExp register.
   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
	UserInExp_register <= 32'h00000000;
     end else if (UserInExp_register_hit_w & AXI_S_WREADY & AXI_S_WVALID) begin
	UserInExp_register <= AXI_S_WDATA;
     end

   // Detect and process writes to the UserInIEN register.
   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
	UserInIEN_register <= 32'h00000000;
     end else if (UserInIEN_register_hit_w & AXI_S_WREADY & AXI_S_WVALID) begin
	UserInIEN_register <= AXI_S_WDATA;
     end

   pulse_width led0_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
			.WIDTH(LEDs_register[ 3: 0]), .PULSE(LED0));
   pulse_width led1_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
			.WIDTH(LEDs_register[ 7: 4]), .PULSE(LED1));
   pulse_width led2_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
			.WIDTH(LEDs_register[11: 8]), .PULSE(LED2));
   pulse_width led3_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
			.WIDTH(LEDs_register[15:12]), .PULSE(LED3));
   pulse_width led4_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
			.WIDTH(LEDs_register[19:16]), .PULSE(LED4));
   pulse_width led5_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
			.WIDTH(LEDs_register[23:20]), .PULSE(LED5));
   pulse_width led6_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
			.WIDTH(LEDs_register[27:24]), .PULSE(LED6));
   pulse_width led7_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
			.WIDTH(LEDs_register[31:28]), .PULSE(LED7));

   debounce pb0_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
		    .SIGNAL_IN(PB0), .SIGNAL_OUT(UserIn_register[0]));
   debounce pb1_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
		    .SIGNAL_IN(PB1), .SIGNAL_OUT(UserIn_register[1]));
   debounce pb2_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
		    .SIGNAL_IN(PB2), .SIGNAL_OUT(UserIn_register[2]));
   debounce pb3_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
		    .SIGNAL_IN(PB3), .SIGNAL_OUT(UserIn_register[3]));

   debounce dip_sw0_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
			.SIGNAL_IN(DIP_SW0), .SIGNAL_OUT(UserIn_register[4]));
   debounce dip_sw1_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
			.SIGNAL_IN(DIP_SW1), .SIGNAL_OUT(UserIn_register[5]));
   debounce dip_sw2_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
			.SIGNAL_IN(DIP_SW2), .SIGNAL_OUT(UserIn_register[6]));
   debounce dip_sw3_mod(.CLOCK(AXI_S_ACLK), .RESET(reset_int),
			.SIGNAL_IN(DIP_SW3), .SIGNAL_OUT(UserIn_register[7]));

   assign UserIn_register[31:8] = 24'h0000_00;

   // The UserIn signals generate an interrupt if the actual input
   // differs from the expected input (and the interrupt for that
   // device is enabled.) 
   wire [31:0] UserIn_interrupt = (UserIn_register ^ UserInExp_register) & UserInIEN_register;

   // Combine all the interrupt sources, to generate a single
   // interrupt output.
   assign INTERRUPT = |UserIn_interrupt;

endmodule // SLF_FPGA
