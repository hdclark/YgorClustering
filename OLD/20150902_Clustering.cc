
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

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

#include <boost/geometry/index/parameters.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/io/svg/svg_mapper.hpp>


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
           typename UserDataClass = ClusteringUserDataEmptyClass > //Used to attach generic data, if desired. Will be copied!
class ClusteringDatum {
    public:
        //Type-defs and constants. (For accessibility after instantiation.)
        constexpr static size_t SpatialDimensionCount_ = SpatialDimensionCount;
        typedef SpatialType SpatialType_;
        constexpr static size_t AttributeDimensionCount_ = AttributeDimensionCount;
        typedef AttributeType AttributeType_;
        typedef ClusterIDType ClusterIDType_;

        //Data members.
        std::array<SpatialType, SpatialDimensionCount> Coordinates; //For spatial indexing in the R*-tree.
        std::array<AttributeType, AttributeDimensionCount> Attributes; //May be indexed in the R*-tree. Depends on algorithm.
        ClusterID<ClusterIDType> CID;
        UserDataClass UserData;

        //Constructors.
        ClusteringDatum() { };
        ClusteringDatum(const ClusteringDatum &in) { *this = in; };
        constexpr ClusteringDatum(const decltype(Coordinates) &in) : Coordinates(in) { };
        constexpr ClusteringDatum(const decltype(Coordinates) &a, 
                       const decltype(Attributes) &b) : Coordinates(a), Attributes(b) { };

        //Member functions.
        ClusteringDatum & operator=(const ClusteringDatum &rhs){
           if(this == &rhs) return *this;
           this->Coordinates   = rhs.Coordinates;
           this->Attributes    = rhs.Attributes;
           this->CID           = rhs.CID;
           this->UserData      = rhs.UserData;           
           return *this;
        }

        bool operator==(const ClusteringDatum &in) const {
            //Default only considers spatial information so it can be used in spatial indexes.
            return (this->Coordinates == in.Coordinates);
        }

        ClusteringDatum CoordinateAlignedBBoxMinimal(SpatialType FullEdgeLength) const {
            //Returns a point, copied from *this, but with spatial coords shifted to a "minimal" coordinate-aligned
            // bounding box corner. The shift is computed so that a box constructed with the "minimal" and "maximal"
            // points will have edge lengths FullEdgeLength.
            // 
            // Bounding box edges will all be aligned with a de-facto cartesian coordinate system.
            //
            // This routine is useful for constructing a bounding box encompassing a spherical region surrounding
            // *this. This box is useful for spatial indexing purposes, such as performing 'within(BBox)' queries.
            ClusteringDatum out(*this);
            for(auto & c : out.Coordinates) c -= FullEdgeLength / static_cast<SpatialType>(2);
            return out;
        }

        ClusteringDatum CoordinateAlignedBBoxMaximal(SpatialType FullEdgeLength) const {
            //Returns a point, copied from *this, but with spatial coords shifted to a "minimal" coordinate-aligned
            // bounding box corner. The shift is computed so that a box constructed with the "minimal" and "maximal"
            // points will have edge lengths FullEdgeLength.
            // 
            // Bounding box edges will all be aligned with a de-facto cartesian coordinate system.
            //
            // This routine is useful for constructing a bounding box encompassing a spherical region surrounding
            // *this. This box is useful for spatial indexing purposes, such as performing 'within(BBox)' queries.
            ClusteringDatum out(*this);
            for(auto & c : out.Coordinates) c += FullEdgeLength / static_cast<SpatialType>(2);
            return out;
        }

};


namespace boost { namespace geometry { namespace traits {
    //Adapt ClusteringDatum to be a Boost Geometry "point" type.

    template < std::size_t SpatialDimensionCount,
               typename SpatialType,
               std::size_t AttributeDimensionCount,
               typename AttributeType,
               typename ClusterIDType,
               typename UserDataClass >
    struct tag< 
          ClusteringDatum< SpatialDimensionCount,
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
                      ClusteringDatum< SpatialDimensionCount,
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
                        ClusteringDatum< SpatialDimensionCount,
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
                ClusteringDatum< SpatialDimensionCount,
                      SpatialType,
                      AttributeDimensionCount,
                      AttributeType,
                      ClusterIDType,
                      UserDataClass > 
                                    > : boost::mpl::int_< SpatialDimensionCount > { };


    template < std::size_t SpatialDimensionCount,
               typename SpatialType,
               std::size_t AttributeDimensionCount,
               typename AttributeType,
               typename ClusterIDType,
               typename UserDataClass,
               std::size_t ElementNumber > // <-- (Note extra access template parameter.)
    struct access<
             ClusteringDatum< SpatialDimensionCount,
                   SpatialType,
                   AttributeDimensionCount,
                   AttributeType,
                   ClusterIDType,
                   UserDataClass >,
             ElementNumber       > {

        static inline SpatialType get(const ClusteringDatum< SpatialDimensionCount,
                                                  SpatialType,
                                                  AttributeDimensionCount,
                                                  AttributeType,
                                                  ClusterIDType,
                                                  UserDataClass > &in){
            return std::get<ElementNumber>(in.Coordinates);
            //Could use static template methods to include the Attributes as 'higher spatial dimensions' here.
        }

        static inline void set(ClusteringDatum< SpatialDimensionCount,
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



int main(){

    typedef ClusteringDatum<2, double, 1, float, uint16_t> CDat;

    constexpr size_t MaxElementsInANode = 6; // 16, 32, 128, 256, ... ?
    typedef boost::geometry::index::rstar<MaxElementsInANode> RTreeParameter_t;
    typedef boost::geometry::index::rtree<CDat,RTreeParameter_t> RTree_t;
    typedef boost::geometry::model::box<CDat> Box_t;

    constexpr auto RTreeSpatialQueryGetAll = [](const CDat &) -> bool { return true; };

    RTree_t rtree;


    if(false){ //Silly little basic test with phony data.
        for(double f = 0 ; f < 10 ; f += 1) rtree.insert(CDat( {f,f}, {static_cast<float>(f)} ));
    
        //------------- Output
        {
          std::ofstream svg("Visualized.svg");
          boost::geometry::svg_mapper<CDat> mapper(svg, 1280, 1024);
    
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
    
        CDat QueryPoint({5.1,5.1});
    
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
        rtree.insert(CDat( { -3.0, 0.0 }, {0.0f} ));
        rtree.insert(CDat( { -2.8, 0.1 }, {1.0f} ));
        rtree.insert(CDat( { -2.7, 0.2 }, {0.0f} ));
        rtree.insert(CDat( { -2.5, 0.5 }, {0.0f} ));
        rtree.insert(CDat( {  2.0, 0.2 }, {1.0f} ));
        rtree.insert(CDat( {  2.1, 1.1 }, {1.0f} ));
        rtree.insert(CDat( {  2.7, 1.5 }, {0.0f} ));
        rtree.insert(CDat( {  6.7, 6.5 }, {0.0f} ));
        rtree.insert(CDat( {  6.7, 7.5 }, {0.0f} ));
        rtree.insert(CDat( {  7.7, 6.5 }, {0.0f} ));
        rtree.insert(CDat( {  6.7, 5.5 }, {0.0f} ));
        rtree.insert(CDat( {  5.7, 6.5 }, {0.0f} ));
    //    for(double x = -100000.0 ; x < 100000.0 ; x += 1.0) rtree.insert(CDat( {x,x}, {static_cast<float>(x)} ));
    
        CDat apoint({ -2.8, 0.1 }, {1.0f});
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
    rtree.insert(CDat( { -3.0, 0.0 }, {0.0f} ));
    rtree.insert(CDat( { -2.8, 0.1 }, {1.0f} ));
    rtree.insert(CDat( { -2.7, 0.2 }, {0.0f} ));
    rtree.insert(CDat( { -2.5, 0.5 }, {0.0f} ));
    rtree.insert(CDat( {  2.0, 0.2 }, {1.0f} ));
    rtree.insert(CDat( {  2.1, 1.1 }, {1.0f} ));
    rtree.insert(CDat( {  2.7, 1.5 }, {0.0f} ));
    rtree.insert(CDat( {  6.7, 6.5 }, {0.0f} ));
    rtree.insert(CDat( {  6.7, 7.5 }, {0.0f} ));
    rtree.insert(CDat( {  7.7, 6.5 }, {0.0f} ));
    rtree.insert(CDat( {  6.7, 5.5 }, {0.0f} ));
    rtree.insert(CDat( {  5.7, 6.5 }, {0.0f} ));
    for(double x = -100'000.0 ; x < 100'000.0 ; x += 1.0) rtree.insert(CDat( {x,x}, {static_cast<float>(x)} ));


    enum SpatialQueryTechnique {
        UseNearby,
        UseWithin
    };


    //START FUNCTION HERE.
    //
    //DBSCAN( this->rtree, Eps, MinPts ){...

    //Specify/harvest user parameters.
    const size_t MinPts = CDat::SpatialDimensionCount_*2; //DBSCAN algorithm parameter. Minimum # of points in a cluster.
    const double Eps = 1.5; //DBSCAN algorithm parameter. Sets scale: is the distance between which points are considered 'sufficiently' close.
    const SpatialQueryTechnique UsersSpatialQueryTechnique = SpatialQueryTechnique::UseWithin;
    

    //Ensure all datum start with Unclassified ClusterIDs.(Note: This step is necessary in case the
    // user has re-run the algorithm, tampered with the data, etc..)
    {
        RTree_t::const_query_iterator it;
        it = rtree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
        for( ; it != rtree.qend(); ++it){
            const_cast<CDat &>(*it).CID.Raw = ClusterID<CDat::ClusterIDType_>::Unclassified;
        }
    }


    //Main loop.
    {
        const std::string ThrowSelfPointCheck = "Spatial vicinity queries should always return the self point."
                                                " This point is missing, indicating numerical stability issues"
                                                " or logical errors in the spatial indexing approach.";
        const std::string ThrowNotImplemented = "Specified spatial query technique has not been implemented.";
                                  
        auto WorkingCID = ClusterID<CDat::ClusterIDType_>().NextValidClusterID();

        RTree_t::const_query_iterator outer_it;
        outer_it = rtree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
        for( ; outer_it != rtree.qend(); ++outer_it){
            if(!outer_it->CID.IsUnclassified()) continue;

            //EXPAND_CLUSTER(this->rtree, outer_it, WorkingCID, Eps, MinPts)
	    bool IterateWorkingClusterID = false;
            //////////////////////////////////////////////

            //Query for nearby items within a distance Eps from the current point "*outer_it".
            std::list<RTree_t::const_query_iterator> seed_its;
            if(UsersSpatialQueryTechnique == SpatialQueryTechnique::UseNearby){
                RTree_t::const_query_iterator nearby_it;
                nearby_it = rtree.qbegin( boost::geometry::index::nearest( *outer_it, rtree.size() ) );
                if(nearby_it == rtree.qend()) throw std::runtime_error(ThrowSelfPointCheck);
                for( ; nearby_it != rtree.qend(); ++nearby_it){
                    if(boost::geometry::distance(*outer_it, *nearby_it) < Eps){
                        seed_its.push_back( nearby_it );
                    }else{
                        break;
                    }
                }
            }else if(UsersSpatialQueryTechnique == SpatialQueryTechnique::UseWithin){
                //This box is aligned with the cartesian grid and bounds the hyper-sphere of radius Eps.
                Box_t BBox( outer_it->CoordinateAlignedBBoxMinimal(Eps*2.0),
                            outer_it->CoordinateAlignedBBoxMaximal(Eps*2.0) );

                RTree_t::const_query_iterator nearby_it;
                nearby_it = rtree.qbegin( boost::geometry::index::within( BBox ) );
                if(nearby_it == rtree.qend()) throw std::runtime_error(ThrowSelfPointCheck);
                for( ; nearby_it != rtree.qend(); ++nearby_it){
                    if(boost::geometry::distance(*outer_it, *nearby_it) < Eps){
                        seed_its.push_back( nearby_it );
                    }
                }

            }else{
                throw std::runtime_error(ThrowNotImplemented);
            }

            if(seed_its.size() < MinPts){
                const_cast<CDat &>(*outer_it).CID.Raw = ClusterID<CDat::ClusterIDType_>::Noise;
                IterateWorkingClusterID = false;
            }else{ 
                //All datum in `seeds` are "density-reachable" from current point "*outer_it".
                // So we update their ClusterID.
                for(auto seed_it : seed_its) const_cast<CDat &>(*seed_it).CID = WorkingCID;

                //Remove the self point from the nearby points query.
                // Note: boost::geometry::index::nearest() will return the nearest first. Since the
                // distance between any point and itself should always be zero, the first point will
                // be the self point. So we compare the addresses to be certain. The following
                // should only ever fail if Boost::Geometry fails to find nearby points properly!
                {
                    auto IsTheSelfPoint = [outer_it](const RTree_t::const_query_iterator &in_it) -> bool {
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
                    std::list<RTree_t::const_query_iterator> result_its;
                    if(UsersSpatialQueryTechnique == SpatialQueryTechnique::UseNearby){
                        RTree_t::const_query_iterator nearby_it;
                        nearby_it = rtree.qbegin( boost::geometry::index::nearest( *currentP_it, rtree.size() ) );
                        if(nearby_it == rtree.qend()) throw std::runtime_error(ThrowSelfPointCheck);
                        for( ; nearby_it != rtree.qend(); ++nearby_it){
                            if(boost::geometry::distance(*currentP_it, *nearby_it) < Eps){
                                result_its.push_back( nearby_it );
                            }else{
                                break;
                            }
                        }
                    }else if(UsersSpatialQueryTechnique == SpatialQueryTechnique::UseWithin){
                        //This box is aligned with the cartesian grid and bounds the hyper-sphere of radius Eps.
                        Box_t BBox( currentP_it->CoordinateAlignedBBoxMinimal(Eps*2.0),
                                    currentP_it->CoordinateAlignedBBoxMaximal(Eps*2.0) );
                
                        RTree_t::const_query_iterator nearby_it;
                        nearby_it = rtree.qbegin( boost::geometry::index::within( BBox ) );
                        if(nearby_it == rtree.qend()) throw std::runtime_error(ThrowSelfPointCheck);
                        for( ; nearby_it != rtree.qend(); ++nearby_it){
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
                                const_cast<CDat &>(*result_it).CID = WorkingCID;
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
    if(false){
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

