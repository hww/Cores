// ============================================================================
//        __
//   \\__/ o\    (C) 2012-2017  Robert Finch, Waterloo
//    \  __ /    All rights reserved.
//     \/_//     robfinch<remove>@opencores.org
//       ||
//
//
// This source file is free software: you can redistribute it and/or modify 
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or     
// (at your option) any later version.                                      
//                                                                          
// This source file is distributed in the hope that it will be useful,      
// but WITHOUT ANY WARRANTY; without even the implied warranty of           
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            
// GNU General Public License for more details.                             
//                                                                          
// You should have received a copy of the GNU General Public License        
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    
//                                                                          
// ============================================================================
//
module scratchmem(rst_i, clk_i, cs_i, cyc_i, stb_i, ack_o, we_i, sel_i, adr_i, dat_i, dat_o);
input rst_i;
input clk_i;
input cs_i;
input cyc_i;
input stb_i;
output ack_o;
input we_i;
input [7:0] sel_i;
input [14:0] adr_i;
input [63:0] dat_i;
output [63:0] dat_o;
reg [63:0] dat_o;

integer n;

reg [7:0] smemA [4095:0];
reg [7:0] smemB [4095:0];
reg [7:0] smemC [4095:0];
reg [7:0] smemD [4095:0];
reg [7:0] smemE [4095:0];
reg [7:0] smemF [4095:0];
reg [7:0] smemG [4095:0];
reg [7:0] smemH [4095:0];
reg [14:2] radr;


initial begin
for (n = 0; n < 4096; n = n + 1)
begin
	smemA[n] = 0;
	smemB[n] = 0;
	smemC[n] = 0;
	smemD[n] = 0;
	smemE[n] = 0;
	smemF[n] = 0;
	smemG[n] = 0;
	smemH[n] = 0;
end
end

wire cs = cs_i && cyc_i && stb_i;

reg rdy,rdy1;
always @(posedge clk_i)
begin
	rdy1 <= cs;
	rdy <= rdy1 & cs && adr_i[4:3]!=2'b11;
end
assign ack_o = cs ? (we_i ? 1'b1 : rdy) : 1'b0;


always @(posedge clk_i)
	if (cs & we_i)
		$display ("wrote to scratchmem: %h=%h", adr_i, dat_i);
always @(posedge clk_i)
if (cs & we_i & sel_i[0])
	smemA[adr_i[14:3]] <= dat_i[7:0];
always @(posedge clk_i)
if (cs & we_i & sel_i[1])
	smemB[adr_i[14:3]] <= dat_i[15:8];
always @(posedge clk_i)
    if (cs & we_i & sel_i[2])
        smemC[adr_i[14:3]] <= dat_i[23:16];
always @(posedge clk_i)
    if (cs & we_i & sel_i[3])
        smemD[adr_i[14:3]] <= dat_i[31:24];
always @(posedge clk_i)
    if (cs & we_i & sel_i[4])
        smemE[adr_i[14:3]] <= dat_i[39:32];
always @(posedge clk_i)
    if (cs & we_i & sel_i[5])
        smemF[adr_i[14:3]] <= dat_i[47:40];
always @(posedge clk_i)
    if (cs & we_i & sel_i[6])
        smemG[adr_i[14:3]] <= dat_i[55:48];
always @(posedge clk_i)
    if (cs & we_i & sel_i[7])
        smemH[adr_i[14:3]] <= dat_i[63:56];

wire pe_cs;
edge_det u1(.rst(rst_i), .clk(clk_i), .ce(1'b1), .i(cs), .pe(pe_cs), .ne(), .ee() );

reg [12:0] ctr;
always @(posedge clk_i)
	if (pe_cs)
		ctr <= adr_i[14:3] + 12'd1;
	else if (cs && ctr[1:0]!=2'b11)
		ctr <= ctr + 12'd1;

always @(posedge clk_i)
	radr <= pe_cs ? adr_i[14:3] : ctr;

//assign dat_o = cs ? {smemH[radr],smemG[radr],smemF[radr],smemE[radr],
//				smemD[radr],smemC[radr],smemB[radr],smemA[radr]} : 64'd0;

always @(posedge clk_i)
if (cs) begin
	dat_o <= {smemH[radr],smemG[radr],smemF[radr],smemE[radr],smemD[radr],smemC[radr],smemB[radr],smemA[radr]};
	if (!we_i)
		$display("read from scratchmem: %h=%h", radr, {smemB[radr],smemA[radr]});
end
else
	dat_o <= 64'd0;

endmodule
