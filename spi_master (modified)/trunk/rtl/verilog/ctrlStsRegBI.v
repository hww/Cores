//////////////////////////////////////////////////////////////////////
////                                                              ////
//// ctrlStsRegBI.v                                               ////
////                                                              ////
//// This file is part of the spiMaster opencores effort.
//// <http://www.opencores.org/cores//>                           ////
////                                                              ////
//// Module Description:                                          ////
//// Wishbone bus interface to spiMaster control and status regs
////                                                              ////
//// To Do:                                                       ////
//// 
////                                                              ////
//// Author(s):                                                   ////
//// - Steve Fielding, sfielding@base2designs.com                 ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
////                                                              ////
//// Copyright (C) 2008 Steve Fielding and OPENCORES.ORG          ////
////                                                              ////
//// This source file may be used and distributed without         ////
//// restriction provided that this copyright statement is not    ////
//// removed from the file and that any derivative work contains  ////
//// the original copyright notice and the associated disclaimer. ////
////                                                              ////
//// This source file is free software; you can redistribute it   ////
//// and/or modify it under the terms of the GNU Lesser General   ////
//// Public License as published by the Free Software Foundation; ////
//// either version 2.1 of the License, or (at your option) any   ////
//// later version.                                               ////
////                                                              ////
//// This source is distributed in the hope that it will be       ////
//// useful, but WITHOUT ANY WARRANTY; without even the implied   ////
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      ////
//// PURPOSE. See the GNU Lesser General Public License for more  ////
//// details.                                                     ////
////                                                              ////
//// You should have received a copy of the GNU Lesser General    ////
//// Public License along with this source; if not, download it   ////
//// from <http://www.opencores.org/lgpl.shtml>                   ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
//
`include "timescale.v"
`include "spiMaster_defines.v"

module ctrlStsRegBI (
  busClk, 
  rstFromWire, 
  dataIn, 
  dataOut, 
  address, 
  writeEn, 
  strobe_i, 
  spiSysClk,
  spiTransType, 
  spiTransCtrl, 
  spiTransStatus,
  spiDirectAccessTxData,
  spiDirectAccessRxData, 
  ctrlStsRegSel, 
  rstSyncToBusClkOut, 
  rstSyncToSpiClkOut,
  SDHC,
  SDWriteError,
  SDReadError,
  SDInitError,
  SDSect,
  spiClkDelay
);

input [7:0] dataIn;
input [7:0] address;
input writeEn;
input strobe_i;
input busClk;
input spiSysClk;
output [7:0] dataOut;
input ctrlStsRegSel;
output [1:0] spiTransType;
output spiTransCtrl;
input spiTransStatus;
output [7:0] spiDirectAccessTxData;
reg [7:0] spiDirectAccessTxData;
input [7:0] spiDirectAccessRxData;
input rstFromWire;
output rstSyncToBusClkOut;
output rstSyncToSpiClkOut;
input SDHC;
input [1:0] SDWriteError;
input [1:0] SDReadError;
input [1:0] SDInitError;
output [31:0] SDSect;
reg [31:0] SDSect;
output [7:0] spiClkDelay;
reg [7:0] spiClkDelay;

wire [7:0] dataIn;
wire [7:0] address;
wire writeEn;
wire strobe_i;
wire clk;
reg [7:0] dataOut;
reg [1:0] spiTransType;
reg spiTransCtrl;
wire ctrlStsRegSel;
wire rstFromWire;
reg rstSyncToBusClkOut;
reg rstSyncToSpiClkOut;

//internal wire and regs
reg [5:0] rstShift;
reg rstFromBus;
reg [7:0] spiDirectAccessTxDataSTB;
reg [7:0] spiDirectAccessRxDataSTB;
reg [1:0] spiTransTypeSTB;
reg spiTransCtrlSTB;
reg spiTransStatusSTB;
reg rstSyncToSpiClkFirst;
reg [5:0] spiTransCtrlShift;
reg spiTransStatusReg1;
reg spiTransStatusReg2;
reg spiTransStatusReg3;
reg [1:0] SDWriteErrorSTB;
reg [1:0] SDReadErrorSTB;
reg [1:0] SDInitErrorSTB;
reg spiTransCtrl_reg1;
reg spiTransCtrl_reg2;
reg spiTransCtrl_reg3;

//sync write demux
always @(posedge busClk)
begin
  if (rstSyncToBusClkOut == 1'b1) begin
    spiTransTypeSTB <= `DIRECT_ACCESS;
    spiTransCtrlSTB <= `TRANS_STOP;
    spiDirectAccessTxDataSTB <= 8'h00;
    spiClkDelay <= `FAST_SPI_CLK;
  end
  else begin
    if (writeEn == 1'b1 && ctrlStsRegSel == 1'b1 && strobe_i == 1'b1 && address == `SPI_MASTER_CONTROL_REG && dataIn[0] == 1'b1 )
      rstFromBus <= 1'b1;
    else
      rstFromBus <= 1'b0;
    if (writeEn == 1'b1 && ctrlStsRegSel == 1'b1 && strobe_i == 1'b1 && address == `TRANS_CTRL_REG && dataIn[0] == 1'b1 )
      spiTransCtrlSTB <= 1'b1;
    else
      spiTransCtrlSTB <= 1'b0;
    if (writeEn == 1'b1 && ctrlStsRegSel == 1'b1 && strobe_i == 1'b1) begin
      case (address)
        `TRANS_TYPE_REG: spiTransTypeSTB <= dataIn[1:0];
        `SD_SECT_7_0_REG: SDSect[7:0] <= dataIn;
        `SD_SECT_15_8_REG: SDSect[15:8] <= dataIn;
        `SD_SECT_23_16_REG: SDSect[23:16] <= dataIn;
        `SD_SECT_31_24_REG: SDSect[31:24] <= dataIn;
        `SPI_CLK_DEL_REG: spiClkDelay <= dataIn;
        `DIRECT_ACCESS_DATA_REG: spiDirectAccessTxDataSTB <= dataIn;
      endcase
    end
  end
end

// async read mux
always @(address or spiTransTypeSTB or spiTransCtrlSTB or
         spiTransStatusSTB or spiDirectAccessRxDataSTB or
         SDSect or SDInitErrorSTB or SDReadErrorSTB or SDWriteErrorSTB or
         spiClkDelay or SDHC)
begin
  case (address)
    `SPI_MASTER_VERSION_REG: dataOut <= `SPI_MASTER_VERSION_NUM;
	`SPI_MASTER_CONTROL_REG: dataOut <= {SDHC,1'b0};
    `TRANS_TYPE_REG: dataOut <= { 6'b000000, spiTransTypeSTB};
    `TRANS_CTRL_REG: dataOut <= { 7'b0000000, spiTransCtrlSTB};
    `TRANS_STS_REG: dataOut <= { 7'b0000000, spiTransStatusSTB};
    `TRANS_ERROR_REG: dataOut <= {2'b00, SDWriteErrorSTB, SDReadErrorSTB, SDInitErrorSTB};
    `SD_SECT_7_0_REG: dataOut <= SDSect[7:0];
    `SD_SECT_15_8_REG: dataOut <= SDSect[15:8];
    `SD_SECT_23_16_REG: dataOut <= SDSect[23:16];
    `SD_SECT_31_24_REG: dataOut <= SDSect[31:24];
    `SPI_CLK_DEL_REG: dataOut <= spiClkDelay;
    `DIRECT_ACCESS_DATA_REG: dataOut <= spiDirectAccessRxDataSTB;
    default: dataOut <= 8'h00;
  endcase
end

// reset control
//generate 'rstSyncToBusClk'
//assuming that 'busClk' < 5 * 'spiSysClk'.
always @(posedge busClk) begin
  if (rstFromWire == 1'b1 || rstFromBus == 1'b1) 
    rstShift <= 6'b111111;
  else
    rstShift <= {1'b0, rstShift[5:1]};
end

always @(rstShift)
  rstSyncToBusClkOut <= rstShift[0];

// double sync across clock domains to generate 'rstSyncToSpiClkOut'
always @(posedge spiSysClk) begin
    rstSyncToSpiClkFirst <= rstSyncToBusClkOut;
    rstSyncToSpiClkOut <= rstSyncToSpiClkFirst;
end


// spi transaction control
//assuming that 'busClk' < 5 * 'spiSysClk'.
always @(posedge busClk) begin
  if (rstSyncToBusClkOut == 1'b1) 
    spiTransCtrlShift <= 6'b000000;
  else if (spiTransCtrlSTB == 1'b1) 
    spiTransCtrlShift <= 6'b111111;
  else
    spiTransCtrlShift <= {1'b0, spiTransCtrlShift[5:1]};
end

//re-sync to spiSysClk
always @(posedge spiSysClk) begin
  if (rstSyncToSpiClkOut == 1'b1) begin
    spiTransCtrl_reg1 <= 1'b0;
    spiTransCtrl_reg2 <= 1'b0;
    spiTransCtrl_reg3 <= 1'b0;
  end
  else begin
    spiTransCtrl_reg1 <= spiTransCtrlShift[0];
    spiTransCtrl_reg2 <= spiTransCtrl_reg1;
    spiTransCtrl_reg3 <= spiTransCtrl_reg2;
    if (spiTransCtrl_reg3 == 1'b0 && spiTransCtrl_reg2 == 1'b1)
      spiTransCtrl <= `TRANS_START;
    else
      spiTransCtrl <= `TRANS_STOP;
  end
end



//re-sync from busClk to spiSysClk. 
always @(posedge spiSysClk) begin
  if (rstSyncToSpiClkOut == 1'b1) begin
    spiTransType <= `DIRECT_ACCESS;
    spiDirectAccessTxData <= 8'h00;
  end
  else begin
    spiDirectAccessTxData <= spiDirectAccessTxDataSTB;
    spiTransType <= spiTransTypeSTB; 
  end
end

//re-sync from spiSysClk to busClk
always @(posedge busClk) begin
  if (rstSyncToBusClkOut == 1'b1) begin
    spiTransStatusSTB <= `TRANS_NOT_BUSY;
    spiTransStatusReg1 <= `TRANS_NOT_BUSY;
    spiTransStatusReg2 <= `TRANS_NOT_BUSY;
    spiTransStatusReg3 <= `TRANS_NOT_BUSY;
  end
  else begin
    spiTransStatusReg1 <= spiTransStatus;
    spiTransStatusReg2 <= spiTransStatusReg1;
    spiTransStatusReg3 <= spiTransStatusReg2;
    if (spiTransCtrlSTB == `TRANS_START)
      spiTransStatusSTB <= `TRANS_BUSY;
    else if (spiTransStatusReg3 == `TRANS_BUSY && spiTransStatusReg2 == `TRANS_NOT_BUSY)
      spiTransStatusSTB <= `TRANS_NOT_BUSY;
  end
  spiDirectAccessRxDataSTB <= spiDirectAccessRxData;
  SDWriteErrorSTB <= SDWriteError;
  SDReadErrorSTB <= SDReadError;
  SDInitErrorSTB <= SDInitError;
end

endmodule
