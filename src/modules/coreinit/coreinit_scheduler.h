#pragma once
#include "types.h"
#include "coreinit_thread.h"

namespace coreinit
{

struct OSThread;
struct OSThreadQueue;

extern ThreadEntryPoint
InterruptThreadEntryPoint;

namespace internal
{

void
lockScheduler();

void
unlockScheduler();

void
rescheduleNoLock();

int32_t
resumeThreadNoLock(OSThread *thread, int32_t counter);

void
sleepThreadNoLock(OSThreadQueue *queue);

void
suspendThreadNoLock(OSThread *thread);

void
testThreadCancelNoLock();

void
wakeupOneThreadNoLock(OSThread *thread);

void
wakeupThreadNoLock(OSThreadQueue *queue);

void
wakeupThreadWaitForSuspensionNoLock(OSThreadQueue *queue, int32_t suspendResult);

OSThread *
setInterruptThread(uint32_t core, OSThread *thread);

} // namespace internal

} // namespace coreinit


