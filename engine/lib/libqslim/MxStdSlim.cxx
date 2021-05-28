/************************************************************************

  Surface simplification using quadric error metrics

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: MxStdSlim.cxx,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

#include "stdmix.h"
#include "MxStdSlim.h"

MxStdSlim::MxStdSlim(MxStdModel *m0)
    : heap(64)
{
    m = m0;

    // Externally visible variables
    placement_policy = MX_PLACE_OPTIMAL;
    weighting_policy = MX_WEIGHT_AREA;
    boundary_weight = 1000.0;
    compactness_ratio = 0.0;
    meshing_penalty = 1.0;
    local_validity_threshold = 0.0;
    vertex_degree_limit = 24;
    will_join_only = false;

    valid_faces = 0;
    valid_verts = 0;
    is_initialized = false;

    uint i;
    for(i=0; i<m->face_count(); i++) if(m->face_is_valid(i))  valid_faces++;
    for(i=0; i<m->vert_count(); i++) if(m->vertex_is_valid(i))  valid_verts++;
}
