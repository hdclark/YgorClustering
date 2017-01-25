

#ifndef YGOR_CLUSTERING_HELPERS_HPP
#define YGOR_CLUSTERING_HELPERS_HPP

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
#include <functional>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

#include <boost/geometry/index/parameters.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/io/svg/svg_mapper.hpp>


//This is a helper function that applies a user function to each datum in the R*-tree. It is needed
// because prior to Boost.Geometry version 1.58.0 there were no rtree::begin()/end() members, so 
// you needed to (inconveniently) specify a query function that returned unanimously true for all 
// datum. This routine simply wraps that approach and gives something like a for(it_t & it : data).
//
// User functions should generally be of the sort:
//     std::function<void(const typename RTree_t::const_query_iterator &)>;
// but can safely return anything they want (which will be ignored).
//
template < typename RTree_t,  //A Boost.Geometry R*-tree, specifically.
           typename ClusteringDatum_t,
           typename IterOperation_t > //A function that operates on Boost.Geometry (const) iterators.
void OnEachDatum( RTree_t & RTree,
                  IterOperation_t OpFunc ){

    //(Needed to work around missing RTree_t.begin()/end() when Boost.Geometry version < 1.58.0.)
    constexpr auto RTreeQueryGetAll = [](const ClusteringDatum_t &) -> bool { return true; };
       
    typename RTree_t::const_query_iterator it;
    it = RTree.qbegin(boost::geometry::index::satisfies( RTreeQueryGetAll ));
    for( ; it != RTree.qend(); ++it){
        OpFunc(it);
    }

    return;
}


//This helper function is a quick and dirty way to get a count (and unique list of) the ClusterIDs 
// present in an R*-tree.
template < typename RTree_t,  //A Boost.Geometry R*-tree, specifically.
           typename ClusteringDatum_t >
std::map< ClusterID<typename ClusteringDatum_t::ClusterIDType_>, size_t> GetClusterIDCounts( RTree_t & RTree ){
    std::map< ClusterID<typename ClusteringDatum_t::ClusterIDType_>, size_t> Counts;
    auto Counter = [&Counts](const typename RTree_t::const_query_iterator &it) -> void {
        Counts[it->CID] += 1; 
        return;
    };
    OnEachDatum<RTree_t,ClusteringDatum_t,decltype(Counter)>(RTree, Counter);
    return Counts;
}



#endif //YGOR_CLUSTERING_HELPERS_HPP
