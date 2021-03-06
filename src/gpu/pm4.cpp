#include "pm4_buffer.h"
#include "modules/gx2/gx2_cbpool.h"

namespace pm4
{

Buffer *
getBuffer(uint32_t size)
{
   return gx2::internal::getCommandBuffer(size);
}

Buffer *
flushBuffer(Buffer *buffer)
{
   return gx2::internal::flushCommandBuffer(buffer);
}

} // namespace pm4
