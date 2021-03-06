package URAMCore;
import Assert::*;
import BRAMCore::*;
import List::*;

export URAM_PORT(..);
export URAM_DUAL_PORT(..);
export mkURAMCore2;

interface URAM_PORT#(type addr, type data);
   method Action put(Bool write, addr address, data datain);
   method data read();
endinterface : URAM_PORT

interface URAM_DUAL_PORT#(type addr, type data);
   interface URAM_PORT#(addr, data) a;
   interface URAM_PORT#(addr, data) b;
endinterface : URAM_DUAL_PORT


module mkURAMCore2#(Integer pipeline_depth)(URAM_DUAL_PORT#(addr, data)) provisos(Bits#(addr, addr_width),
                                                                                          Bits#(data, data_width));
   staticAssert(pipeline_depth > 0,"pipeline_depth need to be greater than 1");
   let m_ = ?;
   if (genVerilog()) begin
      m_ <- mkURAMCore2BVI(pipeline_depth);
   end   
   else begin
      m_ <- mkURAMCore2SIM(pipeline_depth);
   end
   return m_;
endmodule

module mkURAMCore2SIM#(Integer pipeline_depth)(URAM_DUAL_PORT#(addr, data)) provisos(Bits#(addr, addr_width),
                                                                                             Bits#(data, data_width));
   
   BRAM_DUAL_PORT#(addr, data) ram <- mkBRAMCore2(valueOf(TExp#(addr_width)), False);
   
   List#(Reg#(data)) delayRegA <- replicateM(pipeline_depth, mkRegU);

   rule moveA;
      delayRegA[0] <= ram.a.read();
      for (Integer i = 0; i < pipeline_depth - 1; i = i + 1) begin
         delayRegA[i+1] <= delayRegA[i];
      end
   endrule
   
   List#(Reg#(data)) delayRegB <- replicateM(pipeline_depth, mkRegU);

   rule moveB;
      delayRegB[0] <= ram.b.read();
      for (Integer i = 0; i < pipeline_depth - 1; i = i + 1) begin
         delayRegB[i+1] <= delayRegB[i];
      end
   endrule

   
   interface URAM_PORT a;
      method Action put(Bool write, addr address, data datain);
         ram.a.put(write, address, datain);
      endmethod
      method data read();
         return last(delayRegA)._read;
      endmethod
   endinterface   
   interface URAM_PORT b;
      method Action put(Bool write, addr address, data datain);
         ram.b.put(write, address, datain);
      endmethod
      method data read();
         return last(delayRegB)._read;
      endmethod
   endinterface
endmodule


import "BVI" URAM2 =
module mkURAMCore2BVI#(Integer pipeline_depth)(URAM_DUAL_PORT#(addr, data)) provisos(Bits#(addr, addr_width),
                                                                                             Bits#(data, data_width));
   default_clock clk(clk);
   default_reset no_reset;
   
   parameter AWIDTH = valueOf(addr_width);
   parameter DWIDTH = valueOf(data_width);
   parameter NBPIPE = pipeline_depth;
   
   interface URAM_PORT a;
      method       put(wea, addra, dina) enable(mem_ena) clocked_by (clk) reset_by(no_reset);
      method douta read clocked_by (clk) reset_by(no_reset);
   endinterface
   
   schedule (a.put ) CF (a.read);
   schedule (a.read) CF (a.read);
   schedule (a.put ) C (a.put);
   
   interface URAM_PORT b;
      method       put(web, addrb, dinb) enable(mem_enb) clocked_by (clk) reset_by(no_reset);
      method doutb read clocked_by (clk) reset_by(no_reset);
   endinterface

   schedule (b.put ) CF (b.read);
   schedule (b.read) CF (b.read);
   schedule (b.put ) C (b.put);

   schedule (a.put ) CF (b.put);
   schedule (a.put) CF (b.read);
   schedule (a.read ) CF (b.put);
   schedule (a.read ) CF (b.read);


endmodule

endpackage: URAMCore
