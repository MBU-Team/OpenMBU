#include "atlas/core/atlasTOCAggregator.h"
#include "atlas/runtime/atlasInstanceTexTOC.h"

#ifdef TORQUE_DEBUG
// This forces the aggregator to compile.
void atlas_toc_aggregator_test_function()
{
   AtlasTOCAggregator<AtlasInstanceTexTOC> foo;
   foo.initialize(NULL, 1, 0);
   foo.getStubs(0, Point2I(1,1), NULL);
   foo.getResourceStubs(0, Point2I(1,1), NULL);
//   foo.cancelLoadRequest(0, Point2I(1,1), 0);
   foo.cancelAllLoadRequests(0);
//   foo.requestLoad(0, Point2I(1,1), 0, 1.0);
}
#endif