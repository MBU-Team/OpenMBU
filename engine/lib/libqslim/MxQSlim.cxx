/************************************************************************

Surface simplification using quadric error metrics

Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

$Id: MxQSlim.cxx,v 1.2 2006/09/07 00:05:52 bramage Exp $

************************************************************************/


const unsigned char bitsSetTable256[] =
{
   0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
      1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
      1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
      1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
      3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
      4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

#include "MxQMetric.h"
#include "MxQMetric3.h"
#include "MxQSlim.h"
#include "geom3d.h"

typedef MxQuadric3 Quadric;

MxQSlim::MxQSlim(MxStdModel& _m)
: MxStdSlim(&_m),
quadrics(_m.vert_count()),
planarConstraints(_m.vert_count())
{
   // Externally visible variables
   object_transform = NULL;
}

void MxQSlim::initialize()
{
   collect_quadrics();
   if( boundary_weight > 0.0 )
      constrain_boundaries();
   if( object_transform )
      transform_quadrics(*object_transform);

   is_initialized = true;
}

void MxQSlim::collect_quadrics()
{
   uint j;

   for(j=0; j<quadrics.length(); j++)
      quadrics(j).clear();

   for(MxFaceID i=0; i<m->face_count(); i++)
   {
      MxFace& f = m->face(i);

      Vec3 v1(m->vertex(f(0)));
      Vec3 v2(m->vertex(f(1)));
      Vec3 v3(m->vertex(f(2)));

      Vec4 p = (weighting_policy==MX_WEIGHT_RAWNORMALS) ?
         triangle_raw_plane<Vec3,Vec4>(v1, v2, v3):
      triangle_plane<Vec3,Vec4>(v1, v2, v3);
      Quadric Q(p[X], p[Y], p[Z], p[W], m->compute_face_area(i));

      switch( weighting_policy )
      {
      case MX_WEIGHT_ANGLE:
         for(j=0; j<3; j++)
         {
            Quadric Q_j = Q;
            Q_j *= m->compute_corner_angle(i, j);
            quadrics(f[j]) += Q_j;
         }
         break;
      case MX_WEIGHT_AREA:
      case MX_WEIGHT_AREA_AVG:
         Q *= Q.area();
         // no break: fallthrough
      default:
         quadrics(f[0]) += Q;
         quadrics(f[1]) += Q;
         quadrics(f[2]) += Q;
         break;
      }
   }
}

void MxQSlim::transform_quadrics(const Mat4& P)
{
   for(uint j=0; j<quadrics.length(); j++)
      quadrics(j).transform(P);
}

void MxQSlim::discontinuity_constraint(MxVertexID i, MxVertexID j,
                                       const MxFaceList& faces)
{
   for(uint f=0; f<faces.length(); f++)
   {
      Vec3 org(m->vertex(i)), dest(m->vertex(j));
      Vec3 e = dest - org;

      Vec3 n;
      m->compute_face_normal(faces[f], n);

      Vec3 n2 = e ^ n;
      unitize(n2);

      MxQuadric3 Q(n2, -(n2*org));
      Q *= boundary_weight;

      if( weighting_policy == MX_WEIGHT_AREA ||
         weighting_policy == MX_WEIGHT_AREA_AVG )
      {
         Q.set_area(norm2(e));
         Q *= Q.area();
      }

      quadrics(i) += Q;
      quadrics(j) += Q;
   }
}

void MxQSlim::constrain_boundaries()
{
   MxVertexList star;
   MxFaceList faces;

   for(MxVertexID i=0; i<m->vert_count(); i++)
   {
      star.reset();
      m->collect_vertex_star(i, star);

      for(uint j=0; j<star.length(); j++)
         if( i < star(j) )
         {
            faces.reset();
            m->collect_edge_neighbors(i, star(j), faces);
            if( faces.length() == 1 )
               discontinuity_constraint(i, star(j), faces);
         }
   }
}

bool MxQSlim::can_collapse_edge(MxVertexID i, MxVertexID j,
                                int &collapseType, bool &forceEndPoint, MxVertexID &endPoint)
{
   // We only occasionally set this, so let's clear by default.
   forceEndPoint = false;

   // Get our constraints.
   const unsigned char constraintsI = planarConstraints(i);
   const unsigned char constraintsJ = planarConstraints(j);

   // No constraints means we can freely collapse.
   if(constraintsI == 0 && constraintsJ == 0)
      return true;

   // If they're equal, then we can do the merge. (they're on same
   // edge or corner) But only on the same line!
   if(constraintsI == constraintsJ)
   {
      collapseType = MX_PLACE_LINE;
      return true;
   }

   // Now, we have several cases - partially dependent on counts.
   const int iCount = bitsSetTable256[constraintsI];
   const int jCount = bitsSetTable256[constraintsJ];

   const bool isOneZero = iCount != 0 || jCount != 0;
   const bool hasOverlap = constraintsI & constraintsJ;

   // If they are both corners (or worse) and not MATCHING, we cannot collapse.
   if(iCount > 1  && jCount > 1 && constraintsI != constraintsJ)
      return false;

   // Ok, let the one with more bits set be the anchored one. We always
   // collapse to the one with higher priority. We can only collapse on
   // subsets, ie, if corner to edge, the corner must include all the edge
   // bits, if 2-corner to 3-corner, the 3-corner must include both of the
   // planes that made up the 2-corner.

   // e.g. i is a corner, j is an edge.
   if(iCount > jCount && constraintsI & constraintsJ == constraintsJ)
   {
      collapseType = MX_PLACE_ENDPOINTS;
      forceEndPoint = true;
      endPoint = i;
      return true;
   }

   if(jCount > iCount && constraintsI & constraintsJ == constraintsI)
   {
      collapseType = MX_PLACE_ENDPOINTS;
      forceEndPoint = true;
      endPoint = j;
      return true;
   }

   // Nope, no dice.
   return false;
}



MxEdgeQSlim::MxEdgeQSlim(MxStdModel& _m)
: MxQSlim(_m),
edge_links(_m.vert_count())
{
   contraction_callback = NULL;
}

MxEdgeQSlim::~MxEdgeQSlim()
{
   // Delete everything remaining in the heap
   for(uint i=0; i<heap.size(); i++)
      delete ((MxQSlimEdge *)heap.item(i));
}

///////////////////////////////////////////////////////////////////////////
//
// IMPORTANT NOTE:  These check_xxx functions assume that the local
//                  neighborhoods have been marked so that each face
//                  is marked with the number of involved vertices it has.
//

double MxEdgeQSlim::check_local_compactness(uint v1, uint/*v2*/,
                                            const float *vnew)
{
   const MxFaceList& N1 = m->neighbors(v1);
   double c_min = 1.0;

   for(uint i=0; i<N1.length(); i++)
      if( m->face_mark(N1[i]) == 1 )
      {
         const MxFace& f = m->face(N1[i]);
         Vec3 f_after[3];
         for(uint j=0; j<3; j++)
            f_after[j] = (f[j]==v1)?Vec3(vnew):Vec3(m->vertex(f[j]));

         double c=triangle_compactness(f_after[0], f_after[1], f_after[2]);

         if( c < c_min ) c_min = c;
      }

      return c_min;
}

double MxEdgeQSlim::check_local_inversion(uint v1,uint/*v2*/,const float *vnew)
{
   double Nmin = 1.0;
   const MxFaceList& N1 = m->neighbors(v1);

   for(uint i=0; i<N1.length(); i++)
      if( m->face_mark(N1[i]) == 1 )
      {
         const MxFace& f = m->face(N1[i]);
         Vec3 n_before;
         m->compute_face_normal(N1[i], n_before);

         Vec3 f_after[3];
         for(uint j=0; j<3; j++)
            f_after[j] = (f[j]==v1)?Vec3(vnew):Vec3(m->vertex(f[j]));

         double delta = n_before *
            triangle_normal(f_after[0], f_after[1], f_after[2]);

         if( delta < Nmin ) Nmin = delta;
      }

      return Nmin;
}

uint MxEdgeQSlim::check_local_validity(uint v1, uint /*v2*/, const float *vnew)

{
   const MxFaceList& N1 = m->neighbors(v1);
   uint nfailed = 0;
   uint i;

   for(i=0; i<N1.length(); i++)
      if( m->face_mark(N1[i]) == 1 )
      {
         MxFace& f = m->face(N1[i]);
         uint k = f.find_vertex(v1);
         uint x = f[(k+1)%3];
         uint y = f[(k+2)%3];

         float d_yx[3], d_vx[3], d_vnew[3], f_n[3], n[3];
         mxv_sub(d_yx, m->vertex(y),  m->vertex(x), 3);   // d_yx = y-x
         mxv_sub(d_vx, m->vertex(v1), m->vertex(x), 3);   // d_vx = v-x
         mxv_sub(d_vnew, vnew, m->vertex(x), 3);          // d_vnew = vnew-x

         mxv_cross3(f_n, d_yx, d_vx);
         mxv_cross3(n, f_n, d_yx);     // n = ((y-x)^(v-x))^(y-x)
         mxv_unitize(n, 3);

         // assert( mxv_dot(d_vx, n, 3) > -FEQ_EPS );
         if(mxv_dot(d_vnew,n,3)<local_validity_threshold*mxv_dot(d_vx,n,3))
            nfailed++;
      }

      return nfailed;
}

uint MxEdgeQSlim::check_local_degree(uint v1, uint v2, const float *)
{
   const MxFaceList& N1 = m->neighbors(v1);
   const MxFaceList& N2 = m->neighbors(v2);
   uint i;
   uint degree = 0;

   // Compute the degree of the vertex after contraction
   //
   for(i=0; i<N1.length(); i++)
      if( m->face_mark(N1[i]) == 1 )
         degree++;

   for(i=0; i<N2.length(); i++)
      if( m->face_mark(N2[i]) == 1 )
         degree++;


   if( degree > vertex_degree_limit )
      return degree - vertex_degree_limit;
   else
      return 0;
}

void MxEdgeQSlim::apply_mesh_penalties(MxQSlimEdge *info)
{
   uint i;

   const MxFaceList& N1 = m->neighbors(info->v1);
   const MxFaceList& N2 = m->neighbors(info->v2);

   // Set up the face marks as the check_xxx() functions expect.
   //
   for(i=0; i<N2.length(); i++) m->face_mark(N2[i], 0);
   for(i=0; i<N1.length(); i++) m->face_mark(N1[i], 1);
   for(i=0; i<N2.length(); i++) m->face_mark(N2[i], m->face_mark(N2[i])+1);

   double base_error = info->heap_key();
   double bias = 0.0;

   // Check for excess over degree bounds.
   //
   uint max_degree = MAX(N1.length(), N2.length());
   if( max_degree > vertex_degree_limit )
      bias += (max_degree-vertex_degree_limit) * meshing_penalty * 0.001;

#if ALTERNATE_DEGREE_BIAS
   // ??BUG:  This code was supposed to be a slight improvement over
   //         the earlier version above.  But it performs worse.
   //         Should check into why sometime.
   //
   uint degree_excess = check_local_degree(info->v1, info->v2, info->vnew);
   if( degree_excess )
      bias += degree_excess * meshing_penalty;
#endif

   // Local validity checks
   //
   uint nfailed = check_local_validity(info->v1, info->v2, info->vnew);
   nfailed += check_local_validity(info->v2, info->v1, info->vnew);
   if( nfailed )
      bias += nfailed*meshing_penalty;

   if( compactness_ratio > 0.0 )
   {
      double c1_min=check_local_compactness(info->v1, info->v2, info->vnew);
      double c2_min=check_local_compactness(info->v2, info->v1, info->vnew);
      double c_min = MIN(c1_min, c2_min);

      // !!BUG: There's a small problem with this: it ignores the scale
      //        of the errors when adding the bias.  For instance, enabling
      //        this on the cow produces bad results.  I also tried
      //        += (base_error + FEQ_EPS) * (2-c_min), but that works
      //        poorly on the flat planar thing.  A better solution is
      //        clearly needed.
      //
      //  NOTE: The prior heuristic was
      //        if( ratio*cmin_before > cmin_after ) apply penalty;
      //
      if( c_min < compactness_ratio )
         bias += (1-c_min);
   }

#if USE_OLD_INVERSION_CHECK
   double Nmin1 = check_local_inversion(info->v1, info->v2, info->vnew);
   double Nmin2 = check_local_inversion(info->v2, info->v1, info->vnew);
   if( MIN(Nmin1, Nmin2) < 0.0 )
      bias += meshing_penalty;
#endif

   info->heap_key(base_error - bias);
}

void MxEdgeQSlim::compute_target_placement(MxQSlimEdge *info)
{
   MxVertexID i=info->v1, j=info->v2;

   const Quadric &Qi=quadrics(i), &Qj=quadrics(j);

   Quadric Q = Qi;  Q += Qj;
   double e_min;

   // Take constraints into account...
   int forcePlacement = placement_policy;
   bool forceEnd=false;
   MxVertexID forceEndIdx = 0;
   can_collapse_edge(i, j, forcePlacement, forceEnd, forceEndIdx);

   if( forcePlacement==MX_PLACE_OPTIMAL &&
      Q.optimize(&info->vnew[X], &info->vnew[Y], &info->vnew[Z]) )
   {
      e_min = Q(info->vnew);
   }
   else
   {
      Vec3 vi(m->vertex(i)), vj(m->vertex(j));
      Vec3 best;

      if( placement_policy>=MX_PLACE_LINE && Q.optimize(best, vi, vj) )
         e_min = Q(best);
      else
      {
         // Choose between endpoints...
         double ei=Q(vi), ej=Q(vj);

         if( ei < ej ) { e_min = ei; best = vi; }
         else          { e_min = ej; best = vj; }

         //  And consider midpoint if we have that option.
         if( forcePlacement>=MX_PLACE_ENDORMID )
         {
            Vec3 mid = (vi+vj)/2.0;
            double e_mid = Q(mid);

            if( e_mid < e_min ) { e_min = e_mid; best = mid; }
         }

         // Apply constrained selection if necessary...
         if(forceEnd)
         {
            if(forceEndIdx == i)
            {
               e_min = ei; best = vi;
            }
            else
            {
               e_min = ej; best = vj;
            }
         }
      }

      info->vnew[X] = best[X];
      info->vnew[Y] = best[Y];
      info->vnew[Z] = best[Z];
   }

   if( weighting_policy == MX_WEIGHT_AREA_AVG )
      e_min /= Q.area();

   info->heap_key(-e_min);
}

void MxEdgeQSlim::finalize_edge_update(MxQSlimEdge *info)
{
   // Is this a valid potential collapse? If not, don't insert it.
   int forcePlacement = placement_policy;
   bool forceEnd=false;
   MxVertexID forceEndIdx = 0;
   if(!can_collapse_edge(info->v1, info->v2, forcePlacement, forceEnd, forceEndIdx))
   {
      // Remove it if it was present...
      if(info->is_in_heap())
         heap.remove(info);

      return;
   }

   if( meshing_penalty > 1.0 )
      apply_mesh_penalties(info);

   if( info->is_in_heap() )
      heap.update(info);
   else
      heap.insert(info);
}


void MxEdgeQSlim::compute_edge_info(MxQSlimEdge *info)
{
   compute_target_placement(info);

   finalize_edge_update(info);
}

void MxEdgeQSlim::create_edge(MxVertexID i, MxVertexID j)
{
   MxQSlimEdge *info = new MxQSlimEdge;

   edge_links(i).add(info);
   edge_links(j).add(info);

   info->v1 = i;
   info->v2 = j;

   // Can we collapse this guy at all?

   compute_edge_info(info);
}

void MxEdgeQSlim::collect_edges()
{
   MxVertexList star;

   for(MxVertexID i=0; i<m->vert_count(); i++)
   {
      star.reset();
      m->collect_vertex_star(i, star);

      for(uint j=0; j<star.length(); j++)
         if( i < star(j) )  // Only add particular edge once
            create_edge(i, star(j));
   }
}

void MxEdgeQSlim::initialize()
{
   MxQSlim::initialize();
   collect_edges();
}

void MxEdgeQSlim::initialize(const MxEdge *edges, uint count)
{
   MxQSlim::initialize();
   for(uint i=0; i<count; i++)
      create_edge(edges[i].v1, edges[i].v2);
}

void MxEdgeQSlim::update_pre_contract(const MxPairContraction& conx)
{
   MxVertexID v1=conx.v1, v2=conx.v2;
   uint i, j;

   star.reset();
   //
   // Before, I was gathering the vertex "star" using:
   //      m->collect_vertex_star(v1, star);
   // This doesn't work when we initially begin with a subset of
   // the total edges.  Instead, we need to collect the "star"
   // from the edge links maintained at v1.
   //
   for(i=0; i<edge_links(v1).length(); i++)
      star.add(edge_links(v1)[i]->opposite_vertex(v1));

   for(i=0; i<edge_links(v2).length(); i++)
   {
      MxQSlimEdge *e = edge_links(v2)(i);
      MxVertexID u = (e->v1==v2)?e->v2:e->v1;
      SanityCheck( e->v1==v2 || e->v2==v2 );
      SanityCheck( u!=v2 );

      if( u==v1 || varray_find(star, u) )
      {
         // This is a useless link --- kill it
         bool found = varray_find(edge_links(u), e, &j);
         assert( found );
         edge_links(u).remove(j);
         heap.remove(e);
         if( u!=v1 ) delete e; // (v1,v2) will be deleted later
      }
      else
      {
         // Relink this to v1
         e->v1 = v1;
         e->v2 = u;
         edge_links(v1).add(e);
      }
   }

   edge_links(v2).reset();
}

void MxEdgeQSlim::update_post_contract(const MxPairContraction& conx)
{
}

void MxEdgeQSlim::apply_contraction(const MxPairContraction& conx)
{
   //
   // Pre-contraction update
   valid_verts--;
   valid_faces -= conx.dead_faces.length();
   quadrics(conx.v1) += quadrics(conx.v2);

   update_pre_contract(conx);

   m->apply_contraction(conx);

   update_post_contract(conx);

   // Must update edge info here so that the meshing penalties
   // will be computed with respect to the new mesh rather than the old
   for(uint i=0; i<edge_links(conx.v1).length(); i++)
      compute_edge_info(edge_links(conx.v1)[i]);
}

void MxEdgeQSlim::update_pre_expand(const MxPairContraction&)
{
}

void MxEdgeQSlim::update_post_expand(const MxPairContraction& conx)
{
   MxVertexID v1=conx.v1, v2=conx.v2;
   uint i;

   star.reset(); star2.reset();
   PRECAUTION(edge_links(conx.v2).reset());
   m->collect_vertex_star(conx.v1, star);
   m->collect_vertex_star(conx.v2, star2);

   i = 0;
   while( i<edge_links(v1).length() )
   {
      MxQSlimEdge *e = edge_links(v1)(i);
      MxVertexID u = (e->v1==v1)?e->v2:e->v1;
      SanityCheck( e->v1==v1 || e->v2==v1 );
      SanityCheck( u!=v1 && u!=v2 );

      bool v1_linked = varray_find(star, u);
      bool v2_linked = varray_find(star2, u);

      if( v1_linked )
      {
         if( v2_linked )  create_edge(v2, u);
         i++;
      }
      else
      {
         // !! BUG: I expected this to be true, but it's not.
         //         Need to find out why, and whether it's my
         //         expectation or the code that's wrong.
         // SanityCheck(v2_linked);
         e->v1 = v2;
         e->v2 = u;
         edge_links(v2).add(e);
         edge_links(v1).remove(i);
      }

      compute_edge_info(e);
   }

   if( varray_find(star, v2) )
      // ?? BUG: Is it legitimate for there not to be an edge here ??
      create_edge(v1, v2);
}


void MxEdgeQSlim::apply_expansion(const MxPairContraction& conx)
{
   update_pre_expand(conx);

   m->apply_expansion(conx);

   //
   // Post-expansion update
   valid_verts++;
   valid_faces += conx.dead_faces.length();
   quadrics(conx.v1) -= quadrics(conx.v2);

   update_post_expand(conx);
}

bool MxEdgeQSlim::decimate(uint target)
{
   MxPairContraction local_conx;

   while( valid_faces > target )
   {
      MxQSlimEdge *info = (MxQSlimEdge *)heap.extract();
      if( !info ) { return false; }

      MxVertexID v1=info->v1, v2=info->v2;

      if( m->vertex_is_valid(v1) && m->vertex_is_valid(v2) )
      {
         MxPairContraction& conx = local_conx;

         m->compute_contraction(v1, v2, &conx, info->vnew);

         if( will_join_only && conx.dead_faces.length()>0 ) continue;

         if( contraction_callback )
            (*contraction_callback)(conx, -info->heap_key());

         apply_contraction(conx);
      }

      delete info;
   }

   return true;
}



void MxFaceQSlim::compute_face_info(MxFaceID f)
{
   tri_info& info = f_info(f);
   info.f = f;

   MxVertexID i = m->face(f)(0);
   MxVertexID j = m->face(f)(1);
   MxVertexID k = m->face(f)(2);

   const Quadric& Qi = quadrics(i);
   const Quadric& Qj = quadrics(j);
   const Quadric& Qk = quadrics(k);

   Quadric Q = Qi;
   Q += Qj;
   Q += Qk;

   if( placement_policy == MX_PLACE_OPTIMAL &&
      Q.optimize(&info.vnew[X], &info.vnew[Y], &info.vnew[Z]) )
   {
      info.heap_key(-Q(info.vnew));
   }
   else
   {
      Vec3 v1(m->vertex(i)), v2(m->vertex(j)), v3(m->vertex(k));
      double e1=Q(v1), e2=Q(v2), e3=Q(v3);

      Vec3 best;
      double e_min;

      if( e1<=e2 && e1<=e3 ) { e_min=e1; best=v1; }
      else if( e2<=e1 && e2<=e3 ) { e_min=e2; best=v2; }
      else { e_min=e3; best=v3; }

      info.vnew[X] = best[X];
      info.vnew[Y] = best[Y];
      info.vnew[Z] = best[Z];
      info.heap_key(-e_min);
   }

   if( weighting_policy == MX_WEIGHT_AREA_AVG )
      info.heap_key(info.heap_key() / Q.area());

   if( info.is_in_heap() )
      heap.update(&info);
   else
      heap.insert(&info);
}


MxFaceQSlim::MxFaceQSlim(MxStdModel& _m)
: MxQSlim(_m), f_info(_m.face_count())
{
}

void MxFaceQSlim::initialize()
{
   MxQSlim::initialize();

   for(MxFaceID f=0; f<m->face_count(); f++)
      compute_face_info(f);
}

bool MxFaceQSlim::decimate(uint target)
{
   unsigned int i;

   MxFaceList changed;

   while( valid_faces > target )
   {
      tri_info *info = (tri_info *)heap.extract();
      if( !info ) { return false; }

      MxFaceID f = info->f;
      MxVertexID v1 = m->face(f)(0),
         v2 = m->face(f)(1),
         v3 = m->face(f)(2);

      if( m->face_is_valid(f) )
      {
         //
         // Perform the actual contractions
         m->contract(v1, v2, v3, info->vnew, changed);

         quadrics(v1) += quadrics(v2);  	// update quadric of v1
         quadrics(v1) += quadrics(v3);

         //
         // Update valid counts
         valid_verts -= 2;
         for(i=0; i<changed.length(); i++)
            if( !m->face_is_valid(changed(i)) ) valid_faces--;

         for(i=0; i<changed.length(); i++)
            if( m->face_is_valid(changed(i)) )
               compute_face_info(changed(i));
            else
               heap.remove(&f_info(changed(i)));
      }
   }

   return true;
}
