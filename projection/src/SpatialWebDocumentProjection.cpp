#include "SpatialWebDocument.h"
#include "SpatialWebWorld.h"

SpatialInterior SpatialWebDocument::room() const
{
    return SpatialWebWorld::build(*this);
}
