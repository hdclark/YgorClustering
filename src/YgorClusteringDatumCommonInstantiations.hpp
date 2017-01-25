
#ifndef YGOR_CLUSTERING_CLUSTERINGDATUMCOMMONINSTANTIATIONS_HPP
#define YGOR_CLUSTERING_CLUSTERINGDATUMCOMMONINSTANTIATIONS_HPP

//Copyright Haley Clark 2015.
//
///////////////////////////////////////////////////////////////////////////////
// This file is part of LibYgor.                                             //
//                                                                           //
// LibYgor is free software: you can redistribute it and/or modify           //
// it under the terms of the GNU General Public License as published by      //
// the Free Software Foundation, either version 3 of the License, or         //
// (at your option) any later version.                                       //
//                                                                           //
// LibYgor is distributed in the hope that it will be useful,                //
// but WITHOUT ANY WARRANTY; without even the implied warranty of            //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             //
// GNU General Public License for more details.                              //
//                                                                           //
// You should have received a copy of the GNU General Public License         //
// along with LibYgor.  If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////

/*
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <limits>
#include <utility>
#include <memory>
#include <algorithm>
#include <cassert>
#include <string>
#include <complex>
#include <set>
#include <random>
#include <sstream>
*/

#include <map>
#include <vector>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

#include <boost/geometry/index/parameters.hpp>
#include <boost/geometry/index/rtree.hpp>

//#include <boost/geometry/algorithms/within.hpp>
//#include <boost/geometry/io/svg/svg_mapper.hpp>

#include "YgorClustering.hpp"


constexpr size_t MaxElementsInANode = 6; // 16, 32, 128, 256, ... ?
typedef boost::geometry::index::rstar<MaxElementsInANode> RTreeParameter_t;

//--- 1D double spatial, 0D double attributes, 16bit ClusterIDs, 32bit UserData (for mapping to metadata if needed).
typedef ClusteringDatum<1, double, 0, double, uint16_t, uint32_t> CDat_1d_0f_u16_u32_t;
//typedef boost::geometry::model::box<CDat_1d_0f_u16_u32_t> Box_t;
typedef boost::geometry::index::rtree<CDat_1d_0f_u16_u32_t,RTreeParameter_t> RTree_1d_0f_u16_u32_t;


template std::vector< CDat_1d_0f_u16_u32_t::SpatialType_ >
    DBSCANSortedkDistGraph< RTree_1d_0f_u16_u32_t,
                            CDat_1d_0f_u16_u32_t >( RTree_1d_0f_u16_u32_t &, size_t );


template void DBSCAN< RTree_1d_0f_u16_u32_t,
                      CDat_1d_0f_u16_u32_t >( RTree_1d_0f_u16_u32_t &,
                                              CDat_1d_0f_u16_u32_t::SpatialType_,   
                                              size_t,   
                                              SpatialQueryTechnique );

template void OnEachDatum< RTree_1d_0f_u16_u32_t,
                           CDat_1d_0f_u16_u32_t,
                           std::function<void(const RTree_1d_0f_u16_u32_t::const_query_iterator &)> >(
    RTree_1d_0f_u16_u32_t &,
    std::function<void(const RTree_1d_0f_u16_u32_t::const_query_iterator &)> );

template std::map< ClusterID<typename CDat_1d_0f_u16_u32_t::ClusterIDType_>, size_t>
    GetClusterIDCounts<RTree_1d_0f_u16_u32_t,
                       CDat_1d_0f_u16_u32_t>( RTree_1d_0f_u16_u32_t & );



//--- 2D double spatial, 0D double attributes, 16bit ClusterIDs, 32bit UserData (for mapping to metadata if needed).
typedef ClusteringDatum<2, double, 0, double, uint16_t, uint32_t> CDat_2d_0f_u16_u32_t;
//typedef boost::geometry::model::box<CDat_2d_0f_u16_u32_t> Box_t;
typedef boost::geometry::index::rtree<CDat_2d_0f_u16_u32_t,RTreeParameter_t> RTree_2d_0f_u16_u32_t;


template std::vector< CDat_2d_0f_u16_u32_t::SpatialType_ >
    DBSCANSortedkDistGraph< RTree_2d_0f_u16_u32_t,
                            CDat_2d_0f_u16_u32_t >( RTree_2d_0f_u16_u32_t &, size_t );


template void DBSCAN< RTree_2d_0f_u16_u32_t,
                      CDat_2d_0f_u16_u32_t >( RTree_2d_0f_u16_u32_t &,
                                              CDat_2d_0f_u16_u32_t::SpatialType_,  
                                              size_t,  
                                              SpatialQueryTechnique );
   
template void OnEachDatum< RTree_2d_0f_u16_u32_t,
                           CDat_2d_0f_u16_u32_t,
                           std::function<void(const RTree_2d_0f_u16_u32_t::const_query_iterator &)> >(
    RTree_2d_0f_u16_u32_t &,
    std::function<void(const RTree_2d_0f_u16_u32_t::const_query_iterator &)> );

template std::map< ClusterID<typename CDat_2d_0f_u16_u32_t::ClusterIDType_>, size_t>
    GetClusterIDCounts<RTree_2d_0f_u16_u32_t,
                       CDat_2d_0f_u16_u32_t>( RTree_2d_0f_u16_u32_t & );


//--- 3D double spatial, 0D double attributes, 16bit ClusterIDs, 32bit UserData (for mapping to metadata if needed).
typedef ClusteringDatum<3, double, 0, double, uint16_t, uint32_t> CDat_3d_0f_u16_u32_t;
//typedef boost::geometry::model::box<CDat_3d_0f_u16_u32_t> Box_t;
typedef boost::geometry::index::rtree<CDat_3d_0f_u16_u32_t,RTreeParameter_t> RTree_3d_0f_u16_u32_t;


template std::vector< CDat_3d_0f_u16_u32_t::SpatialType_ >
    DBSCANSortedkDistGraph< RTree_3d_0f_u16_u32_t,
                            CDat_3d_0f_u16_u32_t >( RTree_3d_0f_u16_u32_t &, size_t );


template void DBSCAN< RTree_3d_0f_u16_u32_t,
                      CDat_3d_0f_u16_u32_t >( RTree_3d_0f_u16_u32_t &,
                                              CDat_3d_0f_u16_u32_t::SpatialType_,   
                                              size_t,   
                                              SpatialQueryTechnique );

template void OnEachDatum< RTree_3d_0f_u16_u32_t,
                           CDat_3d_0f_u16_u32_t,
                           std::function<void(const RTree_3d_0f_u16_u32_t::const_query_iterator &)> >(
    RTree_3d_0f_u16_u32_t &,
    std::function<void(const RTree_3d_0f_u16_u32_t::const_query_iterator &)> );

template std::map< ClusterID<typename CDat_3d_0f_u16_u32_t::ClusterIDType_>, size_t>
    GetClusterIDCounts<RTree_3d_0f_u16_u32_t,
                       CDat_3d_0f_u16_u32_t>( RTree_3d_0f_u16_u32_t & );



#endif //YGOR_CLUSTERING_CLUSTERINGDATUMCOMMONINSTANTIATIONS_HPP
