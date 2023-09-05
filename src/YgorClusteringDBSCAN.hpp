

#ifndef YGOR_CLUSTERING_DBSCAN_HPP
#define YGOR_CLUSTERING_DBSCAN_HPP

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
#include <list>
#include <random>
#include <sstream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

#include <boost/geometry/index/parameters.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/io/svg/svg_mapper.hpp>




template < typename RTree_t,  //A Boost.Geometry R*-tree, specifically.
           typename ClusteringDatum_t >
std::vector<typename ClusteringDatum_t::SpatialType_> 
    DBSCANSortedkDistGraph( RTree_t & RTree,
                            size_t k = ClusteringDatum_t::SpatialDimensionCount_ * 2 ){
    // This routine is a companion routine for the DBSCAN implementation provided below. It is from
    //   the same article as the DBSCAN algorithm and provides a means for the user to determine an
    //   appropriate DBSCAN Eps parameter.
    //
    // The procedure is as follows:
    //   1. Provide a 'k' that is identical or close to your intended DBSCAN MinPts parameter. The
    //      authors note that the results using k > 4 do not differ significantly from the k = 4
    //      case. Higher k (i.e., more distant nearest neighbours) will result in more computational 
    //      effort spent finding nearest neighbours.
    //   2. Run this routine before running DBSCAN. The output will be a sorted k-distance graph
    //      where the largest k-distances occur first.
    //   3. Plot the data as follows: y_i v.s. x_i where (y_i = output[i]) and (x_i = i). 
    //   4. Visually examine the graph. Scanning from left (highest k-distance) to right (lowest),
    //      find the "threshold" turning point which is "the first point in the first 'valley'."
    //      To the right of this point, the graph should appear more or less linear. To the left, 
    //      the graph should appear to grow rapidly and may be non-linear in appearance.
    //   5. Record the y_i or k-distance of the "threshold" turning point. Use this number as the
    //      DBSCAN Eps parameter.
    //
    // User parameters:
    //
    // 1. RTree --> The R*-tree already loaded with the data to be clustered.
    // 2. k --> DBSCAN algorithm parameter MinPts (up to around 4 in the 2D case -- play around with
    //          different values.
    //
    // NOTE: No clustering will be performed in this routine. 

    if(k == 0) throw std::runtime_error("Parameter 'k' must be >= 1.");

    std::vector<typename ClusteringDatum_t::SpatialType_> out;
    out.reserve(RTree.size());

    //(Needed to work around missing RTree_t.begin()/end() when Boost.Geometry version < 1.58.0.)
    constexpr auto RTreeSpatialQueryGetAll = [](const ClusteringDatum_t &) -> bool { return true; };

    const std::string ThrowSelfPointCheck = "Spatial vicinity queries should always return the self point."
                                            " This point is missing, indicating numerical stability issues"
                                            " or logical errors in the spatial indexing approach.";
    const std::string ThrowkTooLarge = "Parameter 'k' was chosen too large. There are not enough nearest-"
                                       "neighbours to permit this computation!";

    typename RTree_t::const_query_iterator outer_it;
    outer_it = RTree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
double dummy = 0.0;
    for( ; outer_it != RTree.qend(); ++outer_it){
dummy += 1.0;
const auto percent = static_cast<double>( static_cast<intmax_t>(10000.0*dummy/RTree.size())/100.0 );
std::cout << "  --> On datum " << dummy << " / " << RTree.size() << " = " << percent << "%" << std::endl;

        //Query for nearby items ("seeds") within a distance Eps from the current point "*outer_it".
        typename RTree_t::const_query_iterator nearby_it;
        nearby_it = RTree.qbegin( boost::geometry::index::nearest( *outer_it, RTree.size() ) );
        if(nearby_it == RTree.qend()) throw std::runtime_error(ThrowSelfPointCheck);
 
        size_t j = 0; //Start at zero because the self-point is included.
        for(;;){ // Better to just use std::advance? (Can we detect not-enough-points that way?)
            if(nearby_it == RTree.qend()) throw std::runtime_error(ThrowkTooLarge);
            if(j == k){
                out.push_back(boost::geometry::distance(*outer_it, *nearby_it));
                break;
            }
            ++nearby_it;
            ++j;
        }
    }                

    //Sort the data so the largest values occur first.
    std::sort(out.begin(), out.end(), std::greater<typename ClusteringDatum_t::SpatialType_>());
    return out;
}



enum SpatialQueryTechnique {
    UseNearby,
    UseWithin
};


template < typename RTree_t,  //A Boost.Geometry R*-tree, specifically.
           typename ClusteringDatum_t >
void DBSCAN( RTree_t & RTree,
             typename ClusteringDatum_t::SpatialType_ Eps,
             size_t MinPts = ClusteringDatum_t::SpatialDimensionCount_ * 2,
             SpatialQueryTechnique UsersSpatialQueryTechnique = SpatialQueryTechnique::UseWithin ){

    // This routine is an implementation of the well-known DBSCAN clustering algorithm which is 
    //   described in the widely-cited 1996 conference proceedings article:
    //   "A Density-Based Algorithm for Discovering Clusters" by Ester, Kriegel, Sander, and Xu.
    //
    // The routine is used to find 'clusters' of datum which form logical groups. This implementation
    //   is aided by a R*-tree index and is capable of clustering millions of datum within a few 
    //   minutes. It is spatially-oriented, so if you have non-spatial data you may need to perform
    //   some pre-clustering work-around steps, such as scaling all dimensions to be approximately
    //   similar. See notes below for detail.
    //
    // User parameters:
    //
    // 1. RTree --> The R*-tree already loaded with the data to be clustered. It will be modified in-place.
    // 2. Eps --> DBSCAN algorithm parameter. Sets the scale. It is the distance between which points are
    //            considered 'sufficiently' close. (See lengthy note below.) A separate routine is
    //            provided to help determine an appropriate value for Eps.
    // 3. MinPts --> DBSCAN algorithm parameter. Sets the minimal number of nearby connections each point
    //               must have. Authors recommend 2x the dimension.
    // 4. SpatialQueryTechnique --> Specify the method used to find points in the vicinity of a given point.
    //                              Makes a large impact on performance!
    //
    // NOTE: This routine ignores any Attributes and UserData in the ClusteringDatum instances. Points are
    //       heavily copied inside the R*-tree during insert and removal, so keep the ClusteringDatum 
    //       members: (1) easy to copy, and (2) small in size.
    //
    // NOTE: The DBSCAN algorithm does not easily permit custom metrics / specialization of the concept of
    //       'distance' to spaces with embedded non-spatial dimensions. So the 'density' of points along 
    //       one dimension must be logically comparable to the density along other dimensions. This is not
    //       a problem if you are using proper cartesian coordinates like spatial coordinates. But it does
    //       present a problem when you want to mix non-cartesian dimensions or dimensions with logically
    //       different scale. 
    //
    //       For example, consider clustering a collection of photos. There are 2D cartesian GPS coordinates
    //       derived from the metadata, but you also want to use the time they were taken. You will need to
    //       somehow treat time at a third dimension for DBSCAN. But what is the 'distance' between two 
    //       photos? Remember that a sane distance (in this routine) must be single scalar number. More to
    //       the point, is it OK to just drop units, so one second is equivalent to one metre? The choice
    //       of relative scale influences the density along the dimension!
    //
    //       There is no easy solution to this problem. Consider making an application-informed choice
    //       (e.g., one second of temporal separation is effectively equivalent to one metre of spatial
    //       separation) and pre-scaling the troublesome dimensions using this scale factor.
    //
    //       Another technique is to try a variety of scaling factors and see how the clustering changes.
    //       Though this technique is subjective, it should help you nail down a roughly appropriate 
    //       set of scaling factors.
    //
    // NOTE: The Eps DBSCAN parameter should be chosen, according to the authors, using a heuristic. The
    //       Essential idea is that you should find the minimal Eps needed to permit the smallest cluster
    //       to still be formed. In other words, find the Eps such that making Eps smaller leads to a rapid 
    //       loss of clusters. (Yes, this proposal defaults to a subjective choice.)
    //       
    //       There might be more modern suggestions to estimate Eps. Hopefully using some information
    //       theory approach. But I have not researched it at the moment.
    //       
    // NOTE: The MinPts DBSCAN parameter controls the minimum number of points which a point must be near
    //       to for the point to be considered 'connected' to them. The authors recommend setting it to
    //       twice the dimensionality and note that the clusters for real data tend not to be strongly
    //       affected by the specific value. However, the minimum cluster size is strongly application-
    //       dependent. It should be >1, though, because in this case all single points are considered
    //       clusters!
    //

    //(Needed to work around missing RTree_t.begin()/end() when Boost.Geometry version < 1.58.0.)
    constexpr auto RTreeSpatialQueryGetAll = [](const ClusteringDatum_t &) -> bool { return true; };

    typedef boost::geometry::model::box<ClusteringDatum_t> Box_t;


    //Ensure all datum start with Unclassified ClusterIDs. It is necessary to have this here, for example, 
    // if the user has re-run the algorithm or tampered with the IDs.
    //
    // NOTE: pre-defining some objects to be in specific clusters is not supported. You can accomplish this
    // by attaching UserData to 'tag' these objects.
    {
        typename RTree_t::const_query_iterator it;
        it = RTree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
        for( ; it != RTree.qend(); ++it){
            const_cast<ClusteringDatum_t &>(*it).CID.Raw = ClusterID<typename ClusteringDatum_t::ClusterIDType_>::Unclassified;
        }
    }


    //Main loop.
    {
        const std::string ThrowSelfPointCheck = "Spatial vicinity queries should always return the self point."
                                                " This point is missing, indicating numerical stability issues"
                                                " or logical errors in the spatial indexing approach.";
        const std::string ThrowNotImplemented = "Specified spatial query technique has not been implemented.";
                                  
        auto WorkingCID = ClusterID<typename ClusteringDatum_t::ClusterIDType_>().NextValidClusterID();

        typename RTree_t::const_query_iterator outer_it;
        outer_it = RTree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
        for( ; outer_it != RTree.qend(); ++outer_it){
            if(!outer_it->CID.IsUnclassified()) continue;

            //Query for nearby items ("seeds") within a distance Eps from the current point "*outer_it".
            std::list<typename RTree_t::const_query_iterator> seed_its;
            if(UsersSpatialQueryTechnique == SpatialQueryTechnique::UseNearby){
                typename RTree_t::const_query_iterator nearby_it;
                nearby_it = RTree.qbegin( boost::geometry::index::nearest( *outer_it, RTree.size() ) );
                if(nearby_it == RTree.qend()) throw std::runtime_error(ThrowSelfPointCheck);
                for( ; nearby_it != RTree.qend(); ++nearby_it){
                    if(boost::geometry::distance(*outer_it, *nearby_it) < Eps){
                        seed_its.push_back( nearby_it );
                    }else{
                        break;
                    }
                }
            }else if(UsersSpatialQueryTechnique == SpatialQueryTechnique::UseWithin){
                //This box is aligned with the cartesian grid and bounds the hyper-sphere of radius Eps.
                Box_t BBox( outer_it->CoordinateAlignedBBoxMinimal(Eps),
                            outer_it->CoordinateAlignedBBoxMaximal(Eps) );

                typename RTree_t::const_query_iterator nearby_it;
                nearby_it = RTree.qbegin( boost::geometry::index::within( BBox ) );
                if(nearby_it == RTree.qend()) throw std::runtime_error(ThrowSelfPointCheck);
                for( ; nearby_it != RTree.qend(); ++nearby_it){
                    if(boost::geometry::distance(*outer_it, *nearby_it) < Eps){
                        seed_its.push_back( nearby_it );
                    }
                }
            }else{
                throw std::runtime_error(ThrowNotImplemented);
            }

            //Check if the point was sufficiently well-connected. 
            if(seed_its.size() < MinPts){
                const_cast<ClusteringDatum_t &>(*outer_it).CID.Raw = ClusterID<typename ClusteringDatum_t::ClusterIDType_>::Noise;
                continue;
            }else{ 
                //All datum in `seeds` are "density-reachable" from current point "*outer_it".
                // So we update their ClusterID.
                for(auto seed_it : seed_its) const_cast<ClusteringDatum_t &>(*seed_it).CID = WorkingCID;

                //Remove the self point from the nearby points query.
                // Note: boost::geometry::index::nearest() will return the nearest first. Since the
                // distance between any point and itself should always be zero, the first point will
                // be the self point. So we compare the addresses to be certain. The following
                // should only ever fail if Boost::Geometry fails to find nearby points properly!
                {
                    auto IsTheSelfPoint = [outer_it](const typename RTree_t::const_query_iterator &in_it) -> bool {
                        return (std::addressof(*in_it) == std::addressof(*outer_it));
                    };
                    //auto SelfPoint_it = std::find_if(seed_its.begin(), seed_its.end()
                    const auto SizePriorToSelfPointRemoval = seed_its.size();
                    seed_its.remove_if(IsTheSelfPoint);
                    const auto SizeAfterSelfPointRemoval = seed_its.size();
                    if((SizeAfterSelfPointRemoval + 1) != SizePriorToSelfPointRemoval){
                        throw std::runtime_error(ThrowSelfPointCheck);
                    }
                }

                //Loop over the `seeds`, changing cluster IDs as needed.
                while(!seed_its.empty()){
                    auto currentP_it = seed_its.front();

                    //Query for nearby items within a distance Eps from the current point "*currentP_it".
                    std::list<typename RTree_t::const_query_iterator> result_its;
                    if(UsersSpatialQueryTechnique == SpatialQueryTechnique::UseNearby){
                        typename RTree_t::const_query_iterator nearby_it;
                        nearby_it = RTree.qbegin( boost::geometry::index::nearest( *currentP_it, RTree.size() ) );
                        if(nearby_it == RTree.qend()) throw std::runtime_error(ThrowSelfPointCheck);
                        for( ; nearby_it != RTree.qend(); ++nearby_it){
                            if(boost::geometry::distance(*currentP_it, *nearby_it) < Eps){
                                result_its.push_back( nearby_it );
                            }else{
                                break;
                            }
                        }
                    }else if(UsersSpatialQueryTechnique == SpatialQueryTechnique::UseWithin){
                        //This box is aligned with the cartesian grid and bounds the hyper-sphere of radius Eps.
                        Box_t BBox( currentP_it->CoordinateAlignedBBoxMinimal(Eps),
                                    currentP_it->CoordinateAlignedBBoxMaximal(Eps) );
                
                        typename RTree_t::const_query_iterator nearby_it;
                        nearby_it = RTree.qbegin( boost::geometry::index::within( BBox ) );
                        if(nearby_it == RTree.qend()) throw std::runtime_error(ThrowSelfPointCheck);
                        for( ; nearby_it != RTree.qend(); ++nearby_it){
                            if(boost::geometry::distance(*currentP_it, *nearby_it) < Eps){
                                result_its.push_back( nearby_it );
                            }
                        }

                    }else{
                        throw std::runtime_error(ThrowNotImplemented);
                    }

                    //Only need to change anything if there are enough neighbouring points.
                    if(result_its.size() >= MinPts){
                        for(auto result_it : result_its){
                            if(result_it->CID.IsUnclassified() || result_it->CID.IsNoise()){  // equiv. to !result_it->CID.IsRegular()
                                if(result_it->CID.IsUnclassified()){
                                    seed_its.push_back(result_it);
                                }
                                const_cast<ClusteringDatum_t &>(*result_it).CID = WorkingCID;
                            }
                        }
                    }
                 
                    seed_its.pop_front();
                }
            }
            WorkingCID = WorkingCID.NextValidClusterID();
        }

    }
    return;
}

#endif //YGOR_CLUSTERING_DBSCAN_HPP
