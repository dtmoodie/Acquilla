#pragma once
#include "cv_link_config.hpp"
#include "opencv2/cudabgsegm.hpp"
#include "RuntimeObjectSystem/RuntimeLinkLibrary.h"
#ifdef _MSC_VER // Windows

#ifdef _DEBUG
RUNTIME_COMPILER_LINKLIBRARY("opencv_cudabgsegm" CV_VERSION_ "d.lib")
#else
RUNTIME_COMPILER_LINKLIBRARY("opencv_cudabgsegm" CV_VERSION_ ".lib")
#endif

#else // Linux
RUNTIME_COMPILER_LINKLIBRARY("-lopencv_cudabgsegm")
#define CALL
#endif
