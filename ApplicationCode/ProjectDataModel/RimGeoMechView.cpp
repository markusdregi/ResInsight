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

#include "RimGeoMechView.h"

#include "Rim3dOverlayInfoConfig.h"
#include "RiaApplication.h"
#include "RiaPreferences.h"
#include "RimGeoMechResultSlot.h"

#include "RiuMainWindow.h"
#include "cafCeetronPlusNavigation.h"
#include "cafCadNavigation.h"
#include "RimLegendConfig.h"
#include "cvfOverlayScalarMapperLegend.h"

#include "RimGeoMechCase.h"
#include "cvfPart.h"
#include "cvfViewport.h"
#include "cvfModelBasicList.h"
#include "cvfScene.h"
#include "RimEclipseView.h"
#include "RiuViewer.h"
#include "RivGeoMechPartMgr.h"
#include "RigGeoMechCaseData.h"
#include "cvfqtUtils.h"
#include "RigFemPartCollection.h"
#include "cafFrameAnimationControl.h"
#include <QMessageBox>
#include "cafProgressInfo.h"
#include "RimCellRangeFilterCollection.h"
#include "RivGeoMechPartMgrCache.h"
#include "RivGeoMechVizLogic.h"
#include "RigFemPartGrid.h"




CAF_PDM_SOURCE_INIT(RimGeoMechView, "GeoMechView");
//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimGeoMechView::RimGeoMechView(void)
{
    RiaApplication* app = RiaApplication::instance();
    RiaPreferences* preferences = app->preferences();
    CVF_ASSERT(preferences);

    CAF_PDM_InitObject("Geomechanical View", ":/ReservoirView.png", "", "");

    CAF_PDM_InitFieldNoDefault(&cellResult, "GridCellResult", "Color Result", ":/CellResult.png", "", "");
    cellResult = new RimGeoMechResultSlot();

    CAF_PDM_InitFieldNoDefault(&rangeFilterCollection, "RangeFilters", "Range Filters", "", "", "");
    rangeFilterCollection = new RimCellRangeFilterCollection();
    rangeFilterCollection->setReservoirView(this);


    this->cellResult()->setReservoirView(this);
    this->cellResult()->legendConfig()->setPosition(cvf::Vec2ui(10, 120));
    this->cellResult()->legendConfig()->setReservoirView(this);

    m_scaleTransform = new cvf::Transform();
    m_vizLogic = new RivGeoMechVizLogic(this);

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimGeoMechView::~RimGeoMechView(void)
{
    m_geomechCase = NULL;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimGeoMechView::updateViewerWidgetWindowTitle()
{
    if (m_viewer)
    {
        QString windowTitle;
        if (m_geomechCase.notNull())
        {
            windowTitle = QString("%1 - %2").arg(m_geomechCase->caseUserDescription()).arg(name);
        }
        else
        {
            windowTitle = name;
        }

        m_viewer->layoutWidget()->setWindowTitle(windowTitle);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimGeoMechView::loadDataAndUpdate()
{
    caf::ProgressInfo progress(7, "Loading Case");
    progress.setNextProgressIncrement(5);
    updateScaleTransform();

    if (m_geomechCase)
    {
         if (!m_geomechCase->openGeoMechCase())
         {
            QMessageBox::warning(RiuMainWindow::instance(), 
                                "Error when opening project file", 
                                "Could not open the Odb file: \n"+ m_geomechCase->caseFileName());
            m_geomechCase = NULL;
            return;
         }
       // m_geoMechFullModel->clearAndSetReservoir(m_geomechCase->geoMechData(), this);
    }
    progress.incrementProgress();

    progress.setProgressDescription("Reading Current Result");

    CVF_ASSERT(this->cellResult() != NULL);
    m_geomechCase->geoMechData()->assertResultsLoaded(0, this->cellResult()->resultAddress());
    
    progress.incrementProgress();
    progress.setProgressDescription("Create Display model");
   
    updateViewerWidget();

    createDisplayModelAndRedraw();

    if (cameraPosition().isIdentity())
    {
        setDefaultView();
    }

    progress.incrementProgress();
}

//--------------------------------------------------------------------------------------------------
/// 
/// Todo: Work in progress
/// 
//--------------------------------------------------------------------------------------------------

void RimGeoMechView::updateScaleTransform()
{
    cvf::Mat4d scale = cvf::Mat4d::IDENTITY;
    scale(2, 2) = scaleZ();

    this->scaleTransform()->setLocalTransform(scale);

    if (m_viewer) m_viewer->updateCachedValuesInScene();
}

//--------------------------------------------------------------------------------------------------
/// Create display model,
/// or at least empty scenes as frames that is delivered to the viewer
/// The real geometry generation is done inside RivReservoirViewGeometry and friends
//--------------------------------------------------------------------------------------------------
void RimGeoMechView::createDisplayModel()
{
   if (m_viewer.isNull()) return;

   if (!(m_geomechCase 
          && m_geomechCase->geoMechData() 
          && m_geomechCase->geoMechData()->femParts())) 
        return;

   int partCount = m_geomechCase->geoMechData()->femParts()->partCount();

   if (partCount <= 0) return;

   // Define a vector containing time step indices to produce geometry for.
   // First entry in this vector is used to define the geometry only result mode with no results.
   std::vector<int> timeStepIndices;

   // The one and only geometry entry
   timeStepIndices.push_back(0);

   // Find the number of time frames the animation needs to show the requested data.

   if (isTimeStepDependentDataVisible())
   {
       int i;
       for (i = 0; i < geoMechCase()->geoMechData()->frameCount(0, cellResult()->resultAddress()); i++)
       {
           timeStepIndices.push_back(i);
       }
   }

    cvf::Collection<cvf::ModelBasicList> frameModels;
    size_t timeIdx;
    for (timeIdx = 0; timeIdx < timeStepIndices.size(); timeIdx++)
    {
        frameModels.push_back(new cvf::ModelBasicList);
    }

    // Remove all existing animation frames from the viewer. 
    // The parts are still cached in the RivReservoir geometry and friends

    bool isAnimationActive = m_viewer->isAnimationActive();
    m_viewer->removeAllFrames();

    m_vizLogic->appendNoAnimPartsToModel(frameModels[0].p());
    m_vizLogic->updateStaticCellColors();

   // Create Scenes from the frameModels
   // Animation frames for results display, starts from frame 1

   size_t frameIndex;
   for (frameIndex = 0; frameIndex < frameModels.size(); frameIndex++)
   {
       cvf::ModelBasicList* model = frameModels.at(frameIndex);
       model->updateBoundingBoxesRecursive();

       cvf::ref<cvf::Scene> scene = new cvf::Scene;
       scene->addModel(model);

       if (frameIndex == 0)
           m_viewer->setMainScene(scene.p());
       else
           m_viewer->addFrame(scene.p());
   }

   // If the animation was active before recreating everything, make viewer view current frame

   if (isAnimationActive && cellResult->resultFieldName() != "")
   {
       m_viewer->slotSetCurrentFrame(m_currentTimeStep);
   }

   updateLegends();
   overlayInfoConfig()->update3DInfo();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimGeoMechView::updateCurrentTimeStep()
{
    updateLegends();
    if ((this->animationMode() && cellResult()->resultFieldName() != ""))
    {
        if (m_viewer)
        {
            cvf::Scene* frameScene = m_viewer->frame(m_currentTimeStep);
            if (frameScene)
            {
                cvf::ref<cvf::ModelBasicList> frameParts = new cvf::ModelBasicList;
                m_vizLogic->appendPartsToModel(m_currentTimeStep, frameParts.p());

                frameParts->updateBoundingBoxesRecursive();
                frameScene->removeAllModels();
                frameScene->addModel(frameParts.p());
            }
        }

        m_vizLogic->updateCellResultColor(m_currentTimeStep(), this->cellResult());
    }
    else
    {
        this->updateStaticCellColors();
    }

    overlayInfoConfig()->update3DInfo();

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimGeoMechView::updateStaticCellColors()
{
    m_vizLogic->updateStaticCellColors();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimGeoMechView::updateDisplayModelVisibility()
{
    if (m_viewer.isNull()) return;

    const cvf::uint uintSurfaceBit      = surfaceBit;
    const cvf::uint uintMeshSurfaceBit  = meshSurfaceBit;
    const cvf::uint uintFaultBit        = faultBit;
    const cvf::uint uintMeshFaultBit    = meshFaultBit;
 
    // Initialize the mask to show everything except the the bits controlled here
    unsigned int mask = 0xffffffff & ~uintSurfaceBit & ~uintFaultBit & ~uintMeshSurfaceBit & ~uintMeshFaultBit ;

    // Then turn the appropriate bits on according to the user settings

    if (surfaceMode == SURFACE)
    {
         mask |= uintSurfaceBit;
         mask |= uintFaultBit;
    }
    else if (surfaceMode == FAULTS)
    {
        mask |= uintFaultBit;
    }

    if (meshMode == FULL_MESH)
    {
        mask |= uintMeshSurfaceBit;
        mask |= uintMeshFaultBit;
    }
    else if (meshMode == FAULTS_MESH)
    {
        mask |= uintMeshFaultBit;
    }

    m_viewer->setEnableMask(mask);
    m_viewer->update();

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimGeoMechView::setGeoMechCase(RimGeoMechCase* gmCase)
{
    m_geomechCase = gmCase;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimGeoMechView::resetLegendsInViewer()
{
    this->cellResult()->legendConfig->recreateLegend();

    m_viewer->removeAllColorLegends();
    m_viewer->addColorLegendToBottomLeftCorner(this->cellResult()->legendConfig->legend());
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimGeoMechView::updateLegends()
{
    if (m_viewer)
    {
        m_viewer->removeAllColorLegends();
    }

    if (!m_geomechCase || !m_viewer || !m_geomechCase->geoMechData() )
    {
        return;
    }

    RigGeoMechCaseData* gmCase = m_geomechCase->geoMechData();
    CVF_ASSERT(gmCase);


    double localMin, localMax;
    double localPosClosestToZero, localNegClosestToZero;
    double globalMin, globalMax;
    double globalPosClosestToZero, globalNegClosestToZero;

    RigFemResultAddress resVarAddress = cellResult->resultAddress();
    if (resVarAddress.fieldName != "")
    {
        gmCase->minMaxScalarValues(resVarAddress, 0, m_currentTimeStep, &localMin, &localMax);
        gmCase->posNegClosestToZero(resVarAddress, 0, m_currentTimeStep, &localPosClosestToZero, &localNegClosestToZero);

        gmCase->minMaxScalarValues(resVarAddress, 0, &globalMin, &globalMax);
        gmCase->posNegClosestToZero(resVarAddress, 0, &globalPosClosestToZero, &globalNegClosestToZero);


        cellResult->legendConfig->setClosestToZeroValues(globalPosClosestToZero, globalNegClosestToZero, localPosClosestToZero, localNegClosestToZero);
        cellResult->legendConfig->setAutomaticRanges(globalMin, globalMax, localMin, localMax);

        m_viewer->addColorLegendToBottomLeftCorner(cellResult->legendConfig->legend());

        cellResult->legendConfig->legend()->setTitle(cvfqt::Utils::toString(
        caf::AppEnum<RigFemResultPosEnum>(cellResult->resultPositionType()).uiText() + "\n" 
        + cellResult->resultFieldName() + " " + cellResult->resultComponentName() ));
    }
    else
    {
        cellResult->legendConfig->setClosestToZeroValues(0, 0, 0, 0);
        cellResult->legendConfig->setAutomaticRanges(cvf::UNDEFINED_DOUBLE, cvf::UNDEFINED_DOUBLE, cvf::UNDEFINED_DOUBLE, cvf::UNDEFINED_DOUBLE);
    }


}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimGeoMechCase* RimGeoMechView::geoMechCase()
{
    return m_geomechCase;
}

//--------------------------------------------------------------------------------------------------
/// Clamp the current timestep to actual possibilities
//--------------------------------------------------------------------------------------------------
void RimGeoMechView::clampCurrentTimestep()
{
    int maxFrameCount = 0;
    
    if (m_geomechCase){
        maxFrameCount = m_geomechCase->geoMechData()->frameCount(0, cellResult()->resultAddress());
    }

    if (m_currentTimeStep >= maxFrameCount ) m_currentTimeStep = maxFrameCount -1;
    if (m_currentTimeStep < 0 ) m_currentTimeStep = 0; 
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimGeoMechView::isTimeStepDependentDataVisible()
{
    return (cellResult->resultFieldName() != "");
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
cvf::Transform* RimGeoMechView::scaleTransform()
{
    return m_scaleTransform.p();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimGeoMechView::fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue)
{
    RimView::fieldChangedByUi(changedField, oldValue, newValue);

    if (changedField == &showWindow)
    {
        if (showWindow)
        {
            bool generateDisplayModel = (viewer() == NULL);
            updateViewerWidget();
            if (generateDisplayModel)
            {
                scheduleCreateDisplayModelAndRedraw();
            }
        }
        else
        {
            if (m_viewer)
            {
                RiuMainWindow::instance()->removeViewer(m_viewer);
                delete m_viewer;
                m_viewer = NULL;
            }
        }

        this->updateUiIconFromToggleField();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimGeoMechView::initAfterRead()
{
    this->cellResult()->setReservoirView(this);

    this->updateUiIconFromToggleField();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimCase* RimGeoMechView::ownerCase()
{
    return m_geomechCase;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimGeoMechView::scheduleGeometryRegen(unsigned short geometryType)
{
    m_vizLogic->scheduleGeometryRegen(geometryType);
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivElmVisibilityCalculator::computeAllVisible(cvf::UByteArray* elmVisibilities, const RigFemPart* femPart)
{
    elmVisibilities->resize(femPart->elementCount());
    elmVisibilities->setAll(true);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivElmVisibilityCalculator::computeRangeVisibility(cvf::UByteArray* elmVisibilities, RigFemPart* femPart, 
                                                        const cvf::CellRangeFilter& rangeFilter)
{

    elmVisibilities->resize(femPart->elementCount());
    
    const RigFemPartGrid* grid = femPart->structGrid();
 
    for (int elmIdx = 0; elmIdx < femPart->elementCount(); ++elmIdx)
    {
        size_t mainGridI;
        size_t mainGridJ;
        size_t mainGridK;

        grid->ijkFromCellIndex(elmIdx, &mainGridI, &mainGridJ, &mainGridK);
        (*elmVisibilities)[elmIdx] = rangeFilter.isCellVisible(mainGridI, mainGridJ, mainGridK, false);
    }

}
