#pragma once
#include "modules/nn_result.h"
#include "types.h"
#include "utils/structsize.h"

namespace nn
{

namespace olv
{

struct InitializeParam
{
};
UNKNOWN_SIZE(InitializeParam);

struct MainAppParam
{
};
UNKNOWN_SIZE(MainAppParam);

nn::Result
Initialize(InitializeParam *initParam);

nn::Result
Initialize(MainAppParam *mainAppParam, InitializeParam *initParam);

nn::Result
Finalize();

bool
IsInitialized();

}  // namespace olv

}  // namespace nn
