#pragma once
#include "cv_link_config.hpp"
#include "RuntimeObjectSystem/RuntimeLinkLibrary.h"
#include <opencv2/ts.hpp>
#if _WIN32
#if _DEBUG
RUNTIME_COMPILER_LINKLIBRARY("opencv_ts" CV_VERSION_ "d.lib")
#else
RUNTIME_COMPILER_LINKLIBRARY("opencv_ts" CV_VERSION_ ".lib")
#endif
#else
RUNTIME_COMPILER_LINKLIBRARY("-lopencv_ts")
#endif
