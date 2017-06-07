#pragma once
#include "Aquila/core/detail/Export.hpp"

#include <opencv2/core/cuda/utility.hpp>

namespace aq
{
    class ThrustAllocator: public cv::cuda::device::ThrustAllocator
    {
    public:
        typedef uchar value_type;
        virtual __device__ __host__ uchar* allocate(size_t numBytes);
        virtual __device__ __host__ void deallocate(uchar* ptr, size_t numBytes);
        virtual __device__ __host__ void free(uchar* ptr);
    };

}