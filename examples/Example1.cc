
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

/*
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

#include <boost/geometry/index/parameters.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/io/svg/svg_mapper.hpp>
*/

#include "YgorClustering.hpp"
//#include "YgorClusteringDatumCommonInstantiations.hpp"

int main(){

    typedef ClusteringDatum<2, double, 1, float, uint16_t> CDat_t;

    constexpr size_t MaxElementsInANode = 6; // 16, 32, 128, 256, ... ?
    typedef boost::geometry::index::rstar<MaxElementsInANode> RTreeParameter_t;
    typedef boost::geometry::index::rtree<CDat_t,RTreeParameter_t> RTree_t;
    typedef boost::geometry::model::box<CDat_t> Box_t;

    RTree_t rtree;


    if(false){ //Silly little basic test with phony data.
        constexpr auto RTreeSpatialQueryGetAll = [](const CDat_t &) -> bool { return true; };
        for(double f = 0 ; f < 10 ; f += 1) rtree.insert(CDat_t( {f,f}, {static_cast<float>(f)} ));
    
        //------------- Output
        {
          std::ofstream svg("Visualized.svg");
          boost::geometry::svg_mapper<CDat_t> mapper(svg, 1280, 1024);
    
          //Add the items to the map. The virtual bounds will be computed to accomodate.
          RTree_t::const_query_iterator it;
          it = rtree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
          for( ; it != rtree.qend(); ++it){
              mapper.add(*it);
          }
    
          //Actually draw the items.
          it = rtree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
          for( ; it != rtree.qend(); ++it){
              mapper.map(*it, "fill-opacity:0.75; fill:rgb(75,100,0); stroke:rgb(30,40,0); stroke-width:2", 5);
          }
        }
        //------------- Output
    
        CDat_t QueryPoint({5.1,5.1});
    
        RTree_t::const_query_iterator it = rtree.qbegin(boost::geometry::index::nearest(QueryPoint, 100));
        while(it != rtree.qend()){
            double d = boost::geometry::distance(QueryPoint, *it);
            std::cout << "  Point: " << boost::geometry::wkt(*it) 
                      << "\t\t Distance: " << d 
                      << "\t\t Attribute[0]: " << it->Attributes[0] 
                      << std::endl;
    
            // break if the distance is too big
            if(d > 2.0){
                std::cout << "break!" << std::endl;
                break;
            }
            ++it;
        }
        return 0;
    }


    //Within a box method.
    if(false){
        constexpr auto RTreeSpatialQueryGetAll = [](const CDat_t &) -> bool { return true; };

        rtree.insert(CDat_t( { -3.0, 0.0 }, {0.0f} ));
        rtree.insert(CDat_t( { -2.8, 0.1 }, {1.0f} ));
        rtree.insert(CDat_t( { -2.7, 0.2 }, {0.0f} ));
        rtree.insert(CDat_t( { -2.5, 0.5 }, {0.0f} ));
        rtree.insert(CDat_t( {  2.0, 0.2 }, {1.0f} ));
        rtree.insert(CDat_t( {  2.1, 1.1 }, {1.0f} ));
        rtree.insert(CDat_t( {  2.7, 1.5 }, {0.0f} ));
        rtree.insert(CDat_t( {  6.7, 6.5 }, {0.0f} ));
        rtree.insert(CDat_t( {  6.7, 7.5 }, {0.0f} ));
        rtree.insert(CDat_t( {  7.7, 6.5 }, {0.0f} ));
        rtree.insert(CDat_t( {  6.7, 5.5 }, {0.0f} ));
        rtree.insert(CDat_t( {  5.7, 6.5 }, {0.0f} ));
    //    for(double x = -100000.0 ; x < 100000.0 ; x += 1.0) rtree.insert(CDat( {x,x}, {static_cast<float>(x)} ));
    
        CDat_t apoint({ -2.8, 0.1 }, {1.0f});
        Box_t abox( apoint.CoordinateAlignedBBoxMinimal(3.3),
                    apoint.CoordinateAlignedBBoxMaximal(3.3) );
    
        {
            RTree_t::const_query_iterator it;
            it = rtree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
            for( ; it != rtree.qend(); ++it){
                std::cout << "  Point: " << boost::geometry::wkt(*it)
                          << "  Within: " << (boost::geometry::within(*it, abox) ? "yes" : "no") << std::endl;
            }
        }
        return 0;
    
    }

    //Stuff some testing data into the R*-tree.
    rtree.insert(CDat_t( { -3.0, 0.0 }, {0.0f} ));
    rtree.insert(CDat_t( { -2.8, 0.1 }, {1.0f} ));
    rtree.insert(CDat_t( { -2.7, 0.2 }, {0.0f} ));
    rtree.insert(CDat_t( { -2.5, 0.5 }, {0.0f} ));
    rtree.insert(CDat_t( {  2.0, 0.2 }, {1.0f} ));
    rtree.insert(CDat_t( {  2.1, 1.1 }, {1.0f} ));
    rtree.insert(CDat_t( {  2.7, 1.5 }, {0.0f} ));
    rtree.insert(CDat_t( {  6.7, 6.5 }, {0.0f} ));
    rtree.insert(CDat_t( {  6.7, 7.5 }, {0.0f} ));
    rtree.insert(CDat_t( {  7.7, 6.5 }, {0.0f} ));
    rtree.insert(CDat_t( {  6.7, 5.5 }, {0.0f} ));
    rtree.insert(CDat_t( {  5.7, 6.5 }, {0.0f} ));
    //for(double x = -10'000.0 ; x < 10'000.0 ; x += 1.0) rtree.insert(CDat_t( {x,x}, {static_cast<float>(x)} ));

    //Stuff some more points in.
    size_t FixedSeed = 9137;
    std::mt19937 re(FixedSeed);
    std::uniform_real_distribution<> rd(-100.0, 100.0);
    for(size_t i = 0; i < 1000; ++i) rtree.insert(CDat_t( { rd(re), rd(re) }, { rd(re) }));


    //const size_t MinPts = CDat_t::SpatialDimensionCount_*2;
    //const SpatialQueryTechnique UsersSpatialQueryTechnique = SpatialQueryTechnique::UseWithin;
    const double Eps = 6.0;
    
    DBSCAN<RTree_t,CDat_t>(rtree,Eps);



    //Print out the points with cluster info.
    if(true){
        constexpr auto RTreeSpatialQueryGetAll = [](const CDat_t &) -> bool { return true; };
        RTree_t::const_query_iterator it;
        it = rtree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
        for( ; it != rtree.qend(); ++it){
            std::cout << "  Point: " << boost::geometry::wkt(*it)
                      << "\t\t Attribute[0]: " << it->Attributes[0]
                      << "\t\t ClusterID: " << it->CID.ToText()
                      << std::endl;
        }
    }

    //Stream out an svg diagram where colours denote clusters.
    if(true){
        constexpr auto RTreeSpatialQueryGetAll = [](const CDat_t &) -> bool { return true; };
        std::ofstream svg("Visualized.svg");
        boost::geometry::svg_mapper<CDat_t> mapper(svg, 1280, 1024);

        //Add the items to the map. The virtual bounds will be computed to accomodate.
        // Also keep a record of the distinct clusters encountered.
        std::set<typename CDat_t::ClusterIDType_> RawCIDs;

        RTree_t::const_query_iterator it;
        it = rtree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
        for( ; it != rtree.qend(); ++it){
            mapper.add(*it);
            RawCIDs.insert( it->CID.Raw );
        }
        std::cout << RawCIDs.size() << " distinct ClusterIDs encountered." << std::endl;

        //Create a mapping between the ClusterIDs and a pseudo-random RGB colour.
        size_t ColourSeed = 9137;
        std::mt19937 re(ColourSeed);
        std::uniform_real_distribution<> rdC(0.0, 1.0);
        std::uniform_int_distribution<> rdA( 50, 210);
        std::uniform_int_distribution<> rdB( 20, 125);
        std::map<typename CDat_t::ClusterIDType_, std::string> ColoursA, ColoursB;
        for(const auto RawCID : RawCIDs){
            ColoursA[RawCID] = std::to_string((rdC(re) > 0.33) ? rdA(re) : 230) + std::string(",")
                             + std::to_string((rdC(re) > 0.33) ? rdA(re) : 230) + std::string(",")
                             + std::to_string((rdC(re) > 0.33) ? rdA(re) : 230);
            ColoursB[RawCID] = std::to_string((rdC(re) > 0.33) ? rdB(re) :  10) + std::string(",")
                             + std::to_string((rdC(re) > 0.33) ? rdB(re) :  10) + std::string(",")
                             + std::to_string((rdC(re) > 0.33) ? rdB(re) :  10);
        }

        //Actually draw the items.
        it = rtree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
        for( ; it != rtree.qend(); ++it){
            std::stringstream ss;
            ss << "fill-opacity:0.80; "
               << "fill:rgb(" << ColoursA[it->CID.Raw] << "); "
               << "stroke-opacity:0.90; "
               << "stroke:rgb(" << ColoursB[it->CID.Raw] << "); "
               << "stroke-width:1";
            //mapper.map(*it, "fill-opacity:0.75; fill:rgb(75,100,0); stroke:rgb(30,40,0); stroke-width:2", 5);
            mapper.map(*it, ss.str(), 6);
        }
    }

    return 0;
}

