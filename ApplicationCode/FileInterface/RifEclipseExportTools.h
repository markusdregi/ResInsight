/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017-     Statoil ASA
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
#include "cvfObject.h"
#include "cvfLibCore.h"

#include "ert/ecl/ecl_kw.h"

#include <map>
#include <QString>


class RimEclipseWellCollection;
class RimSimWellFracture;
class RimWellPathCollection;
class RimWellPathFracture;
class QFile;




//==================================================================================================
//
// Class for access to Eclipse "keyword" files using libecl
//
//==================================================================================================
class RifEclipseExportTools 
{
public:
    RifEclipseExportTools();
    virtual ~RifEclipseExportTools();


    static bool     writeSimWellFracturesToTextFile(const QString& fileName, RimEclipseWellCollection* wellColl);
    static bool     writeWellPathFracturesToTextFile(const QString& fileName, RimWellPathCollection* wellColl);


private:
    static void     writeSimWellFractureDataToTextFile(QFile* file, const std::vector<RimSimWellFracture*>& fractures);
    static void     writeWellPathFractureDataToTextFile(QFile* file, const std::vector<RimWellPathFracture*>& fractures);


};