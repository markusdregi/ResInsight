/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2016-     Statoil ASA
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

#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmPointer.h"
#include "cafAppEnum.h"

#include "cvfBase.h"
#include "cvfObject.h"
#include "cvfColor3.h"

class RimEclipseWell;
class RigFlowDiagResults;
//==================================================================================================
///  
///  
//==================================================================================================
class RimFlowDiagSolution : public caf::PdmObject
{
     CAF_PDM_HEADER_INIT;
public:
    RimFlowDiagSolution();
    virtual ~RimFlowDiagSolution();

    QString             userDescription() { return m_userDescription();} 
    RigFlowDiagResults* flowDiagResults();
    std::vector<QString>   tracerNames();

    std::map<std::string, std::vector<int> > allInjectorTracerActiveCellIndices(size_t timeStepIndex);
    std::map<std::string, std::vector<int> > allProducerTracerActiveCellIndices(size_t timeStepIndex);

    enum TracerStatusType
    {
        CLOSED,
        PRODUCER, 
        INJECTOR, 
        VARYING, 
        UNDEFINED
    };

    TracerStatusType tracerStatusOverall(QString tracerName);
    TracerStatusType tracerStatusInTimeStep(QString tracerName, size_t timeStepIndex);
    cvf::Color3f     tracerColor(QString tracerName);


protected:
    //virtual QList<caf::PdmOptionItemInfo> calculateValueOptions(const caf::PdmFieldHandle* fieldNeedingOptions, bool * useOptionsOnly) override;

private:
    std::map<std::string, std::vector<int> > allTracerActiveCellIndices(size_t timeStepIndex, bool useInjectors);

    virtual caf::PdmFieldHandle* userDescriptionField() override;
    caf::PdmField<QString> m_userDescription;

    cvf::ref<RigFlowDiagResults> m_flowDiagResults;

    //caf::PdmPtrArrayField<RimEclipseWell*> m_selectedWells;
};
