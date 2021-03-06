#include <functional>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <sstream>
#include <iostream>
#include <thread>
#include "debugnet.h"
#include "debugmsg.h"
#include "debugger.h"
#include "cpu/disassembler.h"
#include "cpu/instructiondata.h"
#include "mem/mem.h"
#include "modules/coreinit/coreinit_thread.h"
#include "processor.h"
#include "platform/platform.h"
#include "system.h"
#include "cpu/trace.h"
#include "utils/log.h"

DebugNet
gDebugNet;

struct DebugTraceEntryField {
   TraceFieldType type;
   TraceFieldValue data;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(type, data.u64v0, data.u64v1);
   }
};
static_assert(sizeof(TraceFieldType) == 4, "Protocol expects 4-byte TraceFieldType");
static_assert(sizeof(TraceFieldValue) == 16, "Protocol expects 16-byte TraceFieldValue");

static_assert(StateField::Invalid == 0, "Protocol expects Invalid to be 0");
static_assert(StateField::GPR == 1, "Protocol expects GPR to begin at 1");
static_assert(StateField::FPR == 33, "Protocol expects FPR to begin at 32");

struct DebugTraceEntry {
   uint32_t cia;
   std::vector<DebugTraceEntryField> fields;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(cia, fields);
   }
};

struct DebugSymbolInfo {
   uint32_t moduleIdx;
   std::string name;
   uint32_t address;
   uint32_t type;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(moduleIdx, name, address, type);
   }
};

struct DebugModuleInfo {
   std::string name;
   uint32_t entryPoint;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(name, entryPoint);
   }
};

struct DebugThreadInfo {
   std::string name;
   uint32_t id;

   int32_t curCoreId;
   uint32_t attribs;
   uint32_t state;

   uint32_t entryPoint;
   uint32_t stackStart;
   uint32_t stackEnd;

   uint32_t cia;
   uint32_t gpr[32];
   uint32_t lr;
   uint32_t ctr;
   uint32_t crf;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(name, id, curCoreId, attribs, state);
      ar(entryPoint, stackStart, stackEnd);
      ar(cia, gpr, lr, ctr, crf);
   }
};

struct DebugPauseInfo {
   std::vector<DebugModuleInfo> modules;
   uint32_t userModuleIdx;
   std::vector<DebugThreadInfo> threads;
   std::vector<DebugSymbolInfo> symbols;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(modules);
      ar(userModuleIdx);
      ar(threads);
      ar(symbols);
   }
};

static void
populateDebugPauseInfo(DebugPauseInfo& info)
{
   auto &loadedModules = gLoader.getLoadedModules();
   int moduleIdx = 0;
   int userModuleIdx = -1;
   auto userModule = gSystem.getUserModule();

   for (auto &modItr : loadedModules) {
      auto &moduleName = modItr.first;
      auto &loadedModule = modItr.second;

      DebugModuleInfo tmod;
      tmod.name = moduleName;
      tmod.entryPoint = loadedModule->entryPoint;
      info.modules.push_back(tmod);

      for (auto &symItr : loadedModule->symbols) {
         DebugSymbolInfo tsym;
         tsym.moduleIdx = moduleIdx;
         tsym.name = symItr.first;
         tsym.address = symItr.second;
         tsym.type = 0;
         info.symbols.push_back(tsym);
      }

      if (loadedModule.get() == userModule) {
         userModuleIdx = moduleIdx;
      }
      moduleIdx++;
   }
   info.userModuleIdx = static_cast<uint32_t>(userModuleIdx);

   auto &coreList = gProcessor.getCoreList();
   auto &fiberList = gProcessor.getFiberList();

   std::map<coreinit::OSThread *, Fiber *> threads;

   for (auto &fiber : fiberList) {
      auto &thread = fiber->thread;
      auto titer = threads.find(thread);
      assert(titer == threads.end());

      threads.emplace(thread, fiber);
   }

   for (auto &i : threads) {
      auto &thread = i.first;
      auto &fiber = i.second;

      DebugThreadInfo tinfo;
      tinfo.name = thread->name;
      tinfo.id = thread->id;

      tinfo.curCoreId = -1;
      for (auto &core : coreList) {
         if (core->currentFiber) {
            if (thread == core->currentFiber->thread) {
               tinfo.curCoreId = core->id;
            }
         }
      }

      tinfo.entryPoint = thread->entryPoint;
      tinfo.stackStart = thread->stackStart.getAddress();
      tinfo.stackEnd = thread->stackEnd.getAddress();

      tinfo.attribs = thread->attr;
      tinfo.state = thread->state;

      tinfo.cia = fiber->state.cia;
      tinfo.lr = fiber->state.lr;
      tinfo.ctr = fiber->state.ctr;
      tinfo.crf = fiber->state.cr.value;
      memcpy(tinfo.gpr, fiber->state.gpr, sizeof(uint32_t) * 32);

      info.threads.push_back(tinfo);
   }
}

static void populateDebugTraceEntrys(std::vector<DebugTraceEntry>& entries, ThreadState *state)
{
   auto &tracer = state->tracer;

   auto numTraces = static_cast<int>(getTracerNumTraces(tracer));
   for (int i = 0; i < numTraces; ++i) {
      auto &trace = getTrace(tracer, i);

      DebugTraceEntry entry;
      entry.cia = trace.cia;

      for (auto &i : trace.writes) {
         DebugTraceEntryField field;
         field.type = i.type;
         field.data = i.prevalue;
         entry.fields.push_back(field);
      }

      entries.push_back(entry);
   }
}

enum class DebugPacketType : uint16_t {
   Invalid = 0,
   PreLaunch = 1,
   BpHit = 2,
   Pause = 3,
   Resume = 4,
   AddBreakpoint = 5,
   RemoveBreakpoint = 6,
   ReadMem = 7,
   ReadMemRes = 8,
   Disasm = 9,
   DisasmRes = 10,
   StepCore = 11,
   CoreStepped = 12,
   Paused = 13,
   GetTrace = 14,
   GetTraceRes = 15,
   StepCoreOver = 16,
};

#pragma pack(push, 1)
struct DebugNetHeader {
   uint32_t size;
   DebugPacketType command;
   uint16_t reqId;
};
#pragma pack(pop)

class DebugPacket : public MessageClass<DebugPacketType> { };
template<DebugPacketType TypeId>
using DebugPacketBase = MessageClassBase<DebugPacket, TypeId>;

class DebugPacketBpHit : public DebugPacketBase<DebugPacketType::BpHit> {
public:
   uint32_t coreId;
   uint32_t userData;
   DebugPauseInfo info;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(coreId, userData, info);
   }

};

class DebugPacketPreLaunch : public DebugPacketBase<DebugPacketType::PreLaunch> {
public:
   DebugPauseInfo info;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(info);
   }

};

class DebugPacketPause : public DebugPacketBase<DebugPacketType::Pause> {
public:
   template <class Archive>
   void serialize(Archive &ar) { }

};

class DebugPacketResume : public DebugPacketBase<DebugPacketType::Resume> {
public:
   template <class Archive>
   void serialize(Archive &ar) { }

};

class DebugPacketBpAdd : public DebugPacketBase<DebugPacketType::AddBreakpoint> {
public:
   uint32_t address;
   uint32_t userData;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(address, userData);
   }

};

class DebugPacketBpRemove : public DebugPacketBase<DebugPacketType::RemoveBreakpoint> {
public:
   uint32_t address;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(address);
   }

};

class DebugPacketReadMem : public DebugPacketBase<DebugPacketType::ReadMem> {
public:
   uint32_t address;
   uint32_t size;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(address, size);
   }

};

class DebugPacketReadMemRes : public DebugPacketBase<DebugPacketType::ReadMemRes> {
public:
   uint32_t address;
   std::vector<uint8_t> data;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(address, data);
   }

};

class DebugPacketDisasm : public DebugPacketBase<DebugPacketType::Disasm> {
public:
   uint32_t address;
   uint32_t numInstr;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(address, numInstr);
   }

};

class DebugPacketDisasmRes : public DebugPacketBase<DebugPacketType::DisasmRes> {
public:
   uint32_t address;
   std::vector<std::string> lines;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(address, lines);
   }

};

class DebugPacketStepCore : public DebugPacketBase<DebugPacketType::StepCore> {
public:
   uint32_t coreId;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(coreId);
   }

};

class DebugPacketCoreStepped : public DebugPacketBase<DebugPacketType::CoreStepped> {
public:
   uint32_t coreId;
   DebugPauseInfo info;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(coreId, info);
   }

};

class DebugPacketPaused : public DebugPacketBase<DebugPacketType::Paused> {
public:
   DebugPauseInfo info;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(info);
   }

};

class DebugPacketGetTrace : public DebugPacketBase<DebugPacketType::GetTrace> {
public:
   uint32_t threadId;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(threadId);
   }

};

class DebugPacketGetTraceRes : public DebugPacketBase<DebugPacketType::GetTraceRes> {
public:
   uint32_t threadId;
   std::vector<DebugTraceEntry> info;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(threadId, info);
   }

};

class DebugPacketStepCoreOver : public DebugPacketBase<DebugPacketType::StepCoreOver> {
public:
   uint32_t coreId;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(coreId);
   }

};

template <typename T>
int serializePacket2(std::vector<uint8_t> &data, DebugPacket *packet) {
   std::ostringstream str;
   cereal::BinaryOutputArchive archive(str);
   T &typedPacket = *(T*)packet;
   archive(typedPacket);
   std::string payload = str.str();

   data.resize(sizeof(DebugNetHeader) + payload.size());
   auto &header = *reinterpret_cast<DebugNetHeader*>(data.data());
   header.size = (uint16_t)data.size();
   header.command = packet->type();
   std::copy(payload.begin(), payload.end(), data.begin() + sizeof(DebugNetHeader));
   return 0;
}

struct vectorwrapbuf : public std::basic_streambuf<char> {
   vectorwrapbuf(const uint8_t *start, const uint8_t *end) {
      this->setg((char*)start, (char*)start, (char*)end);
   }
};

template <typename T>
int deserializePacket2(const std::vector<uint8_t> &data, DebugPacket *&packet) {
   vectorwrapbuf databuf(data.data() + sizeof(DebugNetHeader), data.data() + data.size());
   std::istream stream(&databuf);
   cereal::BinaryInputArchive archive(stream);
   packet = new T();
   T &packetRef = *static_cast<T*>(packet);
   archive(packetRef);
   return 0;
}

int serializePacket(std::vector<uint8_t> &data, DebugPacket *packet)
{
   switch (packet->type()) {
   case DebugPacketType::PreLaunch:
      return serializePacket2<DebugPacketPreLaunch>(data, packet);
   case DebugPacketType::BpHit:
      return serializePacket2<DebugPacketBpHit>(data, packet);
   case DebugPacketType::Pause:
      return serializePacket2<DebugPacketPause>(data, packet);
   case DebugPacketType::Resume:
      return serializePacket2<DebugPacketResume>(data, packet);
   case DebugPacketType::AddBreakpoint:
      return serializePacket2<DebugPacketBpAdd>(data, packet);
   case DebugPacketType::RemoveBreakpoint:
      return serializePacket2<DebugPacketBpRemove>(data, packet);
   case DebugPacketType::ReadMem:
      return serializePacket2<DebugPacketReadMem>(data, packet);
   case DebugPacketType::ReadMemRes:
      return serializePacket2<DebugPacketReadMemRes>(data, packet);
   case DebugPacketType::Disasm:
      return serializePacket2<DebugPacketDisasm>(data, packet);
   case DebugPacketType::DisasmRes:
      return serializePacket2<DebugPacketDisasmRes>(data, packet);
   case DebugPacketType::StepCore:
      return serializePacket2<DebugPacketStepCore>(data, packet);
   case DebugPacketType::CoreStepped:
      return serializePacket2<DebugPacketCoreStepped>(data, packet);
   case DebugPacketType::Paused:
      return serializePacket2<DebugPacketPaused>(data, packet);
   case DebugPacketType::GetTrace:
      return serializePacket2<DebugPacketGetTrace>(data, packet);
   case DebugPacketType::GetTraceRes:
      return serializePacket2<DebugPacketGetTraceRes>(data, packet);
   case DebugPacketType::StepCoreOver:
      return serializePacket2<DebugPacketStepCoreOver>(data, packet);
   default:
      return -1;
   }
}

int deserializePacket(const std::vector<uint8_t> &data, DebugPacket *&packet) {
   auto &header = *reinterpret_cast<const DebugNetHeader*>(data.data());
   assert(data.size() == header.size);

   switch (header.command) {
   case DebugPacketType::PreLaunch:
      return deserializePacket2<DebugPacketPreLaunch>(data, packet);
   case DebugPacketType::BpHit:
      return deserializePacket2<DebugPacketBpHit>(data, packet);
   case DebugPacketType::Pause:
      return deserializePacket2<DebugPacketPause>(data, packet);
   case DebugPacketType::Resume:
      return deserializePacket2<DebugPacketResume>(data, packet);
   case DebugPacketType::AddBreakpoint:
      return deserializePacket2<DebugPacketBpAdd>(data, packet);
   case DebugPacketType::RemoveBreakpoint:
      return deserializePacket2<DebugPacketBpRemove>(data, packet);
   case DebugPacketType::ReadMem:
      return deserializePacket2<DebugPacketReadMem>(data, packet);
   case DebugPacketType::ReadMemRes:
      return deserializePacket2<DebugPacketReadMemRes>(data, packet);
   case DebugPacketType::Disasm:
      return deserializePacket2<DebugPacketDisasm>(data, packet);
   case DebugPacketType::DisasmRes:
      return deserializePacket2<DebugPacketDisasmRes>(data, packet);
   case DebugPacketType::StepCore:
      return deserializePacket2<DebugPacketStepCore>(data, packet);
   case DebugPacketType::CoreStepped:
      return deserializePacket2<DebugPacketCoreStepped>(data, packet);
   case DebugPacketType::Paused:
      return deserializePacket2<DebugPacketPaused>(data, packet);
   case DebugPacketType::GetTrace:
      return deserializePacket2<DebugPacketGetTrace>(data, packet);
   case DebugPacketType::GetTraceRes:
      return deserializePacket2<DebugPacketGetTraceRes>(data, packet);
   case DebugPacketType::StepCoreOver:
      return deserializePacket2<DebugPacketStepCoreOver>(data, packet);
   default:
      return -1;
   }
}

DebugNet::DebugNet()
{
   mConnected = false;
   setTarget("127.0.0.1", 11234);
}

void
DebugNet::setTarget(const std::string& address, uint16_t port)
{
   mAddress = address;
   mPort = port;
}

bool
DebugNet::connect()
{
#ifdef PLATFORM_WINDOWS
   static bool wsaInited = false;

   if (!wsaInited) {
      WSADATA wsaData;
      WSAStartup(MAKEWORD(2, 2), &wsaData);
      wsaInited = true;
   }

   mConnected = false;

   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

   struct sockaddr_in server;
   server.sin_addr.s_addr = inet_addr(mAddress.c_str());
   server.sin_family = AF_INET;
   server.sin_port = htons(mPort);

   int conres = ::connect(sock, (struct sockaddr *)&server, sizeof(server));

   if (conres < 0) {
      // Connect failure
      return false;
   }

   mConnected = true;
   mSocket = (void*)sock;
   mNetworkThread = new std::thread(&DebugNet::networkThread, this);
   return true;
#else
   return false;
#endif
}

void
DebugNet::networkThread()
{
#ifdef PLATFORM_WINDOWS
   SOCKET sock = (SOCKET)mSocket;
   std::vector<uint8_t> buffer;

   while (true) {
      buffer.resize(sizeof(DebugNetHeader));
      int recvd = recv(sock, (char*)&buffer[0], sizeof(DebugNetHeader), 0);
      if (recvd <= 0) {
         // Disconnected
         handleDisconnect();
         break;
      }
      assert(recvd == sizeof(DebugNetHeader));

      DebugNetHeader *header = (DebugNetHeader*)buffer.data();
      if (header->size > (uint32_t)buffer.size()) {
         buffer.resize(header->size);

         recvd = recv(sock,
            (char*)&buffer[sizeof(DebugNetHeader)],
            (int)buffer.size() - sizeof(DebugNetHeader),
            0);
         if (recvd <= 0) {
            handleDisconnect();
            break;
         }
         assert(recvd == buffer.size() - sizeof(DebugNetHeader));
      }

      DebugPacket *packet = nullptr;
      deserializePacket(buffer, packet);
      handlePacket(packet);
   }
#endif
}

void
DebugNet::handleDisconnect()
{
#ifdef PLATFORM_WINDOWS
   mConnected = false;
   mSocket = (void*)INVALID_SOCKET;
   gDebugger.notify(new DebugMessageDebuggerDc());
#endif
}

void
DebugNet::handlePacket(DebugPacket *pak)
{
   gLog->debug("Handling packet {}", (int)pak->type());

   switch (pak->type()) {
   case DebugPacketType::AddBreakpoint: {
      auto *bpPak = static_cast<DebugPacketBpAdd*>(pak);
      gDebugger.addBreakpoint(bpPak->address, bpPak->userData);
      break;
   }
   case DebugPacketType::RemoveBreakpoint: {
      auto *bpPak = static_cast<DebugPacketBpRemove*>(pak);
      gDebugger.removeBreakpoint(bpPak->address);
      break;
   }
   case DebugPacketType::Pause: {
      gDebugger.pause();
      break;
   }
   case DebugPacketType::Resume: {
      gDebugger.resume();
      break;
   }
   case DebugPacketType::StepCore: {
      auto *scPak = static_cast<DebugPacketStepCore*>(pak);
      gDebugger.stepCore(scPak->coreId);
      break;
   }
   case DebugPacketType::StepCoreOver: {
      auto *scPak = static_cast<DebugPacketStepCoreOver*>(pak);

      if (scPak->coreId >= 3) {
         break;
      }

      auto &coreList = gProcessor.getCoreList();
      auto &core = coreList[scPak->coreId];

      Fiber *fiber = nullptr;
      auto fiberList = gProcessor.getFiberList();
      for (auto &i : fiberList) {
         if (i == core->currentFiber) {
            fiber = i;
            break;
         }
      }
      if (!fiber) {
         break;
      }

      uint32_t curAddr = fiber->state.cia;

      auto instr = mem::read<Instruction>(curAddr);
      auto data = gInstructionTable.decode(instr);
      if (data->id == InstructionID::b || data->id == InstructionID::bc ||
         data->id == InstructionID::bcctr || data->id == InstructionID::bclr) {
         if (instr.lk) {
            // This is a branch-and-link.  Put a BP after the next instruction
            gDebugger.addBreakpoint(curAddr + 4, 0xFFFFFFFF);
            gDebugger.resume();
         } else {
            // Direct branch, just step
            gDebugger.stepCore(scPak->coreId);
         }
      } else {
         // Not a branch, just step
         gDebugger.stepCore(scPak->coreId);
      }

      break;
   }
   case DebugPacketType::ReadMem: {
      auto *rmPak = static_cast<DebugPacketReadMem*>(pak);
      if (mem::valid(rmPak->address)) {
         auto pakO = new DebugPacketReadMemRes();

         pakO->address = rmPak->address;
         uint8_t *data = mem::translate(rmPak->address);
         pakO->data.resize(rmPak->size);
         memcpy(&pakO->data[0], data, pakO->data.size());

         writePacket(pakO);
      }
      break;
   }
   case DebugPacketType::Disasm: {
      auto *dPak = static_cast<DebugPacketDisasm*>(pak);
      if (mem::valid(dPak->address)) {
         auto pakO = new DebugPacketDisasmRes();

         pakO->address = dPak->address;
         auto curAddr = dPak->address;
         for (int i = 0; i < (int)dPak->numInstr; ++i, curAddr += 4) {
            auto instr = mem::read<Instruction>(curAddr);
            Disassembly dis;
            gDisassembler.disassemble(instr, dis, curAddr);

            pakO->lines.push_back(dis.text);
         }

         writePacket(pakO);
      }
      break;
   }
   case DebugPacketType::GetTrace: {
      auto *tPak = static_cast<DebugPacketGetTrace*>(pak);

      Fiber *fiber = nullptr;
      auto fiberList = gProcessor.getFiberList();
      for (auto &i : fiberList) {
         if (i->thread->id == tPak->threadId) {
            fiber = i;
            break;
         }
      }
      if (!fiber) {
         break;
      }

      auto pakO = new DebugPacketGetTraceRes();
      populateDebugTraceEntrys(pakO->info, &fiber->state);
      writePacket(pakO);

      break;
   }
   default:
      break;
   }
}

void
DebugNet::writePacket(DebugPacket *pak)
{
#ifdef PLATFORM_WINDOWS
   gLog->debug("Writing packet {}", (int)pak->type());

   std::vector<uint8_t> buffer;
   serializePacket(buffer, pak);

   SOCKET sock = (SOCKET)mSocket;
   send(sock, (char*)&buffer[0], (int)buffer.size(), 0);
#endif
}

void
DebugNet::writePrelaunch()
{
   auto pak = new DebugPacketPreLaunch();
   populateDebugPauseInfo(pak->info);
   writePacket(pak);
}

void
DebugNet::writeBreakpointHit(uint32_t coreId, uint32_t userData)
{
   auto pak = new DebugPacketBpHit();
   pak->coreId = coreId;
   pak->userData = userData;
   populateDebugPauseInfo(pak->info);
   writePacket(pak);
}

void
DebugNet::writeCoreStepped(uint32_t coreId)
{
   auto pak = new DebugPacketCoreStepped();
   pak->coreId = coreId;
   populateDebugPauseInfo(pak->info);
   writePacket(pak);
}

void
DebugNet::writePaused()
{
   auto pak = new DebugPacketPaused();
   populateDebugPauseInfo(pak->info);
   writePacket(pak);
}
