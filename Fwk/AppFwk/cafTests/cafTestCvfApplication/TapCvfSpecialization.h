#pragma once


#include "cvfBase.h"
#include "cvfColor3.h"
#include "cvfVector3.h"
#include "cvfMatrix4.h"

#include "cafPdmObject.h"
#include "cafPdmField.h"

#include "cafPdmFieldCvfColor.h"
#include "cafPdmFieldCvfVec3d.h"
#include "cafPdmFieldCvfMat4d.h"




class TapCvfSpecialization : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;
public:

    TapCvfSpecialization();

    caf::PdmField<QString> m_testField;

    caf::PdmField<cvf::Color3f> m_colorField;
    caf::PdmField<cvf::Vec3d>  m_vectorField;
    caf::PdmField<cvf::Mat4d>  m_matrixField;

};
