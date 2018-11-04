// ============================================================================
//        __
//   \\__/ o\    (C) 2018  Robert Finch, Waterloo
//    \  __ /    All rights reserved.
//     \/_//     robfinch<remove>@finitron.ca
//       ||
//
//	FT64_ipt.v
//  - 64 bit CPU inverted page table memory management unit
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
`ifndef TRUE
`define TRUE    1'b1
`define FALSE   1'b0
`endif

module FT64_ipt(rst, clk, ol_i, cti_i, cs_i, icl_i, cyc_i, stb_i, ack_o, we_i, sel_i, vadr_i, dat_i, dat_o,
	cyc_o, ack_i, we_o, padr_o, exv_o, rdv_o, wrv_o, page_fault);
input rst;
input clk;
input [1:0] ol_i;
input [2:0] cti_i;
input cs_i;
input icl_i;
input cyc_i;
input stb_i;
output reg ack_o;
input we_i;
input [7:0] sel_i;
input [63:0] vadr_i;
input [63:0] dat_i;
output reg [63:0] dat_o;
output reg cyc_o;
input ack_i;
output reg we_o;
output reg [31:0] padr_o;
output reg exv_o;
output reg rdv_o;
output reg wrv_o;
output reg page_fault;

parameter S_IDLE = 4'd0;
parameter S_CMP1 = 4'd1;
parameter S_CMP2 = 4'd2;
parameter S_CMP3 = 4'd3;
parameter S_CMP4 = 4'd4;
parameter S_CMP5 = 4'd5;
parameter S_CMP6 = 4'd6;
parameter S_WAIT1 = 4'd7;
parameter S_ACK = 4'd8;

reg [3:0] state;
reg [15:0] pt_ad;
reg upd;
reg upd_done;
reg probe, probe_done;
reg pte_last;
reg [7:0] pte_asid;
reg [3:0] pte_drwx;
reg [18:0] pte_vadr;
reg pt_wr;
reg [31:0] pt_dati;
wire [31:0] pt_dat;

FT64_iptram uram1 (
  .clka(clk),
  .ena(1'b1),
  .wea(pt_wr),
  .addra(pt_ad),
  .dina(pt_dati),
  .douta(pt_dat)
);

wire pte_last = pt_dat[23];
wire [18:0] pt_vadr = pt_dat[22:4];
wire [7:0] pt_asid = pt_dat[31:24];
wire [3:0] pt_drwx = pt_dat[3:0];

function [15:0] Hash1;
input [39:0] vadr;
begin
	Hash1 = {1'b0,vadr[37:32],vadr[21:13]};
end
endfunction

function [15:0] Hash2;
input [39:0] vadr;
begin
	Hash2 = {1'b1,vadr[37:32],vadr[21:13]};
end
endfunction

always @(posedge clk)
	case(vadr_i[5:3])
	3'd1:
		dat_o <= pt_ad;
	3'd2:
		begin
			dat_o[31:24] <= pte_asid;
			dat_o[23] <= pte_last;
			dat_o[2:0] <= pte_drwx[2:0];
			dat_o[7] <= pte_drwx[3];
		end
	3'd3:
		dat_o <= pte_vadr;
	default:	dat_o <= 1'b0;
	endcase

always @(posedge clk)
if (rst) begin
	cyc_o <= 1'b0;
	padr_o <= 32'hFFFC0100;
	ack_o <= 1'b0;
	exv_o <= 1'b0;
	rdv_o <= 1'b0;
	wrv_o <= 1'b0;
	pt_wr <= 1'b0;
	upd <= 1'b0;
	probe <= 1'b0;
	upd_done <= 1'b0;
	probe_done <= 1'b0;
	goto(S_IDLE);
end
else begin
	pt_wr <= 1'b0;
	page_fault <= 1'b0;
	ack_o <= 1'b0;
case(state)
S_IDLE:
	if (cyc_i) begin
		if (cs_i) begin
			ack_o <= 1'b1;
			case(vadr_i[5:3])
			3'd0:
				begin
					if (dat_i[0] & !upd_done) begin
						pt_ad <= Hash1({pte_asid,pte_vadr});
						upd <= 1'b1;
						goto(S_CMP1);
					end
					else if (dat_i[1] & !probe_done) begin
						pt_ad <= Hash1({pte_asid,pte_vadr});
						probe <= 1'b1;
						goto(S_CMP1);
					end
				end
			3'd2:	
				begin
					pte_asid <= dat_i[31:24];
					pte_last <= dat_i[22];
					pte_drwx <= {dat_i[7],dat_i[2:0]};
				end
			3'd3:
				begin
					pte_vadr <= dat_i[18:0];
				end
			endcase
		end
		else begin
			upd_done <= 1'b0;
			probe_done <= 1'b0;
			upd <= 1'b0;
			probe <= 1'b0;
			if (ol_i==2'b0) begin
				cyc_o <= 1'b1;
				we_o <= we_i;
				padr_o <= vadr_i;
				goto(S_ACK);
			end
			else begin
				pt_ad <= Hash1({vadr_i[63:56],vadr_i});
				goto(S_CMP1);
			end
		end
	end
	else begin
		exv_o <= 1'b0;
		rdv_o <= 1'b0;
		wrv_o <= 1'b0;
	end

S_CMP1:
	goto(S_CMP2);
S_CMP2:
	goto(S_CMP3);
S_CMP3:
	if (pt_drwx[2:0]==3'b0) begin
		if (upd) begin
			pt_wr <= 1'b1;
			pt_dati <= {pte_asid,pte_last,pte_vadr[18:0],pte_drwx};
			upd_done <= 1'b1;
			goto(S_IDLE);
		end
		else if (probe) begin
			pte_drwx <= 3'b0;
			pte_vadr <= 19'b0;
			pte_asid <= 8'b0;
			pte_last <= 1'b0;
			probe_done <= 1'b1;
			goto(S_IDLE);
		end
		else begin
			page_fault <= 1'b1;
			goto(S_WAIT1);
		end
	end
	else if (pt_asid==vadr_i[63:56] && pt_vadr==vadr_i[31:13]) begin
		if (upd) begin
			pt_wr <= 1'b1;
			pt_dati <= {pt_dat[31:4],pte_drwx};
			upd_done <= 1'b1;
			goto(S_IDLE);
		end
		else if (probe) begin
			pte_last <= pt_last;
			pte_drwx <= pt_drwx;
			probe_done <= 1'b1;
			goto(S_IDLE);
		end
		else if (~ack_i) begin
			cyc_o <= 1'b1;
			we_o <= we_i & pt_drwx[1];
			if (!pt_drwx[1] & we_i)	wrv_o <= 1'b1;
			if (!pt_drwx[2] & ~we_i) rdv_o <= 1'b1;
			if (!pt_drwx[0] & icl_i) exv_o <= 1'b1;
			padr_o <= {pt_ad,vadr_i[12:0]};
			goto(S_ACK);
		end	
	end
	else begin
		if (upd|probe)
			pt_ad <= Hash2({pte_asid,pte_vadr});
		else
			pt_ad <= Hash2({vadr_i[63:56],vadr_i});
		goto(S_CMP4);
	end

S_CMP4:
	goto(S_CMP5);
S_CMP5:
	goto(S_CMP6);
S_CMP6:
	if (pt_drwx[2:0]==3'b0) begin
		if (upd) begin
			pt_wr <= 1'b1;
			pt_dati <= {pte_asid,pte_last,pte_vadr[18:0],pte_drwx};
			upd_done <= 1'b1;
			goto(S_IDLE);
		end
		else if (probe) begin
			pte_drwx <= 43'b0;
			pte_vadr <= 19'b0;
			pte_asid <= 8'b0;
			pte_last <= 1'b0;
			probe_done <= 1'b1;
			goto(S_IDLE);
		end
		else begin
			page_fault <= 1'b1;
			goto(S_WAIT1);
		end
	end
	else if (pt_asid==vadr_i[63:56] && pt_vadr==vadr_i[31:13]) begin
		if (upd) begin
			pt_wr <= 1'b1;
			pt_dati <= {pt_dat[31:4],pte_drwx};
			upd_done <= 1'b1;
			goto(S_IDLE);
		end
		else if (probe) begin
			pte_last <= pt_last;
			pte_drwx <= pt_drwx;
			probe_done <= 1'b1;
			goto(S_IDLE);
		end
		else if (~ack_i) begin
			cyc_o <= 1'b1;
			we_o <= we_i & pt_drwx[1];
			if (!pt_drwx[1] & we_i)	wrv_o <= 1'b1;
			if (!pt_drwx[2] & ~we_i) rdv_o <= 1'b1;
			if (!pt_drwx[0] & icl_i) exv_o <= 1'b1;
			padr_o <= {pt_ad,vadr_i[12:0]};
			goto(S_ACK);
		end
	end
	else begin
		pt_ad <= {pt_ad+8'd65};
		goto(S_CMP4);
	end

// Wait a clock cycle for a page fault to register.
S_WAIT1:
	goto(S_IDLE);
	
S_ACK:
	if (ack_i) begin
		if (cti_i==3'b000 || cti_i==3'b111) begin
			cyc_o <= 1'b0;
			we_o <= 1'b0;
			goto(S_WAIT1);
		end
	end

endcase	
end

task goto;
input [3:0] nst;
begin
	state <= nst;
end
endtask

endmodule

