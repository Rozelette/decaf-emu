#pragma once
#include <vector>
#include <list>
#include <string>

#include "instruction.h"

struct InstructionData;
struct ThreadState;
struct Tracer;

// TODO: Probably should rename this to something reasonable
namespace StateField {
   enum Field : uint8_t {
      Invalid,
      GPR,
      GPR0 = GPR,
      GPR31 = GPR + 31,
      FPR,
      FPR0 = FPR,
      FPR31 = FPR + 31,
      GQR,
      GQR0 = GQR,
      GQR7 = GQR + 7,
      CR,
      XER,
      LR,
      CTR,
      FPSCR,
      ReserveAddress,
      Max,
   };
}

typedef uint32_t TraceFieldType;

struct TraceFieldValue {
   struct ValueType {
      bool operator==(const ValueType& rhs) {
         return data[0] == rhs.data[0] && data[1] == rhs.data[1];
      }
      bool operator!=(const ValueType& rhs) {
         return data[0] != rhs.data[0] || data[1] != rhs.data[1];
      }
      uint64_t data[2];
   };

   union {
      struct {
         uint32_t u32v0;
         uint32_t u32v1;
         uint32_t u32v2;
         uint32_t u32v3;
      };
      struct {
         uint64_t u64v0;
         uint64_t u64v1;
      };
      struct {
         float f32v0;
         float f32v1;
         float f32v2;
         float f32v3;
      };
      struct {
         double f64v0;
         double f64v1;
      };
      struct {
         uint32_t mem_size;
         uint32_t mem_offset;
      };
      struct {
         uint64_t value0;
         uint64_t value1;
      };
      ValueType value;
   };
};
static_assert(sizeof(TraceFieldValue) == sizeof(TraceFieldValue::value), "TraceFieldValue::value size must match total structure size");

std::string
getStateFieldName(TraceFieldType type);

void
saveStateField(const ThreadState *state, TraceFieldType type, TraceFieldValue &field);

void
restoreStateField(ThreadState *state, TraceFieldType type, const TraceFieldValue &field);

struct Trace
{
   struct _R {
      TraceFieldType type;
      TraceFieldValue value;
   };
   struct _W {
      TraceFieldType type;
      TraceFieldValue value;
      TraceFieldValue prevalue;
   };

   Instruction instr;
   uint32_t cia;
   std::vector<_R> reads;
   std::vector<_W> writes;
};

const Trace& getTrace(Tracer *tracer, int index);

size_t getTracerNumTraces(Tracer *tracer);

void
traceInit(ThreadState *state, size_t size);

Trace *
traceInstructionStart(Instruction instr, InstructionData *data, ThreadState *state);

void
traceInstructionEnd(Trace *trace, Instruction instr, InstructionData *data, ThreadState *state);

void
tracePrint(ThreadState *state, int start, int count);

int
traceReg(ThreadState *state, int start, int regIdx);

void
traceRegStart(ThreadState *state, int start, int regIdx);

void
traceRegNext(int regIdx);

void
traceRegContinue();

void
tracePrintSyscall(int count);

void
traceLogSyscall(const std::string& info);
