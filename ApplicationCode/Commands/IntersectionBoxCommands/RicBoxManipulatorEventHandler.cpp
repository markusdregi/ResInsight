
#include "RicBoxManipulatorEventHandler.h"

#include "cafBoxManipulatorPartManager.h"
#include "cafEffectGenerator.h"
#include "cafViewer.h"

#include "cvfCamera.h"
#include "cvfDrawableGeo.h"
#include "cvfHitItemCollection.h"
#include "cvfModelBasicList.h"
#include "cvfPart.h"
#include "cvfRay.h"

#include <QDebug>
#include <QMouseEvent>

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RicBoxManipulatorEventHandler::RicBoxManipulatorEventHandler(caf::Viewer* viewer)
    : m_viewer(viewer),
    m_currentPartIndex(cvf::UNDEFINED_SIZE_T)
{
    m_partManager = new caf::BoxManipulatorPartManager;
    m_model = new cvf::ModelBasicList;

    m_viewer->installEventFilter(this);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RicBoxManipulatorEventHandler::~RicBoxManipulatorEventHandler()
{
    m_viewer->removeEventFilter(this);

    // Make sure the model owned by this manipulator is not used anywhere else
    CVF_ASSERT(m_model->refCount() == 1);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicBoxManipulatorEventHandler::setOrigin(const cvf::Vec3d& origin)
{
    m_partManager->setOrigin(origin);

    updateParts();
    emit notifyRedraw();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicBoxManipulatorEventHandler::setSize(const cvf::Vec3d& size)
{
    m_partManager->setSize(size);
    
    updateParts();
    emit notifyRedraw();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
cvf::Model* RicBoxManipulatorEventHandler::model()
{
    return m_model.p();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RicBoxManipulatorEventHandler::eventFilter(QObject *obj, QEvent* inputEvent)
{
    if (inputEvent->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(inputEvent);

        if (mouseEvent->button() == Qt::LeftButton)
        {
            cvf::HitItemCollection hitItems;
            if (m_viewer->rayPick(mouseEvent->x(), mouseEvent->y(), &hitItems))
            {
                if (hitItems.firstItem() && hitItems.firstItem()->part())
                {
                    m_currentPartIndex = m_partManager->partIndexFromSourceInfo(hitItems.firstItem()->part(), hitItems.firstItem()->intersectionPoint());

                    if (m_currentPartIndex != cvf::UNDEFINED_SIZE_T)
                    {
                        updateParts();
                        emit notifyRedraw();
                        
                        return true;
                    }
                }
            }
        }
    }
    else if (inputEvent->type() == QEvent::MouseMove)
    {
        if (m_currentPartIndex != cvf::UNDEFINED_SIZE_T)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(inputEvent);

            //qDebug() << "Inside mouse move";
            //qDebug() << mouseEvent->pos();

            int translatedMousePosX = mouseEvent->pos().x();
            int translatedMousePosY = m_viewer->height() - mouseEvent->pos().y();

            cvf::ref<cvf::Ray> ray = m_viewer->mainCamera()->rayFromWindowCoordinates(translatedMousePosX, translatedMousePosY);
            {
                m_partManager->updateFromPartIndexAndRay(m_currentPartIndex, ray.p());

                updateParts();

                cvf::Vec3d origin;
                cvf::Vec3d size;
                m_partManager->originAndSize(&origin, &size);

                emit notifyUpdate(origin, size);

                emit notifyRedraw();

                return true;
            }
        }
    }
    else if (inputEvent->type() == QEvent::MouseButtonRelease)
    {
        if (m_currentPartIndex != cvf::UNDEFINED_SIZE_T)
        {
            m_currentPartIndex = cvf::UNDEFINED_SIZE_T;

            cvf::Vec3d origin;
            cvf::Vec3d size;
            m_partManager->originAndSize(&origin, &size);

            emit notifyUpdate(origin, size);

            return true;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicBoxManipulatorEventHandler::updateParts()
{
    m_model->removeAllParts();

    m_partManager->appendPartsToModel(m_model.p());
}
