/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017     Statoil ASA
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

#include "RimFracture.h"

#include "RiaApplication.h"

#include "RigCell.h"
#include "RigCellGeometryTools.h"
#include "RigFracture.h"
#include "RigMainGrid.h"
#include "RigTesselatorTools.h"

#include "RimEclipseView.h"
#include "RimEllipseFractureTemplate.h"
#include "RimFractureDefinitionCollection.h"
#include "RimOilField.h"
#include "RimProject.h"
#include "RimView.h"

#include "RivWellFracturePartMgr.h"

#include "cafPdmUiDoubleSliderEditor.h"
#include "cafHexGridIntersectionTools/cafHexGridIntersectionTools.h"

#include "cvfBoundingBox.h"
#include "cvfGeometryTools.h"
#include "cvfMath.h"
#include "cvfMatrix4.h"
#include "cvfPlane.h"

#include "clipper/clipper.hpp"



CAF_PDM_XML_ABSTRACT_SOURCE_INIT(RimFracture, "Fracture");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimFracture::RimFracture(void)
{
    CAF_PDM_InitObject("Fracture", "", "", "");

    CAF_PDM_InitField(&name, "Name", QString("Fracture Name"), "Name", "", "", "");

    CAF_PDM_InitFieldNoDefault(&m_fractureTemplate, "FractureDef", "Fracture Template", "", "", "");

    CAF_PDM_InitFieldNoDefault(&m_anchorPosition, "anchorPosition", "Anchor Position", "", "", "");
    m_anchorPosition.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_uiAnchorPosition, "ui_positionAtWellpath", "Fracture Position", "", "", "");
    m_uiAnchorPosition.registerGetMethod(this, &RimFracture::fracturePositionForUi);
    m_uiAnchorPosition.uiCapability()->setUiReadOnly(true);
    CAF_PDM_InitField(&azimuth, "Azimuth", 0.0, "Azimuth", "", "", "");

    CAF_PDM_InitField(&perforationLength, "PerforationLength", 0.0, "PerforationLength", "", "", "");


    azimuth.uiCapability()->setUiEditorTypeName(caf::PdmUiDoubleSliderEditor::uiEditorTypeName());

    CAF_PDM_InitField(&m_i, "I", 1, "Fracture location cell I", "", "", "");
    m_i.uiCapability()->setUiHidden(true);
    
    CAF_PDM_InitField(&m_j, "J", 1, "Fracture location cell J", "", "", "");
    m_j.uiCapability()->setUiHidden(true);

    CAF_PDM_InitField(&m_k, "K", 1, "Fracture location cell K", "", "", "");
    m_k.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_displayIJK, "Cell_IJK", "Cell IJK", "", "", "");
    m_displayIJK.registerGetMethod(this, &RimFracture::createOneBasedIJK);
    m_displayIJK.uiCapability()->setUiReadOnly(true);

    m_rigFracture = new RigFracture;
    m_recomputeGeometry = true;

    m_rivFracture = new RivWellFracturePartMgr(this);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimFracture::~RimFracture()
{
}
 
//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
const std::vector<cvf::uint>& RimFracture::triangleIndices() const
{
    return m_rigFracture->triangleIndices();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
const std::vector<cvf::Vec3f>& RimFracture::nodeCoords() const
{
    return m_rigFracture->nodeCoords();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<size_t> RimFracture::getPotentiallyFracturedCells()
{
    std::vector<size_t> cellindecies;

    RiaApplication* app = RiaApplication::instance();
    RimView* activeView = RiaApplication::instance()->activeReservoirView();
    if (!activeView) return cellindecies;
 
    RimEclipseView* activeRiv = dynamic_cast<RimEclipseView*>(activeView);
    if (!activeRiv) return cellindecies;

    const RigMainGrid* mainGrid = activeRiv->mainGrid();
    if (!mainGrid) return cellindecies;

    const std::vector<cvf::Vec3f>& nodeCoordVec = nodeCoords();

    if (!hasValidGeometry()) computeGeometry();

    cvf::BoundingBox polygonBBox;
    for (cvf::Vec3f nodeCoord : nodeCoordVec) polygonBBox.add(nodeCoord);

    mainGrid->findIntersectingCells(polygonBBox, &cellindecies);

    return cellindecies;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimFracture::fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue)
{
    if (changedField == &m_fractureTemplate)
    {
        perforationLength = m_fractureTemplate->perforationLength();
        azimuth = m_fractureTemplate->azimuthAngle();
    }

    if (changedField == &azimuth || 
        changedField == &m_fractureTemplate)
    {

        setRecomputeGeometryFlag();

        RimView* rimView = nullptr;
        this->firstAncestorOrThisOfType(rimView);
        if (rimView)
        {
            rimView->createDisplayModelAndRedraw();
        }
        else
        {
            // Can be triggered from well path, find active view
            RimView* activeView = RiaApplication::instance()->activeReservoirView();
            if (activeView)
            {
                activeView->createDisplayModelAndRedraw();
            }
        }

    }

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
caf::PdmFieldHandle* RimFracture::userDescriptionField()
{
    return &name;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimFracture::computeGeometry()
{
    std::vector<cvf::Vec3f> nodeCoords;
    std::vector<cvf::uint>  polygonIndices;

    RimEllipseFractureTemplate* fractureDef = attachedFractureDefinition();
    if (fractureDef )
    {
        fractureDef->fractureGeometry(&nodeCoords, &polygonIndices);
    }

    cvf::Mat4f m = transformMatrix();

    for (cvf::Vec3f& v : nodeCoords)
    {
        v.transformPoint(m);
    }

    m_rigFracture->setGeometry(polygonIndices, nodeCoords); 

    m_recomputeGeometry = false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
cvf::Vec3d RimFracture::anchorPosition()
{
    return m_anchorPosition();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
cvf::Mat4f RimFracture::transformMatrix()
{
    cvf::Vec3d center = anchorPosition();

    // Ellipsis geometry is produced in XY-plane, rotate 90 deg around X to get zero azimuth along Y
    cvf::Mat4f rotationFromTesselator = cvf::Mat4f::fromRotation(cvf::Vec3f::X_AXIS, cvf::Math::toRadians(90.0f));

    // Azimuth rotation
    cvf::Mat4f azimuthRotation = cvf::Mat4f::fromRotation(cvf::Vec3f::Z_AXIS, cvf::Math::toRadians(-azimuth()));

    cvf::Mat4f m = azimuthRotation * rotationFromTesselator;
    m.setTranslation(cvf::Vec3f(center));

    return m;
}



//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimFracture::computeTransmissibility()
{ 
    std::vector<RigFractureData> fracDataVec;

    std::vector<size_t> fracCells = getPotentiallyFracturedCells();

    for (auto fracCell : fracCells)
    {
        //TODO: Remove - only for simplifying debugging...
        RiaApplication* app = RiaApplication::instance();
        RimView* activeView = RiaApplication::instance()->activeReservoirView();
        RimEclipseView* activeRiv = dynamic_cast<RimEclipseView*>(activeView);
        const RigMainGrid* mainGrid = activeRiv->mainGrid();

        size_t i, j, k;
        mainGrid->ijkFromCellIndex(fracCell, &i, &j, &k);

        //End of code only for debugging... 


        std::vector<std::vector<cvf::Vec3d> > planeCellPolygons;
        cvf::Vec3d localX;
        cvf::Vec3d localY;
        cvf::Vec3d localZ;

        bool isPlanIntersected = planeCellIntersectionPolygons(fracCell, planeCellPolygons, localX, localY, localZ);
        
        if (!isPlanIntersected || planeCellPolygons.size()==0) continue;

        //Transform planCell polygon(s) and averageZdirection to x/y coordinate system (where fracturePolygon already is located)
        cvf::Mat4f invertedTransMatrix = transformMatrix().getInverted();
        for (std::vector<cvf::Vec3d> & planeCellPolygon : planeCellPolygons)
        {
            for (cvf::Vec3d& v : planeCellPolygon)
            {
                v.transformPoint(static_cast<cvf::Mat4d>(invertedTransMatrix));
            }
        }


        //TODO: Make copy of z dir vector, we need it both in fracture coords and domain cords
        localZ.transformVector(static_cast<cvf::Mat4d>(invertedTransMatrix));

        RigFractureData fracData; 
        fracData.reservoirCellIndex = fracCell;

        double transmissibility;

        if (attachedFractureDefinition())
        {
            double areaOfCellPlaneFractureOverlap = 0.0;
            std::vector<cvf::Vec3f> fracPolygon = attachedFractureDefinition()->fracturePolygon();

            calculateFracturePlaneCellPolygonOverlapArea(planeCellPolygons, fracPolygon, areaOfCellPlaneFractureOverlap);

            //TODO: get correct input values...
            double fractureLength = 1.2345;
            double flowLength = 2.718281828;

            double c = 0.008527; // TODO: Get value with units, is defined in RimReservoirCellResultsStorage

            //TODO: read permeability from file (should use matrix permeability and not fracture perm)... 
            transmissibility = 8 * c * attachedFractureDefinition()->permeability * areaOfCellPlaneFractureOverlap /
                            ( flowLength + (attachedFractureDefinition()->skinFactor * fractureLength) / cvf::PI_D);
        }
        else transmissibility = cvf::UNDEFINED_DOUBLE;

        fracData.transmissibility = transmissibility;
        //only keep fracData if transmissibility is non-zero
        if (transmissibility > 0)
        {
            fracDataVec.push_back(fracData);
        }

    }

    m_rigFracture->setFractureData(fracDataVec);
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimFracture::planeCellIntersectionPolygons(size_t cellindex, std::vector<std::vector<cvf::Vec3d> > & polygons, 
                                                cvf::Vec3d & localX, cvf::Vec3d & localY, cvf::Vec3d & localZ)
{
    
    cvf::Plane fracturePlane;
    cvf::Mat4f m = transformMatrix();
    bool isCellIntersected = false;

    fracturePlane.setFromPointAndNormal(static_cast<cvf::Vec3d>(m.translation()), 
                                        static_cast<cvf::Vec3d>(m.col(2)));

    RiaApplication* app = RiaApplication::instance();
    RimView* activeView = RiaApplication::instance()->activeReservoirView();
    if (!activeView) return isCellIntersected;

    RimEclipseView* activeRiv = dynamic_cast<RimEclipseView*>(activeView);
    if (!activeRiv) return isCellIntersected;

    const RigMainGrid* mainGrid = activeRiv->mainGrid();
    if (!mainGrid) return isCellIntersected;

    RigCell cell = mainGrid->globalCellArray()[cellindex];
    if (cell.isInvalid()) return isCellIntersected;

    if (cellindex == 186234)
    {
        cvf::Vec3d cellcenter = cell.center();
    }

    //Copied (and adapted) from RigEclipseWellLogExtractor
    cvf::Vec3d hexCorners[8];
    const std::vector<cvf::Vec3d>& nodeCoords = mainGrid->nodes();
    const caf::SizeTArray8& cornerIndices = cell.cornerIndices();

    hexCorners[0] = nodeCoords[cornerIndices[0]];
    hexCorners[1] = nodeCoords[cornerIndices[1]];
    hexCorners[2] = nodeCoords[cornerIndices[2]];
    hexCorners[3] = nodeCoords[cornerIndices[3]];
    hexCorners[4] = nodeCoords[cornerIndices[4]];
    hexCorners[5] = nodeCoords[cornerIndices[5]];
    hexCorners[6] = nodeCoords[cornerIndices[6]];
    hexCorners[7] = nodeCoords[cornerIndices[7]];

    //Find line-segments where cell and fracture plane intersects
    std::list<std::pair<cvf::Vec3d, cvf::Vec3d > > intersectionLineSegments;

    isCellIntersected = RigCellGeometryTools::planeHexCellIntersection(hexCorners, fracturePlane, intersectionLineSegments);
       
    RigCellGeometryTools::createPolygonFromLineSegments(intersectionLineSegments, polygons);

    RigCellGeometryTools::findCellLocalXYZ(hexCorners, localX, localY, localZ);

    return isCellIntersected;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimFracture::calculateFracturePlaneCellPolygonOverlapArea(std::vector<std::vector<cvf::Vec3d> > planeCellPolygons, 
                                                           std::vector<cvf::Vec3f> fracturePolygon, double & areaOfCellPlaneFractureOverlap)
{
    int polygonScaleFactor = 10000; //For transform to clipper int 
    int xInt, yInt;
    cvf::Vec3d areaVector = cvf::Vec3d::ZERO;

    for (std::vector<cvf::Vec3d> planeCellPolygon : planeCellPolygons) 
    {
        //Convert to int for clipper library and store as clipper "path"
        ClipperLib::Path planeCellPath;
        for (cvf::Vec3d& v : planeCellPolygon)
        {
            xInt = v.x()*polygonScaleFactor;
            yInt = v.y()*polygonScaleFactor;
            planeCellPath.push_back(ClipperLib::IntPoint(xInt, yInt));
        }

        ClipperLib::Path fracturePath;
        for (cvf::Vec3f& v : fracturePolygon)
        {
            xInt = v.x()*polygonScaleFactor;
            yInt = v.y()*polygonScaleFactor;
            fracturePath.push_back(ClipperLib::IntPoint(xInt, yInt));
        }

        ClipperLib::Clipper clpr;
        clpr.AddPath(fracturePath, ClipperLib::ptSubject, true);
        clpr.AddPath(planeCellPath, ClipperLib::ptClip, true);

        ClipperLib::Paths solution;
        clpr.Execute(ClipperLib::ctIntersection, solution, ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);

        //Convert back to std::vector<std::vector<cvf::Vec3d> >
        std::vector<std::vector<cvf::Vec3d> > clippedPolygons;
        for (ClipperLib::Path pathInSol : solution)
        {
            std::vector<cvf::Vec3d> clippedPolygon;
            for (ClipperLib::IntPoint IntPosition : pathInSol)
            {
                cvf::Vec3d v = cvf::Vec3d::ZERO; 
                v.x() = (float)IntPosition.X / (float)polygonScaleFactor;
                v.y() = (float)IntPosition.Y / (float)polygonScaleFactor;
                clippedPolygon.push_back(v);
            }
            clippedPolygons.push_back(clippedPolygon);
        }

        for (std::vector<cvf::Vec3d> areaPolygon : clippedPolygons)
        {
            areaVector = cvf::GeometryTools::polygonAreaNormal3D(areaPolygon);
            areaOfCellPlaneFractureOverlap += areaVector.length();
        }
    }   
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimFracture::setRecomputeGeometryFlag()
{
    m_recomputeGeometry = true;

    m_rivFracture->clearGeometryCache();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimFracture::isRecomputeGeometryFlagSet()
{
    return m_recomputeGeometry;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
cvf::Vec3d RimFracture::fracturePositionForUi() const
{
    cvf::Vec3d v = m_anchorPosition;

    v.z() = -v.z();

    return v;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimFracture::createOneBasedIJK() const
{
    return QString("Cell : [%1, %2, %3]").arg(m_i + 1).arg(m_j + 1).arg(m_k + 1);
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimFracture::calculateValueOptions(const caf::PdmFieldHandle* fieldNeedingOptions, bool * useOptionsOnly)
{
    QList<caf::PdmOptionItemInfo> options;

    RimProject* proj = RiaApplication::instance()->project();
    CVF_ASSERT(proj);

    RimOilField* oilField = proj->activeOilField();
    if (oilField == nullptr) return options;

    if (fieldNeedingOptions == &m_fractureTemplate)
    {
        RimFractureDefinitionCollection* fracDefColl = oilField->fractureDefinitionCollection();
        if (fracDefColl == nullptr) return options;

        for (RimEllipseFractureTemplate* fracDef : fracDefColl->fractureDefinitions())
        {
            options.push_back(caf::PdmOptionItemInfo(fracDef->name(), fracDef));
        }
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimFracture::defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering)
{
    uiOrdering.add(&name);

    caf::PdmUiGroup* geometryGroup = uiOrdering.addNewGroup("Properties");
    geometryGroup->add(&azimuth);
    geometryGroup->add(&perforationLength);
    geometryGroup->add(&m_fractureTemplate);

    caf::PdmUiGroup* fractureCenterGroup = uiOrdering.addNewGroup("Fracture Center Info");
    fractureCenterGroup->add(&m_uiAnchorPosition);
    fractureCenterGroup->add(&m_displayIJK);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimFracture::defineEditorAttribute(const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute * attribute)
{
    if (field == &azimuth)
    {
        caf::PdmUiDoubleSliderEditorAttribute* myAttr = dynamic_cast<caf::PdmUiDoubleSliderEditorAttribute*>(attribute);
        if (myAttr)
        {
            myAttr->m_minimum = 0;
            myAttr->m_maximum = 360;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimFracture::setAnchorPosition(const cvf::Vec3d& pos)
{
    m_anchorPosition = pos;
    setRecomputeGeometryFlag();

    // Update ijk
    {
        std::vector<size_t> cellindecies;

        RiaApplication* app = RiaApplication::instance();
        RimView* activeView = RiaApplication::instance()->activeReservoirView();
        if (!activeView) return;

        RimEclipseView* activeRiv = dynamic_cast<RimEclipseView*>(activeView);
        if (!activeRiv) return;

        const RigMainGrid* mainGrid = activeRiv->mainGrid();
        if (!mainGrid) return;

        cvf::BoundingBox polygonBBox;
        polygonBBox.add(m_anchorPosition);

        mainGrid->findIntersectingCells(polygonBBox, &cellindecies);

        if (cellindecies.size() > 0)
        {
            size_t i = cvf::UNDEFINED_SIZE_T;
            size_t j = cvf::UNDEFINED_SIZE_T;
            size_t k = cvf::UNDEFINED_SIZE_T;

            size_t gridCellIndex = cellindecies[0];

            if (mainGrid->ijkFromCellIndex(gridCellIndex, &i, &j, &k))
            {
                m_i = static_cast<int>(i);
                m_j = static_cast<int>(j);
                m_k = static_cast<int>(k);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
const RigFracture* RimFracture::attachedRigFracture() const
{
    CVF_ASSERT(m_rigFracture.notNull());

    return m_rigFracture.p();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimFracture::setFractureTemplate(RimEllipseFractureTemplate* fractureTemplate)
{
    m_fractureTemplate = fractureTemplate;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimEllipseFractureTemplate* RimFracture::attachedFractureDefinition() const
{
    return m_fractureTemplate();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RivWellFracturePartMgr* RimFracture::fracturePartManager()
{
    CVF_ASSERT(m_rivFracture.notNull());

    return m_rivFracture.p();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimFracture::hasValidGeometry() const
{
    if (m_recomputeGeometry) return false;

    return (nodeCoords().size() > 0 && triangleIndices().size() > 0);
}
