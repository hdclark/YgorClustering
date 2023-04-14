
#ifndef YGOR_CLUSTERING_CLUSTERINGDATUM_HPP
#define YGOR_CLUSTERING_CLUSTERINGDATUM_HPP

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

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

#include <boost/geometry/index/parameters.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/io/svg/svg_mapper.hpp>


class ClusteringUserDataEmptyClass { }; //Used to optionally forgo passing in extra data.


//An example class which is destined for a clustering algorithm. This class is heavily templated so it can
// be easily extended by the user.
//
// The type and dimension of spatial coordinates and non-spatial attributes can be adjusted. Additionally, 
// arbitrary user data can be attached to each datum for whatever reason. This data is not used by the 
// clustering algorithms, but (many) copies are likely to be made.
//
// You can choose to instantiate this template as needed, or use one of several pre-instantiated versions
// by including an extra header file. If you're wondering whether a certain type will work, the best way to
// find out is simply to try compiling with it. If it doesn't work, Boost.Geometry will let you know by 
// claiming many templated functions/structs are missing.
// 
template < std::size_t SpatialDimensionCount,   //For 3D data, this would be 3.
           typename SpatialType,                //Generally a double.
           std::size_t AttributeDimensionCount, //Depends on the user's needs. Probably 0 or 1.
           typename AttributeType,              //Depends on the user's needs. Probably float or double.
           typename ClusterIDType = uint32_t,   //The *underlying* type. Generally uint16_t or uint32_t.
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
        ClusteringDatum() {
            this->Coordinates.fill( SpatialType() );
            this->Attributes.fill( AttributeType() );
            this->CID = ClusterID<ClusterIDType>();
            this->UserData = UserDataClass();
        };
        ClusteringDatum(const ClusteringDatum &in){
            *this = in;
        };
        constexpr ClusteringDatum(const decltype(Coordinates) &in) : Coordinates(in) {
            this->Attributes.fill( AttributeType() );
            this->CID = ClusterID<ClusterIDType>();
            this->UserData = UserDataClass();
        };
        constexpr ClusteringDatum(const decltype(Coordinates) &a, 
                                  const decltype(Attributes) &b) : Coordinates(a), Attributes(b) {
            this->CID = ClusterID<ClusterIDType>();
            this->UserData = UserDataClass();
        };
        ClusteringDatum(const decltype(Coordinates) &a,
                        const decltype(Attributes) &b,
                        const decltype(UserData) &c) : Coordinates(a), Attributes(b), UserData(c) {
            this->CID = ClusterID<ClusterIDType>();
        };

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
            // Bounding box edges will all be aligned with a de-facto cartesian, Euclidean coordinate system.
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
            // Bounding box edges will all be aligned with a de-facto cartesian, Euclidean coordinate system.
            //
            // This routine is useful for constructing a bounding box encompassing a spherical region surrounding
            // *this. This box is useful for spatial indexing purposes, such as performing 'within(BBox)' queries.
            ClusteringDatum out(*this);
            for(auto & c : out.Coordinates) c += HalfEdgeLength;
            return out;
        }

};



//This code 'registers' the ClusteringDatum class so it can be used as a Boost Geometry "point" type.
namespace boost { namespace geometry { namespace traits {

    //Tell Boost.Geometry we have a point-like class. (As opposed to a polygon-like class, etc..)
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

    //The C++ type our spatial data will have. Probably double, but maybe something more interesting.
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

    //Tell Boost.Geometry we are assuming a cartesian system, (not spherical or something more exotic).
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

    //Tell Boost.Geometry the dimensionality of our spatial data. (Attribute dimensions not included at the moment.)
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


    //The most important registrations: tell Boost.Geometry how to access the spatial coordinate members.
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
            //Note: could use static template methods to include the Attributes as 'higher spatial dimensions' here.
        }

        static inline void set(ClusteringDatum< SpatialDimensionCount,
                                     SpatialType,
                                     AttributeDimensionCount,
                                     AttributeType,
                                     ClusterIDType,
                                     UserDataClass > &in, 
                               const SpatialType &val){
            std::get<ElementNumber>(in.Coordinates) = val;
            //Note: could use static template methods to include the Attributes as 'higher spatial dimensions' here.
            return;
        }
    };

} } } // namespace boost::geometry::traits


#endif //YGOR_CLUSTERING_CLUSTERINGDATUM_HPP
