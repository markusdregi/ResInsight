/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2015-     Statoil ASA
//  Copyright (C) 2015-     Ceetron Solutions AS
// 
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
// 
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
// 
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "RicWellLogPlotCurveFeatureImpl.h"

#include "RimWellAllocationPlot.h"
#include "RimWellLogCurve.h"

#include "cafSelectionManager.h"

#include <QColor>

static const int RI_LOGPLOT_CURVECOLORSCOUNT = 15;
static const int RI_LOGPLOT_CURVECOLORS[] =
{
    Qt::black,
    Qt::darkBlue,
    Qt::darkRed,
    Qt::darkGreen,
    Qt::darkYellow,
    Qt::darkMagenta,
    Qt::darkCyan,
    Qt::darkGray,
    Qt::blue,
    Qt::red,
    Qt::green,
    Qt::yellow,
    Qt::magenta,
    Qt::cyan,
    Qt::gray
};

//--------------------------------------------------------------------------------------------------
/// Pick default curve color from an index based palette
//--------------------------------------------------------------------------------------------------
cvf::Color3f RicWellLogPlotCurveFeatureImpl::curveColorFromTable()
{
    static int colorIndex = 0;
    QColor color = QColor(Qt::GlobalColor(RI_LOGPLOT_CURVECOLORS[colorIndex % RI_LOGPLOT_CURVECOLORSCOUNT]));
    ++colorIndex;
    cvf::Color3f cvfColor(color.redF(), color.greenF(), color.blueF());
    return cvfColor;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RimWellLogCurve*> RicWellLogPlotCurveFeatureImpl::selectedWellLogCurves()
{
    std::set<RimWellLogCurve*> curveSet;

    {
        std::vector<caf::PdmUiItem*> selectedItems;
        caf::SelectionManager::instance()->selectedItems(selectedItems);

        for (caf::PdmUiItem* selectedItem : selectedItems)
        {
            caf::PdmObjectHandle* objHandle = dynamic_cast<caf::PdmObjectHandle*>(selectedItem);
            if (objHandle)
            {
                std::vector<RimWellLogCurve*> childCurves;
                objHandle->descendantsIncludingThisOfType(childCurves);

                for (RimWellLogCurve* curve : childCurves)
                {
                    curveSet.insert(curve);
                }
            }
        }
    }

    std::vector<RimWellLogCurve*> allCurves;
    for (RimWellLogCurve* curve : curveSet)
    {
        allCurves.push_back(curve);
    }

    return allCurves;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellAllocationPlot* RicWellLogPlotCurveFeatureImpl::parentWellAllocationPlot()
{
    caf::PdmObjectHandle* destinationObject = dynamic_cast<caf::PdmObjectHandle*>(caf::SelectionManager::instance()->selectedItem());
    if (!destinationObject) return nullptr;

    RimWellAllocationPlot* wellAllocationPlot = nullptr;
    destinationObject->firstAncestorOrThisOfType(wellAllocationPlot);

    return wellAllocationPlot;
}
