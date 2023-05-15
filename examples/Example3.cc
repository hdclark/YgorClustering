
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

#include "YgorClustering.hpp"
//#include "YgorClusteringDatumCommonInstantiations.hpp"

int main(){

    constexpr size_t MaxElementsInANode = 6; // 16, 32, 128, 256, ... ?
    typedef boost::geometry::index::rstar<MaxElementsInANode> RTreeParameter_t;

    typedef ClusteringDatum<1, double, 0, double, uint32_t, uint32_t> CDat_t;
    typedef boost::geometry::index::rtree<CDat_t,RTreeParameter_t> RTree_t;

    RTree_t rtree;

    size_t FixedSeed = 9137;
    std::mt19937 re(FixedSeed);
    std::uniform_real_distribution<> rd(0.0, 1'000'000.0);

    for(size_t i = 0; i < 1'000'000; ++i){
        rtree.insert(CDat_t({ rd(re) }));
    }


    //const size_t MinPts = CDat_t::SpatialDimensionCount_*2;
    //const SpatialQueryTechnique UsersSpatialQueryTechnique = SpatialQueryTechnique::UseWithin;
    const double Eps = 0.3;
    
    DBSCAN<RTree_t,CDat_t>(rtree,Eps);

/*
    //Print out the points with cluster info.
    if(false){
        constexpr auto RTreeSpatialQueryGetAll = [](const CDat_t &) -> bool { return true; };
        RTree_t::const_query_iterator it;
        it = rtree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
        for( ; it != rtree.qend(); ++it){
            std::cout << "  Point: " << boost::geometry::wkt(*it)
                      << "\t\t ClusterID: " << it->CID.ToText()
                      << std::endl;
        }
    }
*/
    return 0;
}

