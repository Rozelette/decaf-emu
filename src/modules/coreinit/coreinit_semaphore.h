#pragma once
#include "coreinit_threadqueue.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/virtual_ptr.h"

namespace coreinit
{

#pragma pack(push, 1)

struct OSSemaphore
{
   static const uint32_t Tag = 0x73506852;

   be_val<uint32_t> tag;
   be_ptr<const char> name;
   UNKNOWN(4);
   be_val<int32_t> count;
   OSThreadQueue queue;
};
CHECK_OFFSET(OSSemaphore, 0x00, tag);
CHECK_OFFSET(OSSemaphore, 0x04, name);
CHECK_OFFSET(OSSemaphore, 0x0C, count);
CHECK_OFFSET(OSSemaphore, 0x10, queue);
CHECK_SIZE(OSSemaphore, 0x20);

#pragma pack(pop)

void
OSInitSemaphore(OSSemaphore *semaphore, int32_t count);

void
OSInitSemaphoreEx(OSSemaphore *semaphore, int32_t count, char *name);

int32_t
OSWaitSemaphore(OSSemaphore *semaphore);

int32_t
OSTryWaitSemaphore(OSSemaphore *semaphore);

int32_t
OSSignalSemaphore(OSSemaphore *semaphore);

int32_t
OSGetSemaphoreCount(OSSemaphore *semaphore);

} // namespace coreinit
