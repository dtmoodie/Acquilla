#pragma once

// Meant to be a replacement for the shitty buffer implementation that currently exists
#include <Aquila/core/detail/Export.hpp>
#include "Aquila/utilities/cuda/GpuMatAllocators.h"
#include <opencv2/core/cuda.hpp>
#include <boost/lockfree/queue.hpp>

namespace aq
{
    void scoped_buffer_dallocator_callback(int status, void* user_data);
    class AQUILA_EXPORTS scoped_buffer
    {
    public:
        class AQUILA_EXPORTS GarbageCollector
        {
        public:
            GarbageCollector();
            static void Run();
        };
        scoped_buffer(cv::cuda::Stream stream);
        ~scoped_buffer();
        cv::cuda::GpuMat& getMat();
    private:
        friend void scoped_buffer_dallocator_callback(int status, void* user_data);
        cv::cuda::GpuMat* data;
        cv::cuda::Stream stream;
        static boost::lockfree::queue<cv::cuda::GpuMat*> deallocateQueue;

    };
}
