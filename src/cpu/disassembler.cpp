#include <sstream>
#include <iomanip>
#include <list>
#include "disassembler.h"
#include "instructiondata.h"
#include "system.h"
#include "kernelfunction.h"
#include "utils/bitutils.h"

Disassembler
gDisassembler;

// TODO: Finish disassembler!

static Disassembly::Argument
disassembleField(uint32_t cia, Instruction instr, InstructionData *data, Field field)
{
   Disassembly::Argument result;

   switch (field) {
   case Field::aa:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.aa;
      break;
   case Field::bd:
      result.type = Disassembly::Argument::Address;
      result.address = sign_extend<16>(instr.bd << 2) + (instr.aa ? 0 : cia);
      break;
   case Field::bi:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.bi;
      break;
   case Field::bo:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.bo;
      break;
   case Field::crbA:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.crbA;
      break;
   case Field::crbB:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.crbB;
      break;
   case Field::crbD:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.crbD;
      break;
   case Field::crfD:
      result.type = Disassembly::Argument::Register;
      result.text = "crf" + std::to_string(instr.crfD);
      break;
   case Field::crfS:
      result.type = Disassembly::Argument::Register;
      result.text = "crf" + std::to_string(instr.crfS);
      break;
   case Field::crm:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.crm;
      break;
   case Field::d:
      result.type = Disassembly::Argument::ValueSigned;
      result.valueSigned = sign_extend<16>(instr.d);
      break;
   case Field::fm:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.fm;
      break;
   case Field::frA:
      result.type = Disassembly::Argument::Register;
      result.text = "f" + std::to_string(instr.frA);
      break;
   case Field::frB:
      result.type = Disassembly::Argument::Register;
      result.text = "f" + std::to_string(instr.frB);
      break;
   case Field::frC:
      result.type = Disassembly::Argument::Register;
      result.text = "f" + std::to_string(instr.frC);
      break;
   case Field::frD:
      result.type = Disassembly::Argument::Register;
      result.text = "f" + std::to_string(instr.frD);
      break;
   case Field::frS:
      result.type = Disassembly::Argument::Register;
      result.text = "f" + std::to_string(instr.frS);
      break;
   case Field::i:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.i;
      break;
   case Field::imm:
      result.type = Disassembly::Argument::ValueUnsigned;
      result.valueUnsigned = instr.imm;
      break;
   case Field::kcn:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.kcn;
      break;
   case Field::li:
      result.type = Disassembly::Argument::Address;
      result.address = sign_extend<26>(instr.li << 2) + (instr.aa ? 0 : cia);
      break;
   case Field::lk:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.lk;
      break;
   case Field::mb:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.mb;
      break;
   case Field::me:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.me;
      break;
   case Field::nb:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.nb;
      break;
   case Field::oe:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.oe;
      break;
   case Field::qd:
      result.type = Disassembly::Argument::ValueSigned;
      result.valueSigned = sign_extend<12>(instr.qd);
      break;
   case Field::rA:
      result.type = Disassembly::Argument::Register;
      result.text = "r" + std::to_string(instr.rA);
      break;
   case Field::rB:
      result.type = Disassembly::Argument::Register;
      result.text = "r" + std::to_string(instr.rB);
      break;
   case Field::rc:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.rc;
      break;
   case Field::rD:
      result.type = Disassembly::Argument::Register;
      result.text = "r" + std::to_string(instr.rD);
      break;
   case Field::rS:
      result.type = Disassembly::Argument::Register;
      result.text = "r" + std::to_string(instr.rS);
      break;
   case Field::sh:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.sh;
      break;
   case Field::simm:
      result.type = Disassembly::Argument::ValueSigned;
      result.valueSigned = sign_extend<16>(instr.simm);
      break;
   case Field::sr:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.sr;
      break;
   case Field::spr: // TODO: Real SPR name
      result.type = Disassembly::Argument::Register;
      result.text = "spr" + std::to_string(((instr.spr << 5) & 0x3E0) | ((instr.spr >> 5) & 0x1F));
      break;
   case Field::to:
      result.type = Disassembly::Argument::ConstantUnsigned;
      result.constantUnsigned = instr.to;
      break;
   case Field::tbr:
      result.type = Disassembly::Argument::Register;
      result.text = "tbr" + std::to_string(instr.spr);
      break;
   case Field::uimm:
      result.type = Disassembly::Argument::ValueUnsigned;
      result.valueUnsigned = instr.uimm;
      break;
   case Field::opcd:
   case Field::xo1:
   case Field::xo2:
   case Field::xo3:
   case Field::xo4:
   default:
      result.type = Disassembly::Argument::Invalid;
      break;
   }

   return result;
}

static std::string
argumentToText(Disassembly::Argument &arg)
{
   std::stringstream ss;

   switch (arg.type) {
   case Disassembly::Argument::Address:
      ss << "@" << std::setfill('0') << std::setw(8) << std::hex << std::uppercase << arg.address;
      return ss.str();
   case Disassembly::Argument::Register:
      return arg.text;
   case Disassembly::Argument::ValueUnsigned:
      if (arg.valueUnsigned > 9) {
         ss << "0x" << std::hex << arg.valueUnsigned;
      } else {
         ss << arg.valueUnsigned;
      }
      return ss.str();
   case Disassembly::Argument::ValueSigned:
      if (arg.valueSigned < -9) {
         ss << "-0x" << std::hex << -arg.valueSigned;
      } else if (arg.valueSigned > 9) {
         ss << "0x" << std::hex << arg.valueSigned;
      } else {
         ss << arg.valueSigned;
      }
      return ss.str();
   case Disassembly::Argument::ConstantUnsigned:
      return std::to_string(arg.constantUnsigned);
   case Disassembly::Argument::ConstantSigned:
      return std::to_string(arg.constantSigned);
   case Disassembly::Argument::Invalid:
      return std::string("???");
   }

   return std::string();
}

bool
Disassembler::disassemble(Instruction instr, Disassembly &dis, uint32_t address)
{
   auto data = gInstructionTable.decode(instr);

   if (!data) {
      return false;
   }

   InstructionAlias *alias = gInstructionTable.findAlias(data, instr);

   dis.name = data->name;
   dis.instruction = data;
   dis.address = address;

   std::list<Field> args;

   for (auto &field : data->write) {
      // Skip arguments that are in read list as well
      if (std::find(data->read.begin(), data->read.end(), field) != data->read.end()) {
         continue;
      }

      // Add only unique arguements
      if (std::find(args.begin(), args.end(), field) != args.end()) {
         continue;
      }

      // Ignore trace only fields for disassembly
      if (isFieldMarker(field)) {
         continue;
      }

      args.push_back(field);
   }

   for (auto &field : data->read) {
      // Add only unique arguements
      if (std::find(args.begin(), args.end(), field) != args.end()) {
         continue;
      }

      args.push_back(field);
   }


   for (auto &field : args) {
      // If we have an alias, then don't put the first opcode field in the args...
      if (alias) {
         bool skipField = false;
         for (auto &op : alias->opcode) {
            if (field == op.field) {
               skipField = true;
               break;
            }
         }
         if (skipField) {
            continue;
         }
      }

      if (field == Field::aa ||
          field == Field::lk ||
          field == Field::oe ||
          field == Field::rc) {
         continue;
      }

      dis.args.push_back(disassembleField(dis.address, instr, data, field));
   }

   for (auto &field : data->flags) {
      if (field == Field::aa && instr.aa) {
         dis.name += 'a';
      } else if (field == Field::lk && instr.lk) {
         dis.name += 'l';
      } else if (field == Field::oe && instr.oe) {
         dis.name += 'o';
      } else if (field == Field::rc && instr.rc) {
         dis.name += '.';
      }
   }

   if (alias) {
      dis.text = alias->name;
   } else {
      dis.text = dis.name;
   }

   for (auto &arg : dis.args) {
      if (&arg == &dis.args[0]) {
         dis.text += " ";
      } else {
         dis.text += ", ";
      }

      dis.text += argumentToText(arg);
   }

   // Specialized Handlers
   if (data->id == InstructionID::kc) {
      auto sc = gSystem.getSyscallData(dis.args[0].constantUnsigned);

      if (sc) {
         dis.text += " ; " + sc->name;
      } else {
         dis.text += " ; ?";
      }
   }

   return true;
}
