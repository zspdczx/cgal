// Copyright (c) 2011, 2015 GeometryFactory (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0+
//
//
// Author(s)     : Sebastien Loriot and Andreas Fabri

#ifndef CGAL_POLYGON_MESH_PROCESSING_CONNECTED_COMPONENTS_H
#define CGAL_POLYGON_MESH_PROCESSING_CONNECTED_COMPONENTS_H

#include <CGAL/license/Polygon_mesh_processing/connected_components.h>

#include <CGAL/disable_warnings.h>

#include<set>
#include<vector>

#include <CGAL/boost/graph/named_function_params.h>
#include <CGAL/boost/graph/helpers.h>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/property_map/vector_property_map.hpp>

#include <CGAL/boost/graph/iterator.h>
#include <CGAL/boost/graph/helpers.h>
#include <CGAL/boost/graph/Face_filtered_graph.h>
#include <CGAL/boost/graph/copy_face_graph.h>

#include <CGAL/assertions.h>
#include <CGAL/tuple.h>
#include <CGAL/boost/graph/Dual.h>
#include <CGAL/boost/graph/helpers.h>
#include <CGAL/Default.h>

#include <CGAL/Polygon_mesh_processing/internal/named_function_params.h>
#include <CGAL/Polygon_mesh_processing/internal/named_params_helper.h>

#ifdef DOXYGEN_RUNNING
#define CGAL_PMP_NP_TEMPLATE_PARAMETERS NamedParameters
#define CGAL_PMP_NP_CLASS NamedParameters
#endif


namespace CGAL {

namespace Polygon_mesh_processing{

  namespace internal {
    struct MoreSecond {
      typedef std::pair<std::size_t,std::size_t> T;
      bool operator()(const T& a, const T& b) const {
        return a.second > b.second;
      }
    };

    // A property map 
    template <typename G>
    struct No_constraint {
      friend bool get(No_constraint<G>, typename boost::graph_traits<G>::edge_descriptor)
      {
        return false;
      }
    };

    // A functor
    template <typename G, typename EdgeConstraintMap = No_constraint<G> >
    struct No_border {
      No_border()
      {}

      No_border(const G & g, EdgeConstraintMap ecm = EdgeConstraintMap())
        : g(&g), ecm(ecm)
      {}

      bool operator()(typename boost::graph_traits<G>::edge_descriptor e) const {
        if (!is_border(e, *g)){
          return !get(ecm, e);
        }
        return false;
      }

      const G* g;
      EdgeConstraintMap ecm;
    };

}// namespace internal

/*!
 * \ingroup keep_connected_components_grp
 *  discovers all the faces in the same connected component as `seed_face` and records them in `out`.
 * `seed_face` will also be added in `out`.
 *
 *  \tparam PolygonMesh a model of `FaceGraph`
 *  \tparam FaceOutputIterator a model of `OutputIterator` that accepts
        faces of type
        `boost::graph_traits<PolygonMesh>::%face_descriptor`.
 *  \tparam NamedParameters a sequence of \ref pmp_namedparameters "Named Parameters"
 *
 *  \param seed_face a face of `pmesh` from which exploration starts to detect the connected component
           that contains it
 *  \param pmesh the polygon mesh
 *  \param out the output iterator that collects faces from the same connected component as `seed_face`
 *  \param np optional \ref pmp_namedparameters "Named Parameters" described below
 *
 * \cgalNamedParamsBegin
 *    \cgalParamBegin{edge_is_constrained_map}  a property map containing the constrained-or-not status of each edge of `pmesh` \cgalParamEnd
 * \cgalNamedParamsEnd
 *
 *  \returns the output iterator.
 *
 */
template <typename PolygonMesh
          , typename FaceOutputIterator
          , typename NamedParameters
          >
FaceOutputIterator
connected_component(typename boost::graph_traits<PolygonMesh>::face_descriptor seed_face
                    , const PolygonMesh& pmesh
                    , FaceOutputIterator out
                    , const NamedParameters& np)
{
  using boost::choose_param;
  using boost::get_param;

  typedef typename boost::lookup_named_param_def <
    internal_np::edge_is_constrained_t,
    NamedParameters,
    internal::No_constraint<PolygonMesh>//default
  > ::type                                               EdgeConstraintMap;
  EdgeConstraintMap ecmap
    = choose_param(get_param(np, internal_np::edge_is_constrained),
                   internal::No_constraint<PolygonMesh>());

  typedef typename boost::graph_traits<PolygonMesh>::face_descriptor face_descriptor;
  typedef typename boost::graph_traits<PolygonMesh>::halfedge_descriptor halfedge_descriptor;
  std::set<face_descriptor> already_processed;
  std::vector< face_descriptor > stack;
  stack.push_back(seed_face);
  while (!stack.empty())
    {
      seed_face=stack.back();
      stack.pop_back();
      if (!already_processed.insert(seed_face).second) continue;
      *out++=seed_face;
      for(halfedge_descriptor hd :
                    CGAL::halfedges_around_face(halfedge(seed_face, pmesh), pmesh) )
      {
        if(! get(ecmap, edge(hd, pmesh))){
          face_descriptor neighbor = face( opposite(hd, pmesh), pmesh );
          if ( neighbor != boost::graph_traits<PolygonMesh>::null_face() )
            stack.push_back(neighbor);
        }
      }
    }
  return out;
}

template <typename PolygonMesh, typename OutputIterator>
OutputIterator
connected_component(typename boost::graph_traits<PolygonMesh>::face_descriptor seed_face,
                    const PolygonMesh& pmesh,
                    OutputIterator out)
{
  return connected_component(seed_face, pmesh, out,
          CGAL::Polygon_mesh_processing::parameters::all_default());
}

/*!
 * \ingroup keep_connected_components_grp
 *  computes for each face the index of the corresponding connected component.
 *
 *  A property map for `CGAL::face_index_t` must be either available as an internal property map 
 *  to `pmesh` or provided as one of the \ref pmp_namedparameters "Named Parameters".
 *
 *  \tparam PolygonMesh a model of `FaceListGraph`
 *  \tparam FaceComponentMap a model of `WritablePropertyMap` with
        `boost::graph_traits<PolygonMesh>::%face_descriptor` as key type and
        `boost::graph_traits<PolygonMesh>::%faces_size_type` as value type.
 *  \tparam NamedParameters a sequence of \ref pmp_namedparameters "Named Parameters"

 * \param pmesh the polygon mesh
 * \param fcm the property map with indices of components associated to faces in `pmesh`
 * \param np optional \ref pmp_namedparameters "Named Parameters" described below
 *
 * \cgalNamedParamsBegin
 *    \cgalParamBegin{edge_is_constrained_map} a property map containing the constrained-or-not status of each edge of `pmesh` \cgalParamEnd
 *    \cgalParamBegin{face_index_map} a property map containing the index of each face of `pmesh` \cgalParamEnd
 * \cgalNamedParamsEnd
 *
 *  \returns the number of connected components.
 */

template <typename PolygonMesh
        , typename FaceComponentMap
        , typename NamedParameters
>
typename boost::property_traits<FaceComponentMap>::value_type
connected_components(const PolygonMesh& pmesh,
                     FaceComponentMap fcm,
                     const NamedParameters& np)
{
  using boost::choose_param;
  using boost::get_param;

  typedef typename boost::lookup_named_param_def <
    internal_np::edge_is_constrained_t,
    NamedParameters,
    internal::No_constraint<PolygonMesh>//default
  > ::type                                               EdgeConstraintMap;
  EdgeConstraintMap ecmap
    = choose_param(get_param(np, internal_np::edge_is_constrained),
                   internal::No_constraint<PolygonMesh>());

  typedef Dual<PolygonMesh>                              Dual;
  typedef boost::filtered_graph<Dual,
    internal::No_border<PolygonMesh,EdgeConstraintMap> > FiniteDual;
  Dual dual(pmesh);

  FiniteDual finite_dual(dual,
    internal::No_border<PolygonMesh, EdgeConstraintMap>(pmesh, ecmap));

  typename GetFaceIndexMap<PolygonMesh, NamedParameters>::const_type
    fimap = choose_param(get_param(np, internal_np::face_index),
                         get_const_property_map(boost::face_index, pmesh));

  return boost::connected_components(finite_dual,
    fcm,
    boost::vertex_index_map(fimap)
  );
}

template <typename PolygonMesh, typename FaceComponentMap>
typename boost::property_traits<FaceComponentMap>::value_type
connected_components(const PolygonMesh& pmesh,
                     FaceComponentMap fcm)
{

  return CGAL::Polygon_mesh_processing::connected_components(pmesh, fcm,
    CGAL::Polygon_mesh_processing::parameters::all_default());
}


template <typename PolygonMesh
        , typename ComponentRange
        , typename FaceComponentMap
        , typename NamedParameters>
void keep_connected_components(PolygonMesh& pmesh
                              , const ComponentRange& components_to_keep
                              , const FaceComponentMap& fcm
                              , const NamedParameters& np);

/*!
 * \ingroup keep_connected_components_grp
 *  removes the small connected components and all isolated vertices.
 *  Keep `nb_components_to_keep` largest connected components. 
 *
 * Property maps for `CGAL::face_index_t` and `CGAL::vertex_index_t`
 * must be either available as internal property maps 
 * to `pmesh` or provided as \ref pmp_namedparameters "Named Parameters".
 *
 * \tparam PolygonMesh a model of `FaceListGraph` and `MutableFaceGraph`
 * \tparam NamedParameters a sequence of \ref pmp_namedparameters "Named Parameters"
 *
 * \param pmesh the polygon mesh
 * \param nb_components_to_keep the number of components to be kept
 * \param np optional \ref pmp_namedparameters "Named Parameters", amongst those described below
 *
 * \cgalNamedParamsBegin
 *    \cgalParamBegin{edge_is_constrained_map} a property map containing the constrained-or-not status of each edge of `pmesh` \cgalParamEnd
 *    \cgalParamBegin{face_index_map} a property map containing the index of each face of `pmesh` \cgalParamEnd
 *    \cgalParamBegin{vertex_index_map} a property map containing the index of each vertex of `pmesh` \cgalParamEnd
 * \cgalNamedParamsEnd
 *
 *  \return the number of connected components removed (ignoring isolated vertices).
 */
template <typename PolygonMesh
        , typename NamedParameters>
std::size_t keep_largest_connected_components(PolygonMesh& pmesh
                                            , std::size_t nb_components_to_keep
                                            , const NamedParameters& np)
{
  typedef PolygonMesh PM;
  typedef typename boost::graph_traits<PM>::face_descriptor face_descriptor;

  using boost::choose_param;
  using boost::get_param;

  //FaceIndexMap
  typedef typename GetFaceIndexMap<PM, NamedParameters>::type FaceIndexMap;
  FaceIndexMap fimap = choose_param(get_param(np, internal_np::face_index),
                                    get_property_map(boost::face_index, pmesh));

  //vector_property_map
  boost::vector_property_map<std::size_t, FaceIndexMap> face_cc(fimap);
  std::size_t num = connected_components(pmesh, face_cc, np);

  // Even even we do not want to keep anything we need to first
  // calculate the number of existing connected_components to get the
  // correct return value.
  if(nb_components_to_keep == 0) {
    CGAL::clear(pmesh);
    return num;
  }

  if((num == 1)|| (nb_components_to_keep > num) )
    return 0;

  std::vector< std::pair<std::size_t, std::size_t> > component_size(num);

  for(std::size_t i=0; i < num; i++)
    component_size[i] = std::make_pair(i,0);

  for(face_descriptor f : faces(pmesh))
    ++component_size[face_cc[f]].second;

  // we sort the range [0, num) by component size
  std::sort(component_size.begin(), component_size.end(), internal::MoreSecond());
  std::vector<std::size_t> cc_to_keep;
  for(std::size_t i=0; i<nb_components_to_keep; ++i)
    cc_to_keep.push_back( component_size[i].first );

  keep_connected_components(pmesh, cc_to_keep, face_cc, np);

  return num - nb_components_to_keep;
}

template <typename PolygonMesh>
std::size_t keep_largest_connected_components(PolygonMesh& pmesh,
                                              std::size_t nb_components_to_keep)
{
  return keep_largest_connected_components(pmesh,
    nb_components_to_keep,
    CGAL::Polygon_mesh_processing::parameters::all_default());
}

/*!
 * \ingroup keep_connected_components_grp
 *  removes connected components with less than a given number of faces.
 *
 * Property maps for `CGAL::face_index_t` and `CGAL::vertex_index_t`
 * must be either available as internal property maps 
 * to `pmesh` or provided as \ref pmp_namedparameters "Named Parameters".
 *
 * \tparam PolygonMesh a model of `FaceListGraph` and `MutableFaceGraph`
 * \tparam NamedParameters a sequence of \ref pmp_namedparameters "Named Parameters"
 *
 * \param pmesh the polygon mesh
 * \param threshold_components_to_keep the number of faces a component must have so that it is kept
 * \param np optional \ref pmp_namedparameters "Named Parameters", amongst those described below
 *
 * \cgalNamedParamsBegin
 *    \cgalParamBegin{edge_is_constrained_map} a property map containing the constrained-or-not status of each edge of `pmesh` \cgalParamEnd
 *    \cgalParamBegin{face_index_map} a property map containing the index of each face of `pmesh` \cgalParamEnd
 *    \cgalParamBegin{vertex_index_map} a property map containing the index of each vertex of `pmesh` \cgalParamEnd
 * \cgalNamedParamsEnd
 *
 *  \return the number of connected components removed (ignoring isolated vertices).
 */
template <typename PolygonMesh
        , typename NamedParameters>
std::size_t keep_large_connected_components(PolygonMesh& pmesh
                                            , std::size_t threshold_components_to_keep
                                            , const NamedParameters& np)
{
  typedef PolygonMesh PM;
  typedef typename boost::graph_traits<PM>::face_descriptor face_descriptor;

  using boost::choose_param;
  using boost::get_param;

  //FaceIndexMap
  typedef typename GetFaceIndexMap<PM, NamedParameters>::type FaceIndexMap;
  FaceIndexMap fim = choose_param(get_param(np, internal_np::face_index),
                                  get_property_map(boost::face_index, pmesh));

  //vector_property_map
  boost::vector_property_map<std::size_t, FaceIndexMap> face_cc(fim);
  std::size_t num = connected_components(pmesh, face_cc, np);
  std::vector< std::pair<std::size_t, std::size_t> > component_size(num);

  for(std::size_t i=0; i < num; i++)
    component_size[i] = std::make_pair(i,0);

  for(face_descriptor f : faces(pmesh))
    ++component_size[face_cc[f]].second;


  std::vector<std::size_t> cc_to_keep;
  for(std::size_t i=0; i<num; ++i){
    if(component_size[i].second >= threshold_components_to_keep){
      cc_to_keep.push_back( component_size[i].first );
    }
  }

  keep_connected_components(pmesh, cc_to_keep, face_cc, np);

  return num - cc_to_keep.size();
}


template <typename PolygonMesh>
std::size_t keep_large_connected_components(PolygonMesh& pmesh,
                                            std::size_t threshold_components_to_keep)
{
  return keep_large_connected_components(pmesh,
    threshold_components_to_keep,
    CGAL::Polygon_mesh_processing::parameters::all_default());
}


template <typename PolygonMesh
        , typename ComponentRange
        , typename FaceComponentMap
        , typename NamedParameters>
void keep_or_remove_connected_components(PolygonMesh& pmesh
                                        , const ComponentRange& components_to_keep
                                        , const FaceComponentMap& fcm
                                        , bool  keep
                                        , const NamedParameters& np)
{
  typedef PolygonMesh PM;
  using boost::choose_param;
  using boost::get_param;

  typedef typename boost::graph_traits<PolygonMesh>::face_descriptor   face_descriptor;
  typedef typename boost::graph_traits<PolygonMesh>::face_iterator     face_iterator;
  typedef typename boost::graph_traits<PolygonMesh>::vertex_descriptor vertex_descriptor;
  typedef typename boost::graph_traits<PolygonMesh>::vertex_iterator   vertex_iterator;
  typedef typename boost::graph_traits<PolygonMesh>::halfedge_descriptor halfedge_descriptor;
  typedef typename boost::graph_traits<PolygonMesh>::edge_descriptor   edge_descriptor;
  typedef typename boost::graph_traits<PolygonMesh>::edge_iterator     edge_iterator;

  //VertexIndexMap
  typedef typename GetVertexIndexMap<PM, NamedParameters>::type VertexIndexMap;
  VertexIndexMap vim = choose_param(get_param(np, internal_np::vertex_index),
                                    get_const_property_map(boost::vertex_index, pmesh));

  std::set<std::size_t> cc_to_keep;
  for(std::size_t i : components_to_keep)
    cc_to_keep.insert(i);

  boost::vector_property_map<bool, VertexIndexMap> keep_vertex(vim);
  for(vertex_descriptor v : vertices(pmesh)){
    keep_vertex[v] = false;
  }
  for(face_descriptor f : faces(pmesh)){
    if (cc_to_keep.find(get(fcm,f)) != cc_to_keep.end())
      put(fcm, f, keep ? 1 : 0);
    else
      put(fcm, f, keep ? 0 : 1);
  }

  for(face_descriptor f : faces(pmesh)){
    if (get(fcm, f) == 1){
      for(halfedge_descriptor h : halfedges_around_face(halfedge(f, pmesh), pmesh)){
        vertex_descriptor v = target(h, pmesh);
        keep_vertex[v] = true;
      }
    }
  }

  edge_iterator eb, ee;
  for (boost::tie(eb, ee) = edges(pmesh); eb != ee;)
  {
    edge_descriptor e = *eb;
    ++eb;
    vertex_descriptor v = source(e, pmesh);
    vertex_descriptor w = target(e, pmesh);
    halfedge_descriptor h = halfedge(e, pmesh);
    halfedge_descriptor oh = opposite(h, pmesh);
    if (!keep_vertex[v] && !keep_vertex[w]){
      // don't care about connectivity
      // As vertices are not kept the faces and vertices will be removed later
      remove_edge(e, pmesh);
    }
    else if (keep_vertex[v] && keep_vertex[w]){
      face_descriptor fh = face(h, pmesh), ofh = face(oh, pmesh);
      if (is_border(h, pmesh) && is_border(oh, pmesh)){
#ifdef CGAL_CC_DEBUG
        std::cerr << "null_face on both sides of " << e << " is kept\n";
#endif
      }
      else if ((is_border(oh, pmesh) && get(fcm,fh)) ||
        (is_border(h, pmesh) && get(fcm,ofh)) ||
        (!is_border(oh, pmesh) && !is_border(h, pmesh) && get(fcm,fh) && get(fcm,ofh))){
        // do nothing
      }
      else if (!is_border(h, pmesh) && get(fcm,fh) && !is_border(oh, pmesh) && !get(fcm,ofh)){
        set_face(oh, boost::graph_traits<PolygonMesh>::null_face(), pmesh);
      }
      else if (!is_border(h, pmesh) && !get(fcm,fh) && !is_border(oh, pmesh) && get(fcm,ofh)){
        set_face(h, boost::graph_traits<PolygonMesh>::null_face(), pmesh);
      }
      else {
        // no face kept
        CGAL_assertion((is_border(h, pmesh) || !get(fcm,fh)) && (is_border(oh, pmesh) || !get(fcm,ofh)));
        // vertices pointing to e must change their halfedge
        if (halfedge(v, pmesh) == oh){
          set_halfedge(v, prev(h, pmesh), pmesh);
        }
        if (halfedge(w, pmesh) == h){
          set_halfedge(w, prev(oh, pmesh), pmesh);
        }
        // shortcut the next pointers as e will be removed
        set_next(prev(h, pmesh), next(oh, pmesh), pmesh);
        set_next(prev(oh, pmesh), next(h, pmesh), pmesh);
        remove_edge(e, pmesh);
      }
    }
    else if (keep_vertex[v]){
      if (halfedge(v, pmesh) == oh){
        set_halfedge(v, prev(h, pmesh), pmesh);
      }
      set_next(prev(h, pmesh), next(oh, pmesh), pmesh);
      remove_edge(e, pmesh);
    }
    else {
      CGAL_assertion(keep_vertex[w]);
      if (halfedge(w, pmesh) == h){
        set_halfedge(w, prev(oh, pmesh), pmesh);
      }
      set_next(prev(oh, pmesh), next(h, pmesh), pmesh);
      remove_edge(e, pmesh);
    }
  }

  face_iterator fb, fe;
  // We now can remove all vertices and faces not marked as kept
  for (boost::tie(fb, fe) = faces(pmesh); fb != fe;){
    face_descriptor f = *fb;
    ++fb;
    if (get(fcm,f) != 1){
      remove_face(f, pmesh);
    }
  }
  vertex_iterator b, e;
  for (boost::tie(b, e) = vertices(pmesh); b != e;){
    vertex_descriptor v = *b;
    ++b;
    if (!keep_vertex[v]){
      remove_vertex(v, pmesh);
    }
  }
}

/*!
* \ingroup keep_connected_components_grp
* keeps the connected components designated by theirs ids in `components_to_keep`,
* and removes the other connected components as well as all isolated vertices.
* The connected component id of a face is given by `fcm`.
*
* \note If the removal of the connected components makes `pmesh` a non-manifold surface,
* then the behavior of this function is undefined.
*
* Property maps for `CGAL::vertex_index_t`
* must be either available as internal property map
* to `pmesh` or provided as \ref pmp_namedparameters "Named Parameters".
*
* \tparam PolygonMesh a model of `FaceListGraph` and `MutableFaceGraph`
* \tparam NamedParameters a sequence of \ref pmp_namedparameters "Named Parameters"
* \tparam ComponentRange a range of ids convertible to `std::size`
* \tparam FaceComponentMap a model of `ReadWritePropertyMap` with
*         `boost::graph_traits<PolygonMesh>::%face_descriptor` as key type and
*         `boost::graph_traits<PolygonMesh>::%faces_size_type` as value type.
*
* \param components_to_keep the range of ids of connected components to keep
* \param pmesh the polygon mesh
* \param fcm the property map with indices of components associated to faces in `pmesh`.
*        After calling this function, the values of `fcm` are undefined.
* \param np optional \ref pmp_namedparameters "Named Parameters" described below
*
* \cgalNamedParamsBegin
*    \cgalParamBegin{vertex_index_map} a property map containing the index of each vertex of `pmesh` \cgalParamEnd
* \cgalNamedParamsEnd
*
*/
template <typename PolygonMesh
        , typename ComponentRange
        , typename FaceComponentMap
        , typename NamedParameters>
void keep_connected_components(PolygonMesh& pmesh
                              , const ComponentRange& components_to_keep
                              , const FaceComponentMap& fcm
                              , const NamedParameters& np)
{
  keep_or_remove_connected_components(pmesh, components_to_keep, fcm, true, np);
}

/*!
* \ingroup keep_connected_components_grp
* Removes in `pmesh` the connected components designated by theirs ids
* in `components_to_remove` as well as all isolated vertices.
* The connected component id of a face is given by `fcm`.
*
* \note If the removal of the connected components makes `pmesh` a non-manifold surface,
* then the behavior of this function is undefined.
*
* Property maps for `CGAL::vertex_index_t`
* must be either available as internal property map
* to `pmesh` or provided as \ref pmp_namedparameters "Named Parameters".
*
*
* \tparam PolygonMesh a model of `FaceListGraph` and `MutableFaceGraph`
* \tparam NamedParameters a sequence of \ref pmp_namedparameters "Named Parameters"
* \tparam ComponentRange a range of ids convertible to `std::size`
* \tparam FaceComponentMap a model of `ReadWritePropertyMap` with
*         `boost::graph_traits<PolygonMesh>::%face_descriptor` as key type and
*         `boost::graph_traits<PolygonMesh>::%faces_size_type` as value type.
*
* \param components_to_remove the range of ids of connected components to remove
* \param pmesh the polygon mesh
* \param fcm the property map with indices of components associated to faces in `pmesh`.
*        After calling this function, the values of `fcm` are undefined.
* \param np optional \ref pmp_namedparameters "Named Parameters" described below
*
* \cgalNamedParamsBegin
*    \cgalParamBegin{vertex_index_map} a property map containing the index of each vertex of `pmesh` \cgalParamEnd
* \cgalNamedParamsEnd
*
*/
template <typename PolygonMesh
        , typename ComponentRange
        , typename FaceComponentMap
        , typename NamedParameters>
void remove_connected_components(PolygonMesh& pmesh
                                , const ComponentRange& components_to_remove
                                , const FaceComponentMap& fcm
                                , const NamedParameters& np)
{
  if (components_to_remove.empty()) return;
  keep_or_remove_connected_components(pmesh, components_to_remove, fcm, false, np);
}

/*!
* \ingroup keep_connected_components_grp
*  keeps the connected components not designated by the faces in `components_to_remove`,
*  and removes the other connected components and all isolated vertices.
*
* Property maps for `CGAL::face_index_t` and `CGAL::vertex_index_t`
* must be either available as internal property maps
* to `pmesh` or provided as \ref pmp_namedparameters "Named Parameters".
*
* \note If the removal of the connected components makes `pmesh` a non-manifold surface,
* then the behavior of this function is undefined.
*
* \tparam PolygonMesh a model of `FaceListGraph` and `MutableFaceGraph`
* \tparam NamedParameters a sequence of \ref pmp_namedparameters "Named Parameters"
* \tparam FaceRange a range of `boost::graph_traits<PolygonMesh>::%face_descriptor`
*         indicating the connected components to be removed.
*
* \param components_to_remove a face range, including one face or more on each component to be removed
* \param pmesh the polygon mesh
* \param np optional \ref pmp_namedparameters "Named Parameters", amongst those described below
*
* \cgalNamedParamsBegin
*    \cgalParamBegin{edge_is_constrained_map} a property map containing the constrained-or-not status of each edge of `pmesh` \cgalParamEnd
*    \cgalParamBegin{face_index_map} a property map containing the index of each face of `pmesh` \cgalParamEnd
*    \cgalParamBegin{vertex_index_map} a property map containing the index of each vertex of `pmesh` \cgalParamEnd
* \cgalNamedParamsEnd
*
*/
template <typename PolygonMesh
        , typename FaceRange
        , typename CGAL_PMP_NP_TEMPLATE_PARAMETERS>
void remove_connected_components(PolygonMesh& pmesh
                                , const FaceRange& components_to_remove
                                , const CGAL_PMP_NP_CLASS& np)
{
  if (components_to_remove.empty()) return;
  typedef PolygonMesh PM;
  typedef typename boost::graph_traits<PM>::face_descriptor face_descriptor;
  using boost::choose_param;
  using boost::get_param;

  //FaceIndexMap
  typedef typename GetFaceIndexMap<PM, CGAL_PMP_NP_CLASS>::type FaceIndexMap;
  FaceIndexMap fim = choose_param(get_param(np, internal_np::face_index),
                                  get_property_map(boost::face_index, pmesh));

  //vector_property_map
  boost::vector_property_map<std::size_t, FaceIndexMap> face_cc(fim);

  connected_components(pmesh, face_cc, np);

  std::vector<std::size_t> cc_to_remove;
  for(face_descriptor f : components_to_remove)
    cc_to_remove.push_back( face_cc[f] );

  remove_connected_components(pmesh, cc_to_remove, face_cc, np);
}

/*!
* \ingroup keep_connected_components_grp
*  keeps the connected components designated by the faces in `components_to_keep`,
*  and removes the other connected components and all isolated vertices.
*
* Property maps for `CGAL::face_index_t` and `CGAL::vertex_index_t`
* must be either available as internal property maps
* to `pmesh` or provided as \ref pmp_namedparameters "Named Parameters".
*
* \note If the removal of the connected components makes `pmesh` a non-manifold surface,
* then the behavior of this function is undefined.
*
* \tparam PolygonMesh a model of `FaceListGraph` and `MutableFaceGraph`
* \tparam NamedParameters a sequence of \ref pmp_namedparameters "Named Parameters"
* \tparam FaceRange a range of `boost::graph_traits<PolygonMesh>::%face_descriptor`
*         indicating the connected components to be kept.
*
* \param pmesh the polygon mesh
* \param components_to_keep a face range, including one face or more on each component to be kept
* \param np optional \ref pmp_namedparameters "Named Parameters", amongst those described below
*
* \cgalNamedParamsBegin
*    \cgalParamBegin{edge_is_constrained_map} a property map containing the constrained-or-not status of each edge of `pmesh` \cgalParamEnd
*    \cgalParamBegin{face_index_map} a property map containing the index of each face of `pmesh` \cgalParamEnd
*    \cgalParamBegin{vertex_index_map} a property map containing the index of each vertex of `pmesh` \cgalParamEnd
* \cgalNamedParamsEnd
*
*/
template <typename PolygonMesh
        , typename FaceRange
        , typename CGAL_PMP_NP_TEMPLATE_PARAMETERS>
void keep_connected_components(PolygonMesh& pmesh
                             , const FaceRange& components_to_keep
                             , const CGAL_PMP_NP_CLASS& np)
{
  typedef PolygonMesh PM;
  typedef typename boost::graph_traits<PM>::face_descriptor face_descriptor;

  using boost::choose_param;
  using boost::get_param;

  //FaceIndexMap
  typedef typename GetFaceIndexMap<PM, CGAL_PMP_NP_CLASS>::type FaceIndexMap;
  FaceIndexMap fim = choose_param(get_param(np, internal_np::face_index),
                                  get_property_map(boost::face_index, pmesh));

  //vector_property_map
  boost::vector_property_map<std::size_t, FaceIndexMap> face_cc(fim);

  connected_components(pmesh, face_cc, np);

  std::vector<std::size_t> cc_to_keep;
  for(face_descriptor f : components_to_keep)
    cc_to_keep.push_back( face_cc[f] );

  keep_connected_components(pmesh, cc_to_keep, face_cc, np);
}

// non-documented overloads so that named parameters can be omitted

template <typename PolygonMesh, typename FaceRange>
void remove_connected_components(PolygonMesh& pmesh
                                , const FaceRange& components_to_remove)
{
  remove_connected_components(pmesh, components_to_remove,
    CGAL::Polygon_mesh_processing::parameters::all_default());
}

template <typename PolygonMesh
        , typename ComponentRange
        , typename FaceComponentMap>
void keep_connected_components(PolygonMesh& pmesh
                              , const ComponentRange& components_to_keep
                              , const FaceComponentMap& fcm)
{
  keep_connected_components(pmesh, components_to_keep, fcm,
    CGAL::Polygon_mesh_processing::parameters::all_default());
}

template <typename PolygonMesh
        , typename ComponentRange
        , typename FaceComponentMap>
void remove_connected_components(PolygonMesh& pmesh
                                , const ComponentRange& components_to_remove
                                , const FaceComponentMap& fcm )
{
    remove_connected_components(pmesh, components_to_remove, fcm,
      CGAL::Polygon_mesh_processing::parameters::all_default());
}

template <typename PolygonMesh, typename FaceRange>
void keep_connected_components(PolygonMesh& pmesh
                             , const FaceRange& components_to_keep)
{
  keep_connected_components(pmesh, components_to_keep,
    CGAL::Polygon_mesh_processing::parameters::all_default());
}


namespace internal{
template <class MapFromNP, class Default_tag, class Dynamic_tag, class Mesh>
std::pair<MapFromNP , bool>
get_map(MapFromNP m, Default_tag, Dynamic_tag, Mesh&)
{
  return std::make_pair(m, false);
}

template <class Default_tag, class Dynamic_tag, class Mesh>
std::pair<typename boost::property_map<Mesh, Default_tag >::type, bool>
get_map(boost::param_not_found, Default_tag t, Dynamic_tag , Mesh& m)
{

  return std::make_pair(get(t,m), boost::is_same<Default_tag, Dynamic_tag>::value) ;
}

template <class TriangleMesh, class OutputIterator, class FIMap, class VIMap, class HIMap >
OutputIterator split_connected_components_impl(
    std::pair<FIMap, bool> fim,//pair(map, need_init)
    std::pair<HIMap, bool> him,//pair(map, need_init)
    std::pair<VIMap, bool> vim,//pair(map, need_init)
    OutputIterator out,
    const TriangleMesh& tm
    )
{
  if(fim.second)
  {
    std::size_t id = 0;
    for(auto f : faces(tm))
    {
      put(fim.first, f, id++);
    }
  }
  if(him.second)
  {
    std::size_t id = 0;
    for(auto h : halfedges(tm))
    {
      put(him.first, h, id++);
    }
  }
  if(vim.second)
  {
    std::size_t id = 0;
    for(auto v : vertices(tm))
    {
      put(vim.first, v, id++);
    }
  }
  typename boost::template property_map<TriangleMesh, CGAL::dynamic_face_property_t<int > >::const_type
      pidmap = get(CGAL::dynamic_face_property_t<int>(), tm);

  int nb_patches = CGAL::Polygon_mesh_processing::connected_components(tm, pidmap, CGAL::parameters::face_index_map(fim.first));

  for(int i=0; i<nb_patches; ++i)
  {
    CGAL::Face_filtered_graph<TriangleMesh, FIMap, VIMap, HIMap>
        filter_graph(tm, i, pidmap,
                     CGAL::parameters::face_index_map(fim.first)
                     .halfedge_index_map(him.first)
                     .vertex_index_map(vim.first));
    TriangleMesh new_graph;
    CGAL::copy_face_graph(filter_graph, new_graph);
    *out++ = new_graph;
  }
  return out;
}
}//internal

template <class TriangleMesh, class OutputIterator, class NamedParameters>
OutputIterator split_connected_components(TriangleMesh& tm,
                                OutputIterator out,
                                          const NamedParameters& np)
{
  typedef typename boost::mpl::if_c<CGAL::graph_has_property<TriangleMesh, CGAL::face_index_t>::value
        , CGAL::face_index_t
        ,CGAL::dynamic_face_property_t<std::size_t >
        >::type FIM_def_tag; //or no _c ?

  typedef typename boost::mpl::if_c<CGAL::graph_has_property<TriangleMesh, CGAL::halfedge_index_t>::value
        , CGAL::halfedge_index_t
        ,CGAL::dynamic_halfedge_property_t<std::size_t >
        >::type HIM_def_tag; //or no _c ?

  typedef typename boost::mpl::if_c<CGAL::graph_has_property<TriangleMesh, boost::vertex_index_t>::value
        , boost::vertex_index_t
        ,CGAL::dynamic_vertex_property_t<std::size_t >
        >::type VIM_def_tag; //or no _c ?

  return internal::split_connected_components_impl(
        internal::get_map(
          get_param(np, internal_np::face_index),
          CGAL::dynamic_face_property_t<std::size_t >(),
          FIM_def_tag(), tm),
       internal::get_map(
          get_param(np, internal_np::halfedge_index),
          CGAL::dynamic_halfedge_property_t<std::size_t >(),
          HIM_def_tag(), tm),
        internal::get_map(
          get_param(np, internal_np::vertex_index),
          CGAL::dynamic_vertex_property_t<std::size_t >(),
          VIM_def_tag(), tm),
        out, tm);

}

} // namespace Polygon_mesh_processing

} // namespace CGAL

#include <CGAL/enable_warnings.h>

#endif //CGAL_POLYGON_MESH_PROCESSING_CONNECTED_COMPONENTS_H
