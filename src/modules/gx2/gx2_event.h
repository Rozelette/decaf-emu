#pragma once
#include "modules/coreinit/coreinit_alarm.h"
#include "modules/coreinit/coreinit_time.h"
#include "gx2_enum.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/wfunc_ptr.h"

namespace gx2
{

#pragma pack(push, 1)

struct GX2DisplayListOverrunData
{
   be_ptr<void> oldList;
   be_val<uint32_t> oldSize;
   be_ptr<void> newList;
   be_val<uint32_t> newSize;
   UNKNOWN(8);
};
CHECK_OFFSET(GX2DisplayListOverrunData, 0x00, oldList);
CHECK_OFFSET(GX2DisplayListOverrunData, 0x04, oldSize);
CHECK_OFFSET(GX2DisplayListOverrunData, 0x08, newList);
CHECK_OFFSET(GX2DisplayListOverrunData, 0x0C, newSize);
CHECK_SIZE(GX2DisplayListOverrunData, 0x18);

#pragma pack(pop)

using GX2EventCallbackFunction = wfunc_ptr<void, GX2EventType, void *>;
using be_GX2EventCallbackFunction = be_wfunc_ptr<void, GX2EventType, void *>;

BOOL
GX2DrawDone();

void
GX2WaitForVsync();

void
GX2WaitForFlip();

void
GX2SetEventCallback(GX2EventType type, GX2EventCallbackFunction func, void *userData);

void
GX2GetEventCallback(GX2EventType type, be_GX2EventCallbackFunction *funcOut, be_ptr<void> *userDataOut);

coreinit::OSTime
GX2GetRetiredTimeStamp();

coreinit::OSTime
GX2GetLastSubmittedTimeStamp();

BOOL
GX2WaitTimeStamp(coreinit::OSTime time);

void
GX2GetSwapStatus(be_val<uint32_t> *swapCount,
                 be_val<uint32_t> *flipCount,
                 be_val<coreinit::OSTime> *lastFlip,
                 be_val<coreinit::OSTime> *lastVsync);

namespace internal
{

void
initEvents();

void
vsyncAlarmHandler(coreinit::OSAlarm *alarm,
                  coreinit::OSContext *context);

std::pair<void *, uint32_t>
displayListOverrun(void *list, uint32_t size);

void
setLastSubmittedTimestamp(coreinit::OSTime timestamp);

void
setRetiredTimestamp(coreinit::OSTime timestamp);

void
onSwap();

void
onFlip();

} // namespace internal

} // namespace gx2
