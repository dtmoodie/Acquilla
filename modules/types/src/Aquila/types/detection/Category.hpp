#pragma once
#include "Classification.hpp"
#include <Aquila/core/detail/Export.hpp>
#include <cereal/cereal.hpp>
#include <ct/reflect.hpp>
#include <map>
#include <memory>
#include <opencv2/core/matx.hpp>
#include <string>
#include <vector>
namespace aq
{
    struct AQUILA_EXPORTS Category
    {
        Category(const std::string& name = "",
                 cv::Vec3b color = cv::Vec3b(),
                 const Category* parent = nullptr,
                 unsigned int idx = 0);

        Classification operator()() const;

        Classification operator()(double conf) const;

        const std::string& getName() const;

        const Category* parent = nullptr;
        std::string name;
        unsigned int index;
        cv::Vec3b color;
    };

    bool operator==(const Category& lhs, const Category& rhs);

    struct AQUILA_EXPORTS CategorySet : public std::vector<Category>
    {
        using Ptr = std::shared_ptr<CategorySet>;
        using ConstPtr = std::shared_ptr<const CategorySet>;

        CategorySet()
        {
        }

        template <class InputIt>
        CategorySet(InputIt first, InputIt last)
            : std::vector<Category>(first, last)
        {
        }

        CategorySet(const std::vector<std::string>& cats,
                    const std::map<std::string, cv::Vec3b>& colormap = std::map<std::string, cv::Vec3b>(),
                    const std::vector<int>& tree = std::vector<int>());

        CategorySet(const std::string& cat_file,
                    const std::map<std::string, cv::Vec3b>& colormap = std::map<std::string, cv::Vec3b>(),
                    const std::vector<int>& tree = std::vector<int>());

        const Category& operator()(const std::string& name) const;

      private:
        void colorize(const std::map<std::string, cv::Vec3b>& colormap);
        void hierarchy(const std::vector<int>& tree);
        Category& operator()(const std::string& name);
    };
}

namespace ct
{
    REFLECT_BEGIN(aq::Category)
        PUBLIC_ACCESS(name)
        PUBLIC_ACCESS(color)
        PUBLIC_ACCESS(parent)
        PUBLIC_ACCESS(index)
    REFLECT_END;
}
