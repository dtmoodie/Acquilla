#pragma once
#include "Aquila/core/detail/Export.hpp"
#include "Plotter.h"
#include <RuntimeObjectSystem/shared_ptr.hpp>

namespace aq
{
    class Plotter;
    class AQUILA_EXPORTS PlotManager
    {
    public:
        static PlotManager* Instance();
        rcc::shared_ptr<Plotter> GetPlot(const std::string& plotName);
        std::vector<std::string> GetAvailablePlots();
        std::vector<std::string> GetAcceptablePlotters(mo::IParam* param);
        bool CanPlotParameter(mo::IParam* param);
    };
}