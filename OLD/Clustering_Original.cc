
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
           typename ClusterIDType = uint32_t,   //Generally uint16_t or uint32_t.
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

        ClusteringDatum CoordinateAlignedBBoxMinimal(SpatialType HalfEdgeLength) const {
            //Returns a point, copied from *this, but with spatial coords shifted to a "minimal" coordinate-aligned
            // bounding box corner. The shift is computed so that a box constructed with the "minimal" and "maximal"
            // points will have edge lengths 2*HalfEdgeLength.
            // 
            // Bounding box edges will all be aligned with a de-facto cartesian coordinate system.
            //
            // This routine is useful for constructing a bounding box encompassing a spherical region surrounding
            // *this. This box is useful for spatial indexing purposes, such as performing 'within(BBox)' queries.
            ClusteringDatum out(*this);
            for(auto & c : out.Coordinates) c -= HalfEdgeLength;
            return out;
        }

        ClusteringDatum CoordinateAlignedBBoxMaximal(SpatialType HalfEdgeLength) const {
            //Returns a point, copied from *this, but with spatial coords shifted to a "minimal" coordinate-aligned
            // bounding box corner. The shift is computed so that a box constructed with the "minimal" and "maximal"
            // points will have edge lengths 2*HalfEdgeLength.
            // 
            // Bounding box edges will all be aligned with a de-facto cartesian coordinate system.
            //
            // This routine is useful for constructing a bounding box encompassing a spherical region surrounding
            // *this. This box is useful for spatial indexing purposes, such as performing 'within(BBox)' queries.
            ClusteringDatum out(*this);
            for(auto & c : out.Coordinates) c += HalfEdgeLength;
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

    //User parameters:
    //
    // 1. RTree --> The R*-tree already loaded with the data to be clustered. It will be modified in-place.
    // 2. Eps --> DBSCAN algorithm parameter. Sets the scale. It is the distance between which points are
    //            considered 'sufficiently' close. (See lengthy note below.)
    // 3. MinPts --> DBSCAN algorithm parameter. Sets the minimal number of nearby connections each point
    //               must have. Authors recommend 2x the dimension.
    // 4. SpatialQueryTechnique --> Specify the method used to find points in the vicinity of a given point.
    //                              Makes a large impact on performance!
    //
    // NOTE: This routine is an implementation of the DBSCAN clustering algorithm described in:
    //       "A Density-Based Algorithm for Discovering Clusters" by Ester, Kriegel, Sander, and Xu in 1996.
    //        This is a widely-cited paper and widely-known algorithm.
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

    //Specify/harvest user parameters.
    //const size_t MinPts = ClusteringDatum_t::SpatialDimensionCount_*2; //DBSCAN algorithm parameter. Minimum # of points in a cluster.
    //const double Eps = 1.5; //DBSCAN algorithm parameter. Sets scale: is the distance between which points are considered 'sufficiently' close.
    //const SpatialQueryTechnique UsersSpatialQueryTechnique = SpatialQueryTechnique::UseWithin;
    
    //Needed to work around missing RTree_t.begin()/end() when Boost.Geometry version < 1.58.0.
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

//	    bool IterateWorkingClusterID = false;

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
//IterateWorkingClusterID = false;
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
//                IterateWorkingClusterID = true;
            }
            WorkingCID = WorkingCID.NextValidClusterID();
//            if(IterateWorkingClusterID) WorkingCID = WorkingCID.NextValidClusterID();
        }

    }
    return;
}




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

