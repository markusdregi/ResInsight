/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2016 Statoil ASA
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
#pragma once
#include "cvfBase.h"
#include "cvfColor3.h"
#include <set>
#include "RifEclipseSummaryAddress.h"
#include "RimPlotCurve.h"

class RimSummaryCurve;
class RimSummaryCase;

class RimSummaryCurveAppearanceCalculator
{
public:
    RimSummaryCurveAppearanceCalculator(const std::set<std::pair<RimSummaryCase*, RifEclipseSummaryAddress> >& curveDefinitions);
    enum CurveAppearanceType
    {
        NONE,
        COLOR,
        SYMBOL,
        LINE_STYLE,
        GRADIENT,
        LINE_THICKNESS
    };

    void                          assignDimensions(CurveAppearanceType caseAppearance,
                                                   CurveAppearanceType variAppearance,
                                                   CurveAppearanceType wellAppearance,
                                                   CurveAppearanceType gropAppearance,
                                                   CurveAppearanceType regiAppearance);
    void                          getDimensions(CurveAppearanceType* caseAppearance,
                                                CurveAppearanceType* variAppearance,
                                                CurveAppearanceType* wellAppearance,
                                                CurveAppearanceType* gropAppearance,
                                                CurveAppearanceType* regiAppearance) const;

    void                          setupCurveLook(RimSummaryCurve* curve);

private:
 

    void                           setOneCurveAppearance(CurveAppearanceType appeaType, size_t totalCount, int appeaIdx, RimSummaryCurve* curve);

    cvf::Color3f                   cycledPaletteColor(int colorIndex);
    RimPlotCurve::LineStyleEnum    cycledLineStyle(int index);
    RimPlotCurve::PointSymbolEnum  cycledSymbol(int index);
    int                            cycledLineThickness(int index);
    float                          gradient(size_t totalCount, int index);
    
    cvf::Color3f                   gradeColor(const cvf::Color3f& color , float factor);

    cvf::Color3f                   m_currentCurveBaseColor;
    float                          m_currentCurveGradient;

    size_t                         m_caseCount;
    size_t                         m_variableCount;
    size_t                         m_wellCount;
    size_t                         m_groupCount;
    size_t                         m_regionCount;
    int                            m_dimensionCount;

    CurveAppearanceType            m_caseAppearanceType;
    CurveAppearanceType            m_varAppearanceType;
    CurveAppearanceType            m_wellAppearanceType;
    CurveAppearanceType            m_groupAppearanceType;
    CurveAppearanceType            m_regionAppearanceType;

    std::map<RimSummaryCase*, int> m_caseToAppearanceIdxMap;
    std::map<std::string    , int> m_varToAppearanceIdxMap;
    std::map<std::string    , int> m_welToAppearanceIdxMap;
    std::map<std::string    , int> m_grpToAppearanceIdxMap;
    std::map<int            , int> m_regToAppearanceIdxMap;

};


