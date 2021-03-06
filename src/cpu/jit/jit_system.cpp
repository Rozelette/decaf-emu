#include <cassert>
#include "jit_insreg.h"
#include "utils/bitutils.h"
#include "utils/log.h"

namespace cpu
{

namespace jit
{

static SprEncoding
decodeSPR(Instruction instr)
{
   return static_cast<SprEncoding>(((instr.spr << 5) & 0x3E0) | ((instr.spr >> 5) & 0x1F));
}

// Enforce In-Order Execution of I/O
static bool
eieio(PPCEmuAssembler& a, Instruction instr)
{
   return true;
}

// Synchronise
static bool
sync(PPCEmuAssembler& a, Instruction instr)
{
   return true;
}

// Instruction Synchronise
static bool
isync(PPCEmuAssembler& a, Instruction instr)
{
   return true;
}

// Move from Special Purpose Register
static bool
mfspr(PPCEmuAssembler& a, Instruction instr)
{
   auto spr = decodeSPR(instr);
   switch (spr) {
   case SprEncoding::XER:
      a.mov(a.eax, a.ppcxer);
      break;
   case SprEncoding::LR:
      a.mov(a.eax, a.ppclr);
      break;
   case SprEncoding::CTR:
      a.mov(a.eax, a.ppcctr);
      break;
   case SprEncoding::UGQR0:
      a.mov(a.eax, a.ppcgpr[0]);
      break;
   case SprEncoding::UGQR1:
      a.mov(a.eax, a.ppcgpr[1]);
      break;
   case SprEncoding::UGQR2:
      a.mov(a.eax, a.ppcgpr[2]);
      break;
   case SprEncoding::UGQR3:
      a.mov(a.eax, a.ppcgpr[3]);
      break;
   case SprEncoding::UGQR4:
      a.mov(a.eax, a.ppcgpr[4]);
      break;
   case SprEncoding::UGQR5:
      a.mov(a.eax, a.ppcgpr[5]);
      break;
   case SprEncoding::UGQR6:
      a.mov(a.eax, a.ppcgpr[6]);
      break;
   case SprEncoding::UGQR7:
      a.mov(a.eax, a.ppcgpr[7]);
      break;
   default:
      gLog->error("Invalid mfspr SPR {}", static_cast<uint32_t>(spr));
   }

   a.mov(a.ppcgpr[instr.rD], a.eax);
   return true;
}

// Move to Special Purpose Register
static bool
mtspr(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rD]);

   auto spr = decodeSPR(instr);
   switch (spr) {
   case SprEncoding::XER:
      a.mov(a.ppcxer, a.eax);
      break;
   case SprEncoding::LR:
      a.mov(a.ppclr, a.eax);
      break;
   case SprEncoding::CTR:
      a.mov(a.ppcctr, a.eax);
      break;
   case SprEncoding::UGQR0:
      a.mov(a.ppcgqr[0], a.eax);
      break;
   case SprEncoding::UGQR1:
      a.mov(a.ppcgqr[1], a.eax);
      break;
   case SprEncoding::UGQR2:
      a.mov(a.ppcgqr[2], a.eax);
      break;
   case SprEncoding::UGQR3:
      a.mov(a.ppcgqr[3], a.eax);
      break;
   case SprEncoding::UGQR4:
      a.mov(a.ppcgqr[4], a.eax);
      break;
   case SprEncoding::UGQR5:
      a.mov(a.ppcgqr[5], a.eax);
      break;
   case SprEncoding::UGQR6:
      a.mov(a.ppcgqr[6], a.eax);
      break;
   case SprEncoding::UGQR7:
      a.mov(a.ppcgqr[7], a.eax);
      break;
   default:
      gLog->error("Invalid mtspr SPR {}", static_cast<uint32_t>(spr));
   }

   return true;
}

// Kernel call
static bool
kc(PPCEmuAssembler& a, Instruction instr)
{
   auto id = instr.kcn;

   auto kc = cpu::getKernelCall(id);
   if (!kc) {
      gLog->error("Encountered invalid Kernel Call ID {}", id);
      a.int3();
      return false;
   }

   a.mov(a.zcx, a.state);
   a.mov(a.zdx, asmjit::Ptr(kc->second));
   a.call(asmjit::Ptr(kc->first));
   return true;
}

void
registerSystemInstructions()
{
   RegisterInstructionFallback(dcbf);
   RegisterInstructionFallback(dcbi);
   RegisterInstructionFallback(dcbst);
   RegisterInstructionFallback(dcbt);
   RegisterInstructionFallback(dcbtst);
   RegisterInstructionFallback(dcbz);
   RegisterInstructionFallback(dcbz_l);
   RegisterInstruction(eieio);
   RegisterInstruction(isync);
   RegisterInstruction(sync);
   RegisterInstruction(mfspr);
   RegisterInstruction(mtspr);
   RegisterInstructionFallback(mftb);
   RegisterInstructionFallback(mfmsr);
   RegisterInstructionFallback(mtmsr);
   RegisterInstructionFallback(mfsr);
   RegisterInstructionFallback(mfsrin);
   RegisterInstructionFallback(mtsr);
   RegisterInstructionFallback(mtsrin);
   RegisterInstruction(kc);
}

} // namespace jit

} // namespace cpu
