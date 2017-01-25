
#ifndef YGOR_CLUSTERING_CLUSTERID_HPP
#define YGOR_CLUSTERING_CLUSTERID_HPP

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
#include <set>
#include <random>
#include <sstream>

//Utility class for (integral) cluster IDs. This class is essentially an integer (this->Raw)
// wrapped with a bit of logic concerning getting the 'next' available ID and a few reserved
// values.
//
// At the moment, there are three types of IDs:
//   1. Unclassified -- which have not yet been clustered, or were not able to be clustered.
//   2. Noise        -- datum which are 'thought' to not belong to any clusters.
//   3. Regular IDs  -- a number indicating which cluster the datum belongs to.
//
template <class T> class ClusterID {
    public:
        T Raw;
    
        static const auto Unclassified = std::numeric_limits<T>::max();
        static const auto Noise = std::numeric_limits<T>::max() - static_cast<T>(1);
        static const auto Cluster0 = std::numeric_limits<T>::min(); //Cluster1 = Cluster0 + 1, etc..
    
        constexpr ClusterID(void) : Raw(ClusterID<T>::Unclassified) { }
        constexpr ClusterID(const ClusterID<T> &in) : Raw(in.Raw) { }
        constexpr ClusterID(T in) : Raw(in) { }
   
        bool operator<(const ClusterID<T> &rhs) const {
            return (this->Raw < rhs.Raw);
        }
        bool operator==(const ClusterID<T> &rhs) const {
            if(this == &rhs) return true;
            return (this->Raw == rhs.Raw);
        }
 
        constexpr bool IsNoise(void) const {
            return (this->Raw == this->Noise);
        }
    
        constexpr bool IsUnclassified(void) const {
            return (this->Raw == this->Unclassified);
        }
    
        constexpr bool IsRegular(void) const {
            return (this->Raw != this->Unclassified) && (this->Raw != this->Noise);
        }
   
        bool IsNoise(T in) const {
            return (in == this->Noise);
        }

        bool IsUnclassified(T in) const {
            return (in == this->Unclassified);
        }

        bool IsRegular(T in) const {
            return (in != this->Unclassified) && (in != this->Noise);
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

#endif //YGOR_CLUSTERING_CLUSTERID_HPP
