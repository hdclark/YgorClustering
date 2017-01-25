#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

#include <boost/geometry/index/parameters.hpp> 

#include <boost/geometry/index/rtree.hpp>

#include <boost/geometry/io/svg/svg_mapper.hpp> 

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <limits>
#include <utility>


//Utility class for (integer) cluster IDs. These differentiate clusters and noise datum.
template <class T>
class ClusterID {
public:
    T Raw;

    static const auto Unclassified = std::numeric_limits<T>::max();
    static const auto Noise = std::numeric_limits<T>::max() - static_cast<T>(1);
    static const auto Cluster0 = std::numeric_limits<T>::min(); //Cluster1 = Cluster0 + 1, etc..

    constexpr ClusterID(void) : Raw(ClusterID<T>::Unclassified) { }
    constexpr ClusterID(T in) : Raw(in.Raw) { }

    constexpr bool IsNoise(void){
        return (this->Raw == this->Noise);
    }

    constexpr bool IsUnclassified(void){
        return (this->Raw == this->Unclassified);
    }

    constexpr bool IsRegular(void){
        return (this->Raw != this->Unclassified) && (this->Raw != this->Noise);
    }

    ClusterID<T> NextValidClusterID(){
        if(!this->IsRegular()) return this->Cluster0;

        auto CandidateID(this->Raw + static_cast<T>(1));
        if(!CandidateID.IsRegular()){
            throw std::runtime_error("Ran out of valid ClusterIDs. Use a type with larger range (e.g., uint64_t).");
        }
        return CandidateID;
    }

};

class ClusteringDatum {
public:

    typedef double coord_type;
    typedef float attr_type;
    typedef uint32_t cid_type;

    constexpr static size_t spatial_dimension_count = 2;

    std::array<coord_type, 2> Coordinates; //Dimension is arbitrary. Must match r*-tree dimension and coordinate system.
    std::array<attr_type, 1>  Attributes;  //Dimension is arbitrary.
    ClusterID<cid_type> CID;

    ClusteringDatum() {}; //No elements will be initialized if using this constructor.
    constexpr ClusteringDatum(const std::array<coord_type,2> &in) : Coordinates(in), Attributes({(attr_type)(0)}) {};
    constexpr ClusteringDatum(const std::array<coord_type,2> &a, attr_type b) : Coordinates(a), Attributes({b}) {};
     
    bool operator==(const ClusteringDatum &in) const {
        //Default only considers spatial information so it can be used in spatial indexes.
        return (this->Coordinates == in.Coordinates);
    }

    //Any extra parameters can go here, like pointers to other data or datum can be made "fat" by packing all necessary info
    // in as needed so no other copies are needed. You should overload the class to provide such extra info.
};

class ClusteringDatumBox {
public:
    std::pair<ClusteringDatum*,ClusteringDatum*> Corners;
};

namespace boost {
    namespace geometry {
        namespace traits {
            //Adapt ClusteringDatum to be a Boost Geometry "point" type.

            template<> struct tag<ClusteringDatum> { 
                typedef point_tag type; 
            };
        
            template<> struct coordinate_type<ClusteringDatum> { 
                typedef ClusteringDatum::coord_type type; 
            };
        
            template<> struct coordinate_system<ClusteringDatum> { 
                typedef boost::geometry::cs::cartesian type; 
            };
        
            template<> struct dimension<ClusteringDatum> : boost::mpl::int_<ClusteringDatum::spatial_dimension_count> { };
        
            template<> struct access<ClusteringDatum, 0> {
                static ClusteringDatum::coord_type get(ClusteringDatum const &in){
                    return in.Coordinates[0];
                }
        
                static void set(ClusteringDatum &p, ClusteringDatum::coord_type const &val){
                    p.Coordinates[0] = val;
                }
            };
        
            template<> struct access<ClusteringDatum, 1> {
                static ClusteringDatum::coord_type get(ClusteringDatum const &in){
                    return in.Coordinates[1];
                }
        
                static void set(ClusteringDatum &p, typename ClusteringDatum::coord_type const &val){
                    p.Coordinates[1] = val;
                }
            };

        }
    }
}


int main(){

    //typedef ClusteringDatum ClusteringDatum;
    //constexpr auto Dimensions = 2;

    constexpr size_t MaxElementsInANode = 6; // 16, 32, 128, 256, ... ?
    typedef boost::geometry::index::rstar<MaxElementsInANode> RTreeParameter_t;
    typedef boost::geometry::index::rtree<ClusteringDatum,RTreeParameter_t> RTree_t;

    constexpr auto RTreeSpatialQueryGetAll = [](const ClusteringDatum &) -> bool { return true; };


    RTree_t rtree;


    if(false){ //Silly little basic test with phony data.

        for(double f = 0 ; f < 10 ; f += 1) rtree.insert(ClusteringDatum( {f,f}, f ));
    
        //------------- Output
        {
          std::ofstream svg("Visualized.svg");
          boost::geometry::svg_mapper<ClusteringDatum> mapper(svg, 1280, 1024);
    
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
    
        ClusteringDatum QueryPoint({5.1,5.1});
    
        RTree_t::const_query_iterator it = rtree.qbegin(boost::geometry::index::nearest(QueryPoint, 100));
        while(it != rtree.qend()){
            double d = boost::geometry::distance(QueryPoint, *it);
            std::cout << "  Point: " << boost::geometry::wkt(*it) 
                      << "\t\t Distance: " << d 
                      << "\t\t Attribute[0]: " << it->Attributes[0] 
                      << std::endl;
    
            // break if the distance is too big
            if(d > 2){
                std::cout << "break!" << std::endl;
                break;
            }
            ++it;
        }
    }



    //Stuff some testing data into the R*-tree.
    rtree.insert(ClusteringDatum( { -3.0, 0.0 }, 0.0 ));
    rtree.insert(ClusteringDatum( { -2.8, 0.1 }, 1.0 ));
    rtree.insert(ClusteringDatum( { -2.7, 0.2 }, 0.0 ));
    rtree.insert(ClusteringDatum( { -2.5, 0.5 }, 0.0 ));
    rtree.insert(ClusteringDatum( {  2.0, 0.2 }, 1.0 ));
    rtree.insert(ClusteringDatum( {  2.1, 1.1 }, 1.0 ));
    rtree.insert(ClusteringDatum( {  2.7, 1.5 }, 0.0 ));


    //Specify/harvest user parameters.
    const size_t MinPts = ClusteringDatum::spatial_dimension_count*2; //DBSCAN algorithm parameter. Minimum # of points in a cluster.



    //Ensure all datum start with Unclassified ClusterIDs.(Note: This step is necessary in case the
    // user has re-run the algorithm, tampered with the data, etc..)
    for(auto &datum : DataToCluster){
        datum.CID.Raw = ClusterID<uint32_t>::Unclassified;
    }

/*
    uint32_t WorkingCID = ClusterID<uint32_t>().NextValidClusterID();
    for(auto datum : DataToCluster){
        if(!datum.CID.IsUnclassified()) continue;

        //////// "ExpandCluster" routine here /////////
        bool IterateWorkingClusterID = false;

        if(wCard({datum}) <= static_cast<double>(0)){
            datum.CID.Raw = ClusterID<uint32_t>::Unclassified;
            continue;
        }

        auto NearbyData = FindNearbyData(datum, NPred); //Including self!

        if(wCard(NearbyData) < MinCard){
            datum.CID.Raw = ClusterID<uint32_t>::Noise;
            continue;
        }

        //If we get here, 'datum' is a core object.
        
        for(auto &NearbyDatum : NearbyData){
            NearbyDatum.CID.Raw = WorkingCID;
        }

        //Remove datum from NearbyDatum set.
        NearbyData.remove(datum); // TODO

        while(!NearbyData.empty()){
            auto ADatum = NearbyDatum.front();

            auto ADatumVicinity = FindNearbyData(ADatum,  NPred); //Including self!
 
            if(wCard(ADatumVicinity) < MinCard){
                NearbyData.remove(ADatum); // TODO
                continue;
            }

            for(auto &ANearbyDatum : ADatumVicinity){
                if((wCard(ANearbyDatum) > static_cast<double>(0))
                && (!ANearbyDatum.CID.IsRegular())){
                    if(ANearbyDatum.CID.IsUnclassified()){
                        NearbyData.append(ANearbyDatum);
                    }
                    ANearbyDatum.CID = WorkingCID;
                }
            }
            NearbyData.remove(ADatum); // TODO
            continue;
        }

        //////////////////////////////////////////////
        if(IterateWorkingClusterID) WorkingCID = WorkingCID.NextValidClusterID();
    }

//This pseudo code is a total fucking mess! It assumes you are working with integer-indexed arrays AND
// SIMULATANEOUSLY assumes you can find an object by value in the array. WTH?
//
//
// You need to first figure out how you are going to generate sets of data.
// Probably you will want to use iterators.
//
// Next, go through the pseudo code and RENAME the variables to something more sane and differentiable.
// Even SET_A and SET_B would be more clear.
//
// Next, translate the pseudo code to C++, using the snippets above where possible. But be aware that
// some of the above might be wrong because of the strange pseudo code I was working with...

*/

    return 0;
}

