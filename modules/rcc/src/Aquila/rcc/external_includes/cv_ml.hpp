#pragma once
#include "cv_link_config.hpp"
#include "RuntimeObjectSystem/RuntimeLinkLibrary.h"
#include <opencv2/ml.hpp>
#if _WIN32
#if _DEBUG
RUNTIME_COMPILER_LINKLIBRARY("opencv_ml" CV_VERSION_ "d.lib")
#else
RUNTIME_COMPILER_LINKLIBRARY("opencv_ml" CV_VERSION_ ".lib")
#endif
#else
RUNTIME_COMPILER_LINKLIBRARY("-lopencv_ml")
#endif
