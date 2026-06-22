
#pragma once

#include "pluginterfaces/base/fplatform.h"

#define MAJOR_VERSION_STR   "1"
#define MAJOR_VERSION_INT    1
#define SUB_VERSION_STR     "0"
#define SUB_VERSION_INT      0
#define RELEASE_NUMBER_STR  "2"
#define RELEASE_NUMBER_INT   0
#define BUILD_NUMBER_STR    "0"
#define BUILD_NUMBER_INT     0

#define FULL_VERSION_STR \
    MAJOR_VERSION_STR "." SUB_VERSION_STR "." RELEASE_NUMBER_STR "." BUILD_NUMBER_STR

#define stringOriginalFilename  "Ducker.vst3"
#if SMTG_PLATFORM_64
#define stringFileDescription   "Ducker VST3 (64Bit)"
#else
#define stringFileDescription   "Ducker VST3"
#endif
#define stringCompanyName       "EigenDSP\0"
#define stringLegalCopyright    "Copyright(c) 2025 EigenDSP."
#define stringLegalTrademarks   "VST is a trademark of Steinberg Media Technologies GmbH"

