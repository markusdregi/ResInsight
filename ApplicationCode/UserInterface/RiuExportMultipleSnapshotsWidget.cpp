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

#include "RiuExportMultipleSnapshotsWidget.h"

#include "RiaApplication.h"

#include "RimCase.h"
#include "RimMultiSnapshotDefinition.h"
#include "RimProject.h"
#include "RimView.h"

#include "cafCmdFeatureManager.h"
#include "cafPdmUiTableView.h"
#include "cafSelectionManager.h"

#include <QFileDialog>
#include <QAbstractItemView>
#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTableView>
#include <QToolButton>
#include <QWidget>


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RiuExportMultipleSnapshotsWidget::RiuExportMultipleSnapshotsWidget(QWidget* parent, RimProject* project)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    m_rimProject(project)
{
    setWindowTitle("Export Multiple Snapshots");

    int nWidth = 800;
    int nHeight = 300;
    resize(nWidth, nHeight);

    QVBoxLayout* dialogLayout = new QVBoxLayout;
    setLayout(dialogLayout);

    m_pdmTableView = new caf::PdmUiTableView(this);
    m_pdmTableView->tableView()->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_pdmTableView->tableView()->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_pdmTableView->tableView(), SIGNAL(customContextMenuRequested(QPoint)), SLOT(customMenuRequested(QPoint)));

    m_pdmTableView->setListField(&(project->multiSnapshotDefinitions()));
    m_pdmTableView->tableView()->resizeRowsToContents();
    m_pdmTableView->tableView()->resizeColumnsToContents();

    // Set active child array to be able to use generic delete
    caf::SelectionManager::instance()->setActiveChildArrayFieldHandle(&(project->multiSnapshotDefinitions()));

    dialogLayout->addWidget(m_pdmTableView);

    // Export folder
    {
        QHBoxLayout* layout = new QHBoxLayout;

        layout->addWidget(new QLabel("Export folder"));

        // Save images in snapshot catalog relative to project directory
        QString snapshotFolderName = RiaApplication::instance()->createAbsolutePathFromProjectRelativePath("snapshots");

        m_lineEdit = new QLineEdit(snapshotFolderName);

        QToolButton* button = new QToolButton;
        button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred));
        button->setText(QLatin1String("..."));

        layout->addWidget(m_lineEdit);
        layout->addWidget(button);

        connect(button, SIGNAL(clicked()), this, SLOT(folderSelectionClicked()));

        dialogLayout->addLayout(layout);
    }


    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QPushButton* exportButton = new QPushButton(tr("Export"));
    exportButton->setDefault(true);
    buttonBox->addButton(exportButton, QDialogButtonBox::ActionRole);
    connect(exportButton, SIGNAL(clicked()), this, SLOT(exportSnapshots()));

    dialogLayout->addWidget(buttonBox);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RiuExportMultipleSnapshotsWidget::~RiuExportMultipleSnapshotsWidget()
{
    m_pdmTableView->setListField(nullptr);

    caf::SelectionManager::instance()->setActiveChildArrayFieldHandle(nullptr);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RiuExportMultipleSnapshotsWidget::customMenuRequested(QPoint pos)
{
    caf::CmdFeatureManager* commandManager = caf::CmdFeatureManager::instance();

    QMenu menu;
    menu.addAction(commandManager->action("PdmListField_DeleteItem","Delete row"));

    QAction* newRowAction = new QAction("New row", this);
    connect(newRowAction, SIGNAL(triggered()), SLOT(addSnapshotItem()));
    menu.addAction(newRowAction);

    QAction* clearAllRows = new QAction("Clear", this);
    connect(clearAllRows, SIGNAL(triggered()), SLOT(deleteAllSnapshotItems())); 
    menu.addAction(clearAllRows);
    
    
    // Qt doc: QAbstractScrollArea and its subclasses that map the context menu event to coordinates of the viewport().
    QPoint globalPos = m_pdmTableView->tableView()->viewport()->mapToGlobal(pos);

    menu.exec(globalPos);
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RiuExportMultipleSnapshotsWidget::deleteAllSnapshotItems()
{
    if (!m_rimProject) return;

    m_rimProject->multiSnapshotDefinitions.deleteAllChildObjects();

    m_rimProject->multiSnapshotDefinitions.uiCapability()->updateConnectedEditors();
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RiuExportMultipleSnapshotsWidget::exportSnapshots()
{
    // TODO: wire up call of static method
    // RicExportMultipleSnapshotsFeature::staticMethod()
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RiuExportMultipleSnapshotsWidget::folderSelectionClicked()
{
    QString defaultPath = m_lineEdit->text();

    QString directoryPath = QFileDialog::getExistingDirectory(m_lineEdit,
        tr("Get existing directory"),
        defaultPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!directoryPath.isEmpty())
    {
        m_lineEdit->setText(directoryPath);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RiuExportMultipleSnapshotsWidget::addSnapshotItem()
{
    if (!m_rimProject) return;

    RimMultiSnapshotDefinition* multiSnapshot = new RimMultiSnapshotDefinition();


    //Getting default value from last entered line: 
    if (m_rimProject->multiSnapshotDefinitions.size() > 0)
    {
        RimMultiSnapshotDefinition* other = m_rimProject->multiSnapshotDefinitions[m_rimProject->multiSnapshotDefinitions.size() - 1];

        multiSnapshot->caseObject = other->caseObject();
        multiSnapshot->viewObject = other->viewObject();
        multiSnapshot->timeStepStart = other->timeStepStart();
        multiSnapshot->timeStepEnd = other->timeStepEnd();


        // Variant using copy based on xml string
//         QString copyOfOriginalObject = other->writeObjectToXmlString();
//         multiSnapshot->readObjectFromXmlString(copyOfOriginalObject, caf::PdmDefaultObjectFactory::instance());


    }

    else
    {
        RimProject* proj = RiaApplication::instance()->project();
        std::vector<RimCase*> cases;
        proj->allCases(cases);
        RimCase* CaseExample = nullptr;
        RimView* ViewExample = nullptr;

        if (cases.size() > 0)
        {
            CaseExample = cases.at(0);
            multiSnapshot->caseObject = CaseExample;
 
            std::vector<RimView*> viewExamples;
            viewExamples = CaseExample->views();

            if (viewExamples.size() > 0)
            {
                ViewExample = viewExamples.at(0);
                multiSnapshot->viewObject = ViewExample;
            }
        }


    }

        


    m_rimProject->multiSnapshotDefinitions.push_back(multiSnapshot);
    m_rimProject->multiSnapshotDefinitions.uiCapability()->updateConnectedEditors();

    if (m_rimProject->multiSnapshotDefinitions.size() == 1)
    {
        m_pdmTableView->tableView()->resizeRowsToContents();
        m_pdmTableView->tableView()->resizeColumnsToContents();
    }
}