//===------ PPCDisassembler.cpp - Disassembler for PowerPC ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/PPCMCTargetDesc.h"
#include "TargetInfo/PowerPCTargetInfo.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCFixedLenDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

DEFINE_PPC_REGCLASSES;

#define DEBUG_TYPE "ppc-disassembler"

typedef MCDisassembler::DecodeStatus DecodeStatus;

namespace {
class PPCDisassembler : public MCDisassembler {
  bool IsLittleEndian;

public:
  PPCDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx,
                  bool IsLittleEndian)
      : MCDisassembler(STI, Ctx), IsLittleEndian(IsLittleEndian) {}

  DecodeStatus getInstruction(MCInst &Instr, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t Address,
                              raw_ostream &CStream) const override;
};
} // end anonymous namespace

static MCDisassembler *createPPCDisassembler(const Target &T,
                                             const MCSubtargetInfo &STI,
                                             MCContext &Ctx) {
  return new PPCDisassembler(STI, Ctx, /*IsLittleEndian=*/false);
}

static MCDisassembler *createPPCLEDisassembler(const Target &T,
                                               const MCSubtargetInfo &STI,
                                               MCContext &Ctx) {
  return new PPCDisassembler(STI, Ctx, /*IsLittleEndian=*/true);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializePowerPCDisassembler() {
  // Register the disassembler for each target.
  TargetRegistry::RegisterMCDisassembler(getThePPC32Target(),
                                         createPPCDisassembler);
  TargetRegistry::RegisterMCDisassembler(getThePPC64Target(),
                                         createPPCDisassembler);
  TargetRegistry::RegisterMCDisassembler(getThePPC64LETarget(),
                                         createPPCLEDisassembler);
}

static DecodeStatus decodeCondBrTarget(MCInst &Inst, unsigned Imm,
                                       uint64_t /*Address*/,
                                       const void * /*Decoder*/) {
  Inst.addOperand(MCOperand::createImm(SignExtend32<14>(Imm)));
  return MCDisassembler::Success;
}

static DecodeStatus decodeDirectBrTarget(MCInst &Inst, unsigned Imm,
                                         uint64_t /*Address*/,
                                         const void * /*Decoder*/) {
  int32_t Offset = SignExtend32<24>(Imm);
  Inst.addOperand(MCOperand::createImm(Offset));
  return MCDisassembler::Success;
}

// FIXME: These can be generated by TableGen from the existing register
// encoding values!

template <std::size_t N>
static DecodeStatus decodeRegisterClass(MCInst &Inst, uint64_t RegNo,
                                        const MCPhysReg (&Regs)[N]) {
  assert(RegNo < N && "Invalid register number");
  Inst.addOperand(MCOperand::createReg(Regs[RegNo]));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeCRRCRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return decodeRegisterClass(Inst, RegNo, CRRegs);
}

static DecodeStatus DecodeCRBITRCRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return decodeRegisterClass(Inst, RegNo, CRBITRegs);
}

static DecodeStatus DecodeF4RCRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return decodeRegisterClass(Inst, RegNo, FRegs);
}

static DecodeStatus DecodeF8RCRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return decodeRegisterClass(Inst, RegNo, FRegs);
}

static DecodeStatus DecodeVFRCRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return decodeRegisterClass(Inst, RegNo, VFRegs);
}

static DecodeStatus DecodeVRRCRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return decodeRegisterClass(Inst, RegNo, VRegs);
}

static DecodeStatus DecodeVSRCRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return decodeRegisterClass(Inst, RegNo, VSRegs);
}

static DecodeStatus DecodeVSFRCRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return decodeRegisterClass(Inst, RegNo, VSFRegs);
}

static DecodeStatus DecodeVSSRCRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return decodeRegisterClass(Inst, RegNo, VSSRegs);
}

static DecodeStatus DecodeGPRCRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return decodeRegisterClass(Inst, RegNo, RRegs);
}

static DecodeStatus DecodeGPRC_NOR0RegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return decodeRegisterClass(Inst, RegNo, RRegsNoR0);
}

static DecodeStatus DecodeG8RCRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return decodeRegisterClass(Inst, RegNo, XRegs);
}

static DecodeStatus DecodeG8RC_NOX0RegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return decodeRegisterClass(Inst, RegNo, XRegsNoX0);
}

#define DecodePointerLikeRegClass0 DecodeGPRCRegisterClass
#define DecodePointerLikeRegClass1 DecodeGPRC_NOR0RegisterClass

static DecodeStatus DecodeSPERCRegisterClass(MCInst &Inst, uint64_t RegNo,
                                            uint64_t Address,
                                            const void *Decoder) {
  return decodeRegisterClass(Inst, RegNo, SPERegs);
}

#define DecodeQSRCRegisterClass DecodeQFRCRegisterClass
#define DecodeQBRCRegisterClass DecodeQFRCRegisterClass

template<unsigned N>
static DecodeStatus decodeUImmOperand(MCInst &Inst, uint64_t Imm,
                                      int64_t Address, const void *Decoder) {
  assert(isUInt<N>(Imm) && "Invalid immediate");
  Inst.addOperand(MCOperand::createImm(Imm));
  return MCDisassembler::Success;
}

template<unsigned N>
static DecodeStatus decodeSImmOperand(MCInst &Inst, uint64_t Imm,
                                      int64_t Address, const void *Decoder) {
  assert(isUInt<N>(Imm) && "Invalid immediate");
  Inst.addOperand(MCOperand::createImm(SignExtend64<N>(Imm)));
  return MCDisassembler::Success;
}

static DecodeStatus decodeImmZeroOperand(MCInst &Inst, uint64_t Imm,
                                         int64_t Address, const void *Decoder) {
  if (Imm != 0)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createImm(Imm));
  return MCDisassembler::Success;
}

static DecodeStatus decodeMemRIOperands(MCInst &Inst, uint64_t Imm,
                                        int64_t Address, const void *Decoder) {
  // Decode the memri field (imm, reg), which has the low 16-bits as the
  // displacement and the next 5 bits as the register #.

  uint64_t Base = Imm >> 16;
  uint64_t Disp = Imm & 0xFFFF;

  assert(Base < 32 && "Invalid base register");

  switch (Inst.getOpcode()) {
  default: break;
  case PPC::LBZU:
  case PPC::LHAU:
  case PPC::LHZU:
  case PPC::LWZU:
  case PPC::LFSU:
  case PPC::LFDU:
    // Add the tied output operand.
    Inst.addOperand(MCOperand::createReg(RRegsNoR0[Base]));
    break;
  case PPC::STBU:
  case PPC::STHU:
  case PPC::STWU:
  case PPC::STFSU:
  case PPC::STFDU:
    Inst.insert(Inst.begin(), MCOperand::createReg(RRegsNoR0[Base]));
    break;
  }

  Inst.addOperand(MCOperand::createImm(SignExtend64<16>(Disp)));
  Inst.addOperand(MCOperand::createReg(RRegsNoR0[Base]));
  return MCDisassembler::Success;
}

static DecodeStatus decodeMemRIXOperands(MCInst &Inst, uint64_t Imm,
                                         int64_t Address, const void *Decoder) {
  // Decode the memrix field (imm, reg), which has the low 14-bits as the
  // displacement and the next 5 bits as the register #.

  uint64_t Base = Imm >> 14;
  uint64_t Disp = Imm & 0x3FFF;

  assert(Base < 32 && "Invalid base register");

  if (Inst.getOpcode() == PPC::LDU)
    // Add the tied output operand.
    Inst.addOperand(MCOperand::createReg(RRegsNoR0[Base]));
  else if (Inst.getOpcode() == PPC::STDU)
    Inst.insert(Inst.begin(), MCOperand::createReg(RRegsNoR0[Base]));

  Inst.addOperand(MCOperand::createImm(SignExtend64<16>(Disp << 2)));
  Inst.addOperand(MCOperand::createReg(RRegsNoR0[Base]));
  return MCDisassembler::Success;
}

static DecodeStatus decodeMemRIX16Operands(MCInst &Inst, uint64_t Imm,
                                         int64_t Address, const void *Decoder) {
  // Decode the memrix16 field (imm, reg), which has the low 12-bits as the
  // displacement with 16-byte aligned, and the next 5 bits as the register #.

  uint64_t Base = Imm >> 12;
  uint64_t Disp = Imm & 0xFFF;

  assert(Base < 32 && "Invalid base register");

  Inst.addOperand(MCOperand::createImm(SignExtend64<16>(Disp << 4)));
  Inst.addOperand(MCOperand::createReg(RRegsNoR0[Base]));
  return MCDisassembler::Success;
}

static DecodeStatus decodeMemRI34PCRelOperands(MCInst &Inst, uint64_t Imm,
                                               int64_t Address,
                                               const void *Decoder) {
  // Decode the memri34_pcrel field (imm, reg), which has the low 34-bits as the
  // displacement, and the next 5 bits as an immediate 0.
  uint64_t Base = Imm >> 34;
  uint64_t Disp = Imm & 0x3FFFFFFFFUL;

  assert(Base < 32 && "Invalid base register");

  Inst.addOperand(MCOperand::createImm(SignExtend64<34>(Disp)));
  return decodeImmZeroOperand(Inst, Base, Address, Decoder);
}

static DecodeStatus decodeMemRI34Operands(MCInst &Inst, uint64_t Imm,
                                          int64_t Address,
                                          const void *Decoder) {
  // Decode the memri34 field (imm, reg), which has the low 34-bits as the
  // displacement, and the next 5 bits as the register #.
  uint64_t Base = Imm >> 34;
  uint64_t Disp = Imm & 0x3FFFFFFFFUL;

  assert(Base < 32 && "Invalid base register");

  Inst.addOperand(MCOperand::createImm(SignExtend64<34>(Disp)));
  Inst.addOperand(MCOperand::createReg(RRegsNoR0[Base]));
  return MCDisassembler::Success;
}

static DecodeStatus decodeSPE8Operands(MCInst &Inst, uint64_t Imm,
                                         int64_t Address, const void *Decoder) {
  // Decode the spe8disp field (imm, reg), which has the low 5-bits as the
  // displacement with 8-byte aligned, and the next 5 bits as the register #.

  uint64_t Base = Imm >> 5;
  uint64_t Disp = Imm & 0x1F;

  assert(Base < 32 && "Invalid base register");

  Inst.addOperand(MCOperand::createImm(Disp << 3));
  Inst.addOperand(MCOperand::createReg(RRegsNoR0[Base]));
  return MCDisassembler::Success;
}

static DecodeStatus decodeSPE4Operands(MCInst &Inst, uint64_t Imm,
                                         int64_t Address, const void *Decoder) {
  // Decode the spe4disp field (imm, reg), which has the low 5-bits as the
  // displacement with 4-byte aligned, and the next 5 bits as the register #.

  uint64_t Base = Imm >> 5;
  uint64_t Disp = Imm & 0x1F;

  assert(Base < 32 && "Invalid base register");

  Inst.addOperand(MCOperand::createImm(Disp << 2));
  Inst.addOperand(MCOperand::createReg(RRegsNoR0[Base]));
  return MCDisassembler::Success;
}

static DecodeStatus decodeSPE2Operands(MCInst &Inst, uint64_t Imm,
                                         int64_t Address, const void *Decoder) {
  // Decode the spe2disp field (imm, reg), which has the low 5-bits as the
  // displacement with 2-byte aligned, and the next 5 bits as the register #.

  uint64_t Base = Imm >> 5;
  uint64_t Disp = Imm & 0x1F;

  assert(Base < 32 && "Invalid base register");

  Inst.addOperand(MCOperand::createImm(Disp << 1));
  Inst.addOperand(MCOperand::createReg(RRegsNoR0[Base]));
  return MCDisassembler::Success;
}

static DecodeStatus decodeCRBitMOperand(MCInst &Inst, uint64_t Imm,
                                        int64_t Address, const void *Decoder) {
  // The cr bit encoding is 0x80 >> cr_reg_num.

  unsigned Zeros = countTrailingZeros(Imm);
  assert(Zeros < 8 && "Invalid CR bit value");

  Inst.addOperand(MCOperand::createReg(CRRegs[7 - Zeros]));
  return MCDisassembler::Success;
}

#include "PPCGenDisassemblerTables.inc"

DecodeStatus PPCDisassembler::getInstruction(MCInst &MI, uint64_t &Size,
                                             ArrayRef<uint8_t> Bytes,
                                             uint64_t Address,
                                             raw_ostream &CS) const {
  auto *ReadFunc = IsLittleEndian ? support::endian::read32le
                                  : support::endian::read32be;

  // If this is an 8-byte prefixed instruction, handle it here.
  // Note: prefixed instructions aren't technically 8-byte entities - the prefix
  //       appears in memory at an address 4 bytes prior to that of the base
  //       instruction regardless of endianness. So we read the two pieces and
  //       rebuild the 8-byte instruction.
  // TODO: In this function we call decodeInstruction several times with
  //       different decoder tables. It may be possible to only call once by
  //       looking at the top 6 bits of the instruction.
  if (STI.getFeatureBits()[PPC::FeaturePrefixInstrs] && Bytes.size() >= 8) {
    uint32_t Prefix = ReadFunc(Bytes.data());
    uint32_t BaseInst = ReadFunc(Bytes.data() + 4);
    uint64_t Inst = BaseInst | (uint64_t)Prefix << 32;
    DecodeStatus result = decodeInstruction(DecoderTable64, MI, Inst, Address,
                                            this, STI);
    if (result != MCDisassembler::Fail) {
      Size = 8;
      return result;
    }
  }

  // Get the four bytes of the instruction.
  Size = 4;
  if (Bytes.size() < 4) {
    Size = 0;
    return MCDisassembler::Fail;
  }

  // Read the instruction in the proper endianness.
  uint64_t Inst = ReadFunc(Bytes.data());

  if (STI.getFeatureBits()[PPC::FeatureSPE]) {
    DecodeStatus result =
        decodeInstruction(DecoderTableSPE32, MI, Inst, Address, this, STI);
    if (result != MCDisassembler::Fail)
      return result;
  }

  return decodeInstruction(DecoderTable32, MI, Inst, Address, this, STI);
}
