#include "Aquila/utilities/ColorMapperFactory.hpp"
#include "Aquila/utilities/ColorScale.hpp"
#include <boost/filesystem.hpp>
#include <fstream>
#if defined(Aquila_HAVE_CEREAL)
#include "Aquila/utilities/LinearColormapper.hpp"
#include <MetaObject/params/IO/TextPolicy.hpp>
#include <MetaObject/serialization/CerealPolicy.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/tuple.hpp>
#endif

using namespace aq;

ColorMapperFactory* ColorMapperFactory::Instance()
{
    static ColorMapperFactory inst;
    return &inst;
}

IColorMapper* ColorMapperFactory::create(std::string color_mapping_scheme_, float alpha, float beta)
{
    auto itr = _registered_functions.find(color_mapping_scheme_);
    if (itr != _registered_functions.end())
    {
        return itr->second(alpha, beta);
    }
    return nullptr;
}

void ColorMapperFactory::Register(std::string colorMappingScheme,
                                  std::function<IColorMapper*(float, float)> creation_function_)
{
    _registered_functions[colorMappingScheme] = creation_function_;
}
void ColorMapperFactory::Load(std::string definition_file_xml)
{
    if (boost::filesystem::is_regular_file(definition_file_xml))
    {
#ifdef Aquila_HAVE_CEREAL
        std::ifstream ifs(definition_file_xml);
        cereal::XMLInputArchive ar(ifs);
        std::map<std::string, std::tuple<ColorScale, ColorScale, ColorScale>> color_scales;
        ar(CEREAL_NVP(color_scales));
        for (auto& scheme : color_scales)
        {
            ColorMapperFactory::Instance()->Register(scheme.first, [scheme](float alpha, float beta) -> IColorMapper* {
                ColorScale red = std::get<0>(scheme.second);
                ColorScale green = std::get<1>(scheme.second);
                ColorScale blue = std::get<2>(scheme.second);
                red.Rescale(alpha, beta);
                green.Rescale(alpha, beta);
                blue.Rescale(alpha, beta);
                return new LinearColorMapper(red, green, blue);
            });
        }
#endif
    }
}

void ColorMapperFactory::Save(std::string definition_file_xml)
{
#ifdef Aquila_HAVE_CEREAL
    std::ofstream ofs(definition_file_xml);
    cereal::XMLOutputArchive ar(ofs);

#endif
}
std::vector<std::string> ColorMapperFactory::ListSchemes()
{
    std::vector<std::string> output;
    for (auto& scheme : _registered_functions)
    {
        output.push_back(scheme.first);
    }
    return output;
}
