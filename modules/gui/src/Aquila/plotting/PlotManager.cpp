#include "Aquila/plotters/PlotManager.h"
#include "RuntimeObjectSystem/IObjectState.hpp"
#include <MetaObject/Logging/Log.hpp>
#include "Aquila/plotters/PlotInfo.hpp"
#include "MetaObject/Parameters/detail/TypedInputParameterPtrImpl.hpp"
#include "MetaObject/Parameters/detail/TypedParameterPtrImpl.hpp"
using namespace aq;


PlotManager* PlotManager::Instance()
{
    static PlotManager instance;
    return &instance;
}

rcc::shared_ptr<Plotter> PlotManager::GetPlot(const std::string& plotName)
{
    auto pConstructor = mo::MetaObjectFactory::Instance()->GetConstructor(plotName.c_str());
    //IObjectConstructor* pConstructor = ObjectManager::Instance().m_pRuntimeObjectSystem->GetObjectFactorySystem()->GetConstructor(plotName.c_str());
    if (pConstructor && pConstructor->GetInterfaceId() == Plotter::s_interfaceID)
    {
        IObject* obj = pConstructor->Construct();
        if (obj)
        {
            obj = obj->GetInterface(Plotter::s_interfaceID);
            if (obj)
            {
                Plotter* plotter = static_cast<Plotter*>(obj);
                if (plotter)
                {
                    plotter->Init(true);
                    LOG(info) << "[ PlotManager ] successfully generating plot " << plotName;
                    return rcc::shared_ptr<Plotter>(plotter);
                }
                else
                {
                    LOG(warning) << "[ PlotManager ] failed to cast to plotter object " << plotName;
                }
            }
            else
            {
                LOG(warning) << "[ PlotManager ] incorrect interface " << plotName;
            }
        }
        else
        {
            LOG(warning) << "[ PlotManager ] failed to construct plot " << plotName;
        }
    }
    else
    {
        LOG(warning) << "[ PlotManager ] failed to get constructor " << plotName;
    }
    return rcc::shared_ptr<Plotter>();
}

std::vector<std::string> PlotManager::GetAvailablePlots()
{
    std::vector<std::string> output;
    auto constructors = mo::MetaObjectFactory::Instance()->GetConstructors(Plotter::s_interfaceID);
    
    for (size_t i = 0; i < constructors.size(); ++i)
    {
        output.push_back(constructors[i]->GetName());
    }
    return output;
}
std::vector<std::string> PlotManager::GetAcceptablePlotters(mo::IParameter* param)
{
    std::vector<std::string> output;
    auto constructors = mo::MetaObjectFactory::Instance()->GetConstructors(Plotter::s_interfaceID);
    
    for(auto& constructor : constructors)
    {
        auto object_info = constructor->GetObjectInfo();
        if(object_info)
        {
            auto plot_info = dynamic_cast<PlotterInfo*>(object_info);
            if(plot_info)
            {
                if(plot_info->AcceptsParameter(param))
                {
                    output.push_back(constructor->GetName());
                }
            }
        }
    }
    return output;
}
bool PlotManager::CanPlotParameter(mo::IParameter* param)
{
    auto constructors = mo::MetaObjectFactory::Instance()->GetConstructors(Plotter::s_interfaceID);
    //auto constructors = ObjectManager::Instance().GetConstructorsForInterface(IID_Plotter);
    for(auto& constructor : constructors)
    {
        auto object_info = constructor->GetObjectInfo();
        if(object_info)
        {
            auto plot_info = dynamic_cast<PlotterInfo*>(object_info);
            if(plot_info)
            {
                if(plot_info->AcceptsParameter(param))
                {
                    return true;
                }
            }
        }
    }
    return false;
}
