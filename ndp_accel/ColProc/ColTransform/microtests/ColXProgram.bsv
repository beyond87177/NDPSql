import Vector::*;
import ColXFormPE::*;
import BuildVector::*;
import NDPCommon::*;
import SimdAlu256::*;

////////////////////////////////////////////////////////////////////////////////
/// ColEng Instruction Section
////////////////////////////////////////////////////////////////////////////////
typedef 4 NumColEngs;
////////////////////////////////////////////////////////////////////////////////
/// End of ColEng Instruction Section
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// ColEng Instruction Section
////////////////////////////////////////////////////////////////////////////////

function Tuple2#(Vector#(NumColEngs, Bit#(4)), Vector#(NumColEngs, Vector#(8, Bit#(32)))) genTest();
   Integer numColEngs = valueOf(NumColEngs);
   Bit#(64) most_negative = 1<<63;
   Vector#(NumColEngs, Vector#(8, DecodeInst)) pePrograms = ?;//replicate(peProg);
   Vector#(NumColEngs, Integer) progLength = ?;//replicate(1);
   Integer i = 0;
   
   Vector#(8, DecodeInst) peProg = append(vec(DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Byte, strType: ?, imm: ?}, //rf
                                              DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Byte, strType: ?, imm: ?}, //ls
                                              DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Int, strType: ?, imm: ?}, //quantity
                                              DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Long, strType: ?, imm: ?}, // extended_price
                                              DecodeInst{iType: AluImm, aluOp: Sub, isSigned: True, colType: Long, strType: ?, imm: 100}, // 1 - discount
                                              DecodeInst{iType: AluImm, aluOp: Add, isSigned: True, colType: Long, strType: ?, imm: 100}), // 1 + tax
                                          ?);

   pePrograms[i] = peProg;
   progLength[i] = 6;
   i = i + 1;

   
   peProg = append(vec(DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Byte, strType: ?, imm: ?}, //rf
                       DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Byte, strType: ?, imm: ?}, //ls
                       DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Int, strType: ?, imm: ?}, //quantity
                       DecodeInst{iType: Copy, aluOp: ?, isSigned: ?, colType: Long, strType: Long, imm: ?}, // copy extended_price
                       DecodeInst{iType: Alu,  aluOp: Mullo, isSigned: True, colType: Long, strType: ?, imm: 100}, // 1 - discount * extended_price
                       DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Long, strType: ?, imm: ?}), // pass 1 + tax
                   ?);
   
   pePrograms[i] = peProg;
   progLength[i] = 6;
   i = i + 1;
   
   
   peProg = append(vec(DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Byte, strType: ?, imm: ?}, //rf
                       DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Byte, strType: ?, imm: ?}, //ls
                       DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Int, strType: ?, imm: ?}, //quantity
                       DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Long, strType: ?, imm: ?}, // pass extended_price
                       DecodeInst{iType: Copy, aluOp: ?, isSigned: ?, colType: Long, strType: Long, imm: ?}, // copy 1 - discount * extended_price
                       DecodeInst{iType: Alu,  aluOp: Mullo, isSigned: True, colType: Long, strType: ?, imm: ?}), // (1 + tax) * (1 - discount * extended_price)
                   ?);
   
   pePrograms[i] = peProg;
   progLength[i] = 6;
   i = i + 1;

   peProg = append(vec(DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Byte, strType: ?, imm: ?}, //rf
                       DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Byte, strType: ?, imm: ?}, //ls
                       DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Int, strType: ?, imm: ?}, //quantity
                       DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Long, strType: ?, imm: ?}, // pass extended_price
                       DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Long, strType: ?, imm: ?}, // pass 1 - discount * extended_price
                       DecodeInst{iType: Pass, aluOp: ?, isSigned: ?, colType: Long, strType: ?, imm: ?}), // pass (1 + tax) * (1 - discount * extended_price)
                   ?);
   
   pePrograms[i] = peProg;
   progLength[i] = 6;
   i = i + 1;
   Vector#(NumColEngs, Bit#(4)) a = map(fromInteger, progLength);
   Vector#(NumColEngs, Vector#(8, Bit#(32))) b = unpack(pack(pePrograms));
   return tuple2(a, b);
endfunction

// Vector#(NumColEngs, Bit#(4)) progLength_bit = ?;
// Vector#(NumColEngs, Vector#(8, Bit#(32))) peInsts = ?;

// {progLength_bit, peInsts} = genTest();
////////////////////////////////////////////////////////////////////////////////
/// End of ColEng Instruction Section
////////////////////////////////////////////////////////////////////////////////
