#include <MetaObject/core/SystemTable.hpp>
#include <MetaObject/core/detail/MemoryBlock.hpp>
#include <MetaObject/core/detail/allocator_policies/Default.hpp>

#include <MetaObject/object/MetaObjectFactory.hpp>

#include <MetaObject/cuda/Allocator.hpp>
#include <MetaObject/cuda/MemoryBlock.hpp>

#include <gtest/gtest.h>

TEST(system_table, init)
{
    auto table = SystemTable::instance();
    EXPECT_NE(table, nullptr);
}

int main(int argc, char** argv)
{
    std::shared_ptr<SystemTable> table;
    bool make_table = true;
    for (int i = 0; i < argc; ++i)
    {
        if (std::string("--gtest_list_tests") == argv[i])
        {
            make_table = false;
        }
    }
    ::testing::InitGoogleTest(&argc, argv);
    if (make_table)
    {
        table = SystemTable::instance();
        auto allocator = std::make_shared<mo::DefaultAllocator<mo::CPU>>();
        table->setDefaultAllocator(allocator);
        using DeviceAllocator = mo::DefaultAllocator<mo::cuda::CUDA>;
        table->getSingleton<mo::DeviceAllocator, DeviceAllocator>();
        mo::MetaObjectFactory::instance()->registerTranslationUnit();
    }

    return RUN_ALL_TESTS();
}