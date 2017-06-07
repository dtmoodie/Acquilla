#include "VariableSink.h"
#include <MetaObject/params/IVariableManager.hpp>
#include <sstream>
//#include <MetaObject/params/IO/TextSerializer.hpp>
using namespace aq;

CSV_VariableSink::CSV_VariableSink(const std::string& output_file)
{
    _ofs.open(output_file);
}

CSV_VariableSink::~CSV_VariableSink()
{
    _ofs.close();
}

void CSV_VariableSink::SerializeVariables(unsigned long long frame_number, mo::IVariableManager* manager)
{
    if(_serialization_layout.size())
    {
        _ofs << frame_number;
        std::stringstream ss;
        /*for(auto& var_name : _serialization_layout)
        {
            auto param = manager->getOutputParameter(var_name);
            if(param && Parameters::Persistence::Text::InterpreterRegistry::Exists(param->getTypeInfo()))
            {
                ss << ", ";
                Parameters::Persistence::Text::SerializeValue(&ss, param);
            }
        }*/
        _ofs << ss.str();
    }
}

std::string CSV_VariableSink::SerializeExample(unsigned long long frame_number, mo::IVariableManager* manager)
{
    std::stringstream example;
    if(_serialization_layout.size())
    {
        example << frame_number;
        for(auto& var_name : _serialization_layout)
        {
            auto param = manager->getOutputParam(var_name);
            if(param)
            {
                // #TODO move to new implementation
                /*if(Parameters::Persistence::Text::InterpreterRegistry::Exists(param->getTypeInfo()))
                {
                    example << ", ";
                    Parameters::Persistence::Text::SerializeValue(&example, param);
                }*/
            }
        }
    }
    return example.str();
}

std::vector<std::string> CSV_VariableSink::ListSerializableVariables(mo::IVariableManager* manager)
{
    auto params = manager->getAllParms();
    std::vector<std::string> output;
    /*for(auto param : params)
    {
        // #TODO new implementation
        if(Parameters::Persistence::Text::InterpreterRegistry::Exists(param->getTypeInfo());
            output.push_back(param->getTreeName());
    }*/
    return output;
}

void CSV_VariableSink::AddVariable(const std::string& name)
{
    _serialization_layout.push_back(name);
}

void CSV_VariableSink::SetLayout(const std::vector<std::string>& layout)
{
    _serialization_layout = layout;
}

std::vector<std::string> CSV_VariableSink::GetLayout()
{
    return _serialization_layout;
}
