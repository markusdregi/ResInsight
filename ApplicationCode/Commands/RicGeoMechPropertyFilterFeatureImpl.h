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

#pragma once

#include <vector>
#include <cstddef>

class RimGeoMechPropertyFilter;
class RimGeoMechPropertyFilterCollection;

//==================================================================================================
/// 
//==================================================================================================
class RicGeoMechPropertyFilterFeatureImpl
{
public:
    static std::vector<RimGeoMechPropertyFilter*>              selectedPropertyFilters();
    static std::vector<RimGeoMechPropertyFilterCollection*>    selectedPropertyFilterCollections();

    static void addPropertyFilter(RimGeoMechPropertyFilterCollection* propertyFilterCollection);
    static void insertPropertyFilter(RimGeoMechPropertyFilterCollection* propertyFilterCollection, size_t index);

private:
    static RimGeoMechPropertyFilter* createPropertyFilter(RimGeoMechPropertyFilterCollection* propertyFilterCollection);
    static void setDefaults(RimGeoMechPropertyFilter* propertyFilter);
};