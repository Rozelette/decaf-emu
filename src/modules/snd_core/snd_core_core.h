#pragma once
#include "snd_core_result.h"
#include "utils/wfunc_ptr.h"

namespace snd_core
{

struct AXProfile
{
};

using AXFrameCallback = wfunc_ptr<void>;

void
AXInit();

BOOL
AXIsInit();

void
AXInitProfile(AXProfile *profile, uint32_t count);

uint32_t
AXGetSwapProfile(AXProfile *profile, uint32_t count);

AXResult
AXSetDefaultMixerSelect(uint32_t);

AXResult
AXRegisterAppFrameCallback(AXFrameCallback callback);

int32_t
AXRmtGetSamplesLeft();

int32_t
AXRmtGetSamples(int32_t, be_val<uint8_t> *buffer, int32_t samples);

int32_t
AXRmtAdvancePtr(int32_t);

} // namespace snd_core
