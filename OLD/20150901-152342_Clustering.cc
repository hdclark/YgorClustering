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
#include <memory>
#include <algorithm>
#include <cassert>
#include <string>
#include <complex>

//Utility class for (integer) cluster IDs. These differentiate clusters and noise datum.
template <class T> class ClusterID {
    public:
        T Raw;
    
        static const auto Unclassified = std::numeric_limits<T>::max();
        static const auto Noise = std::numeric_limits<T>::max() - static_cast<T>(1);
        static const auto Cluster0 = std::numeric_limits<T>::min(); //Cluster1 = Cluster0 + 1, etc..
    
        constexpr ClusterID(void) : Raw(ClusterID<T>::Unclassified) { }
        constexpr ClusterID(const ClusterID<T> &in) : Raw(in.Raw) { }
        constexpr ClusterID(T in) : Raw(in) { }
    
        constexpr bool IsNoise(void) const {
            return (this->Raw == this->Noise);
        }
    
        constexpr bool IsUnclassified(void) const {
            return (this->Raw == this->Unclassified);
        }
    
        constexpr bool IsRegular(void) const {
            return (this->Raw != this->Unclassified) && (this->Raw != this->Noise);
        }
    
        ClusterID<T> NextValidClusterID() const {
            if(!this->IsRegular()) return this->Cluster0;
    
            ClusterID<T> CandidateID(this->Raw + static_cast<T>(1));
            if(!CandidateID.IsRegular()){
                throw std::runtime_error("Ran out of valid ClusterIDs. Use a type with larger range (e.g., uint64_t).");
            }
            return CandidateID;
        }
    
        std::string ToText(void) const {
            std::string out;
            if(this->IsNoise()){
                out = "Noise";
            }else if(this->IsUnclassified()){
                out = "Unclassified";
            }else if(this->IsRegular()){
                out = std::to_string(this->Raw);
            }else{
                throw std::runtime_error("Programming error. This ClusterID is not Noise, Unclassified, or Regular. What is it?");
            }
            return out;
        }
};


class ClusteringUserDataEmptyClass { }; //Used to optionally forgo passing in extra data.


template < std::size_t SpatialDimensionCount,   //For 3D data, this would be 3.
           typename SpatialType,                //Generally a double.
           std::size_t AttributeDimensionCount, //Depends on the user's needs. Probably 0 or 1.
           typename AttributeType,              //Depends on the user's needs. Probably float or double.
           typename ClusterIDType,              //Generally uint16_t or uint32_t.
           typename UserDataClass = ClusteringUserDataEmptyClass > //Used to attach generic data to elements, if desired.
class CDat {
    public:
        //Type-defs and constants. (For accessibility after instantiation.)
        constexpr static size_t SpatialDimensionCount_ = SpatialDimensionCount;
        typedef SpatialType SpatialType_;
        constexpr static size_t AttributeDimensionCount_ = AttributeDimensionCount;
        typedef AttributeType AttributeType_;

        //Data members.
        std::array<SpatialType, SpatialDimensionCount> Coordinates; //For spatial indexing in the R*-tree.
        std::array<AttributeType, AttributeDimensionCount> Attributes; //May be indexed in the R*-tree. Depends on algorithm.
        ClusterID<ClusterIDType> CID;
        UserDataClass UserData;

        //Constructors.
        CDat() { };
        constexpr CDat(const decltype(Coordinates) &in) : Coordinates(in) { };
        constexpr CDat(const decltype(Coordinates) &a, 
                       const decltype(Attributes) &b) : Coordinates(a), Attributes(b) { };

        //Member functions.
        bool operator==(const CDat &in) const {
            //Default only considers spatial information so it can be used in spatial indexes.
            return (this->Coordinates == in.Coordinates);
        }
};


namespace boost { namespace geometry { namespace traits {
    //Adapt CDat to be a Boost Geometry "point" type.

    template < std::size_t SpatialDimensionCount,
               typename SpatialType,
               std::size_t AttributeDimensionCount,
               typename AttributeType,
               typename ClusterIDType,
               typename UserDataClass >
    struct tag< 
          CDat< SpatialDimensionCount,
                SpatialType,
                AttributeDimensionCount,
                AttributeType,
                ClusterIDType,
                UserDataClass > 
                              > {
        typedef point_tag type;
    };


    template < std::size_t SpatialDimensionCount,   
               typename SpatialType,                
               std::size_t AttributeDimensionCount, 
               typename AttributeType,              
               typename ClusterIDType,              
               typename UserDataClass > 
    struct coordinate_type< 
                      CDat< SpatialDimensionCount,
                            SpatialType,
                            AttributeDimensionCount,
                            AttributeType,
                            ClusterIDType,
                            UserDataClass > 
                                          > {
        typedef SpatialType type;
    };


    template < std::size_t SpatialDimensionCount,
               typename SpatialType,               
               std::size_t AttributeDimensionCount,
               typename AttributeType, 
               typename ClusterIDType, 
               typename UserDataClass >
    struct coordinate_system<
                        CDat< SpatialDimensionCount,
                              SpatialType,
                              AttributeDimensionCount,
                              AttributeType,
                              ClusterIDType,
                              UserDataClass > 
                                            > {
        typedef boost::geometry::cs::cartesian type;
    };


    template < std::size_t SpatialDimensionCount,
               typename SpatialType,               
               std::size_t AttributeDimensionCount,
               typename AttributeType, 
               typename ClusterIDType, 
               typename UserDataClass >
    struct dimension<
                CDat< SpatialDimensionCount,
                      SpatialType,
                      AttributeDimensionCount,
                      AttributeType,
                      ClusterIDType,
                      UserDataClass > 
                                    > : boost::mpl::int_< SpatialDimensionCount - AttributeDimensionCount> { };
//        static const std::size_t type = SpatialDimensionCount; //(Deprecated method of specifying dimension count?)
//    }; 


    template < std::size_t SpatialDimensionCount,
               typename SpatialType,
               std::size_t AttributeDimensionCount,
               typename AttributeType,
               typename ClusterIDType,
               typename UserDataClass,
               std::size_t ElementNumber > // <-- (Note extra access template parameter.)
    struct access<
             CDat< SpatialDimensionCount,
                   SpatialType,
                   AttributeDimensionCount,
                   AttributeType,
                   ClusterIDType,
                   UserDataClass >,
             ElementNumber       > {

        static inline SpatialType get(const CDat< SpatialDimensionCount,
                                                  SpatialType,
                                                  AttributeDimensionCount,
                                                  AttributeType,
                                                  ClusterIDType,
                                                  UserDataClass > &in){
            return std::get<ElementNumber>(in.Coordinates);
            //Could use static template methods to include the Attributes as 'higher spatial dimensions' here.
        }

        static inline void set(CDat< SpatialDimensionCount,
                                     SpatialType,
                                     AttributeDimensionCount,
                                     AttributeType,
                                     ClusterIDType,
                                     UserDataClass > &in, 
                               const SpatialType &val){
            std::get<ElementNumber>(in.Coordinates) = val;
            //Could use static template methods to include the Attributes as 'higher spatial dimensions' here.
            return;
        }
    };

} } } // namespace boost::geometry::traits


/*
class ClusteringDatum {
    public:
    
        typedef double coord_type;
        typedef float attr_type;
        typedef uint32_t cid_type;
    
        constexpr static size_t spatial_dimension_count = 2;
    
        std::array<coord_type, 2> Coordinates; //For spatial indexing in the R*-tree.
        std::array<attr_type, 1>  Attributes;
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


namespace boost { namespace geometry { namespace traits {
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

} } } // namespace boost::geometry::traits
*/




int main(){

    if(false){

        CDat<3, double, 1, float, uint16_t> a;
        CDat<9, std::complex<double>, 9, uintmax_t, size_t> b;

        CDat<3, double, 1, float, uint16_t, ClusteringUserDataEmptyClass> c;
        CDat<9, std::complex<double>, 9, uintmax_t, size_t, std::complex<double>> d;

        typedef CDat<3, double, 1, float, uint16_t> CDat_3_1;

        constexpr size_t MaxElementsInANode = 6; // 16, 32, 128, 256, ... ?
        typedef boost::geometry::index::rstar<MaxElementsInANode> RTreeParameter_t;
        typedef boost::geometry::index::rtree<CDat_3_1,RTreeParameter_t> RTree_t;

        RTree_t rtree;

        for(double x = -100.0 ; x < 100.0 ; x += 1.0){
           rtree.insert(CDat_3_1( {x,x,x}, {x} ));
        }

    }

    typedef ClusteringDatum<3, double, 1, float, uint16_t> C Dat;

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
    rtree.insert(ClusteringDatum( {  6.7, 6.5 }, 0.0 ));
    rtree.insert(ClusteringDatum( {  6.7, 7.5 }, 0.0 ));
    rtree.insert(ClusteringDatum( {  7.7, 6.5 }, 0.0 ));
    rtree.insert(ClusteringDatum( {  6.7, 5.5 }, 0.0 ));
    rtree.insert(ClusteringDatum( {  5.7, 6.5 }, 0.0 ));


    //START FUNCTION HERE.
    //
    //DBSCAN( this->rtree, Eps, MinPts ){...

    //Specify/harvest user parameters.
    const size_t MinPts = ClusteringDatum::spatial_dimension_count*2; //DBSCAN algorithm parameter. Minimum # of points in a cluster.
    const double Eps = 1.5; //DBSCAN algorithm parameter. Sets scale: is the distance between which points are considered 'sufficiently' close.

    //Ensure all datum start with Unclassified ClusterIDs.(Note: This step is necessary in case the
    // user has re-run the algorithm, tampered with the data, etc..)
    {
        RTree_t::const_query_iterator it;
        it = rtree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
        for( ; it != rtree.qend(); ++it){
            const_cast<ClusteringDatum &>(*it).CID.Raw = ClusterID<uint32_t>::Unclassified;
        }
    }


    //Main loop.
    {
        const std::string ThrowMessage = "Spatial vicinity queries should always return the self point."
                                         " This point is missing, indicating numerical stability issues"
                                         " or logical errors in the spatial indexing approach.";
                                         
        auto WorkingCID = ClusterID<ClusteringDatum::cid_type>().NextValidClusterID();

        RTree_t::const_query_iterator outer_it;
        outer_it = rtree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
        for( ; outer_it != rtree.qend(); ++outer_it){
            if(!outer_it->CID.IsUnclassified()) continue;

            //EXPAND_CLUSTER(this->rtree, outer_it, WorkingCID, Eps, MinPts)
	    bool IterateWorkingClusterID = false;
            //////////////////////////////////////////////

            //Query for nearby items within a distance Eps from the current point "*outer_it".
            std::list<RTree_t::const_query_iterator> seed_its;
            {
                RTree_t::const_query_iterator nearby_it;
                nearby_it = rtree.qbegin( boost::geometry::index::nearest( *outer_it, rtree.size() ) );
                if(nearby_it == rtree.qend()) throw std::runtime_error(ThrowMessage);
                for( ; nearby_it != rtree.qend(); ++nearby_it){
                    if(boost::geometry::distance(*outer_it, *nearby_it) < Eps){
                        seed_its.push_back( nearby_it );
                    }else{
                        break;
                    }
                }
            }

            if(seed_its.size() < MinPts){
                const_cast<ClusteringDatum &>(*outer_it).CID.Raw = ClusterID<uint32_t>::Noise;
                IterateWorkingClusterID = false;
            }else{ 
                //All datum in `seeds` are "density-reachable" from current point "*outer_it".
                // So we update their ClusterID.
                for(auto seed_it : seed_its) const_cast<ClusteringDatum &>(*seed_it).CID = WorkingCID;

                //Remove the self point from the nearby points query.
                // Note: boost::geometry::index::nearest() will return the nearest first. Since the
                // distance between any point and itself should always be zero, the first point will
                // be the self point. So we compare the addresses to be certain. The following
                // should only ever fail if Boost::Geometry fails to find nearby points properly!
                auto IsTheSelfPoint = [outer_it](const RTree_t::const_query_iterator &in_it) -> bool {
                    return (std::addressof(*in_it) == std::addressof(*outer_it));
                };
                //auto SelfPoint_it = std::find_if(seed_its.begin(), seed_its.end()
                const auto SizePriorToSelfPointRemoval = seed_its.size();
                seed_its.remove_if(IsTheSelfPoint);
                const auto SizeAfterSelfPointRemoval = seed_its.size();
                if((SizeAfterSelfPointRemoval + 1) != SizePriorToSelfPointRemoval){
                    throw std::runtime_error(ThrowMessage);
                }

                //Loop over the `seeds`, changing the cluster IDs as needed.
                while(!seed_its.empty()){
                    auto currentP_it = seed_its.front();

                    //Query for nearby items within a distance Eps from the current point "*currentP_it".
                    std::list<RTree_t::const_query_iterator> result_its;
                    {
                        RTree_t::const_query_iterator nearby_it;
                        nearby_it = rtree.qbegin( boost::geometry::index::nearest( *currentP_it, rtree.size() ) );
                        if(nearby_it == rtree.qend()) throw std::runtime_error(ThrowMessage);
                        for( ; nearby_it != rtree.qend(); ++nearby_it){
                            if(boost::geometry::distance(*currentP_it, *nearby_it) < Eps){
                                result_its.push_back( nearby_it );
                            }else{
                                break;
                            }
                        }
                    }

                    //Only need to change anything if there are enough neighbouring points.
                    if(result_its.size() >= MinPts){
                        for(auto result_it : result_its){
                            if(result_it->CID.IsUnclassified() || result_it->CID.IsNoise()){  // equiv. to !result_it->CID.IsRegular()
                                if(result_it->CID.IsUnclassified()){
                                    seed_its.push_back(result_it);
                                }
                                const_cast<ClusteringDatum &>(*outer_it).CID = WorkingCID;
                            }
                        }
                    }
                 
                    seed_its.pop_front();
                }
                IterateWorkingClusterID = true;
            }


//        RTree_t::const_query_iterator it = rtree.qbegin(boost::geometry::index::nearest(QueryPoint, 100));
//        while(it != rtree.qend()){
//            double d = boost::geometry::distance(QueryPoint, *it);


	    //////////////////////////////////////////////
            if(IterateWorkingClusterID) WorkingCID = WorkingCID.NextValidClusterID();
        }

    }


    //Print out the points with cluster info.
    {
        RTree_t::const_query_iterator it;
        it = rtree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
        for( ; it != rtree.qend(); ++it){
            std::cout << "  Point: " << boost::geometry::wkt(*it)
                      << "\t\t Attribute[0]: " << it->Attributes[0]
                      << "\t\t ClusterID: " << it->CID.ToText()
                      << std::endl;
        }
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

