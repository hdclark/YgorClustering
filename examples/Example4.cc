
// Example4.cc -- clusters files based on modification time.

#include <iostream>
#include <fstream>
#include <vector>
#include <list>
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
#include <type_traits>
#include <chrono>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <boost/date_time.hpp>

#include "YgorClustering.hpp"
//#include "YgorClusteringDatumCommonInstantiations.hpp"


int main(int argc, char* argv[]){

    if(argc <= 1){
        std::cerr << "Usage: " << argv[0] << " <path_to_photo_directory>" << std::endl;
        return 1;
    }

    std::list<boost::filesystem::path> SpecifiedURIs = { argv[1] };

    std::cout << "Considering the following seed URIs:" << std::endl;
    for(const auto &p : SpecifiedURIs) std::cout << "    " << p.string() << std::endl;

    //Verify each path is a reachable file or directory. Discard those that are not.
    {
        boost::filesystem::path PathShuttle;
        auto it = SpecifiedURIs.begin();
        while(it != SpecifiedURIs.end()){
            bool wasOK = false;
            try{
                PathShuttle = boost::filesystem::canonical(*it);
                wasOK = boost::filesystem::exists(PathShuttle);
            }catch(const boost::filesystem::filesystem_error &){ }

            if(wasOK){
                //std::cout << "Able to resolve " << *it << " to " << PathShuttle << "" << std::endl;
                *it = PathShuttle;
                ++it;
            }else{
                std::cerr << "Unable to resolve " << *it << ". Ignoring it." << std::endl;
                it = SpecifiedURIs.erase(it);
            }
        }
    }


    //Recursively find all files in the remaining specified URIs.
    std::list<boost::filesystem::path> EnumeratedFiles;
    {
        for(auto it = SpecifiedURIs.begin(); it != SpecifiedURIs.end(); ++it){
            try{
                if(boost::filesystem::is_regular_file(*it) ){
                //&& (std::string(boost::filesystem::extension(*it)) == std::string(".jpg"))){
                    if(boost::filesystem::hard_link_count(*it) == 1){
                        EnumeratedFiles.emplace_back(boost::filesystem::canonical(*it));
                    }else{
                        std::cerr << "Encountered a hard-linked file. Refusing to consider it." << std::endl;
                    }
                }else if(boost::filesystem::is_directory(*it) && !boost::filesystem::is_symlink(*it)){
                    auto it_orig_next = std::next(it); //Keep the ordering by maintaining an insertion iterator.
                    for(const auto & DirectoryEntry : boost::filesystem::directory_iterator(*it)){
                        SpecifiedURIs.insert(it_orig_next,DirectoryEntry.path());
                    }
                }
            }catch(const boost::filesystem::filesystem_error &){ }
        }
    }

    //Remove URIs which have been specified more than once.
    {
        EnumeratedFiles.sort();
        EnumeratedFiles.unique();
    }
    std::cout << "Considering the following files:" << std::endl;
    for(const auto &p : EnumeratedFiles) std::cout << "    " << p.string() << std::endl;

    //Find a timestamp for each file. Attach the data to a ClusteringDatum_t and insert into a tree.
    constexpr size_t MaxElementsInANode = 6; // 16, 32, 128, 256, ... ?
    typedef boost::geometry::index::rstar<MaxElementsInANode> RTreeParameter_t;

    //typedef std::pair<boost::filesystem::path, std::time_t> UserData_t;
    typedef boost::filesystem::path UserData_t;
    typedef ClusteringDatum<1, double, 0, double, uint32_t, UserData_t> CDat_t;
    typedef boost::geometry::index::rtree<CDat_t,RTreeParameter_t> RTree_t;

    RTree_t rtree;
    uint64_t BeforeCount = 0;

    const std::time_t BaseTime = 0;
    //static_assert(std::is_floating_point<std::time_t>::value, "std::time_t is not a floating point type!");

    {
        for(auto & EnumeratedFile : EnumeratedFiles){
            try{
                const auto modtime = boost::filesystem::last_write_time(EnumeratedFile);
                const auto timedelta = std::difftime(modtime,BaseTime);
                rtree.insert(CDat_t({ timedelta }, { }, EnumeratedFile));
                ++BeforeCount;

            }catch(const boost::filesystem::filesystem_error &){ }
        }
    }

    std::cout << "Number of photos being considered: " << BeforeCount << std::endl;

    const size_t MinPts = 3;
    const double Eps = 3600.0 * 12.0; //Seconds.
    
    DBSCAN<RTree_t,CDat_t>(rtree,Eps,MinPts);


    //Print out the points with cluster info.
    if(false){
        constexpr auto RTreeSpatialQueryGetAll = [](const CDat_t &) -> bool { return true; };
        RTree_t::const_query_iterator it;
        it = rtree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
        for( ; it != rtree.qend(); ++it){
            //std::cout << "  Point: " << boost::geometry::wkt(*it)
            std::cout << "ClusterID: " << it->CID.ToText()
                      << "\t\t Filename: " << it->UserData
                      << std::endl;
        }
    }


    //Segregate the data based on ClusterID.
    std::map<uint32_t, std::list<CDat_t> > Segregated;
    if(true){
        constexpr auto RTreeSpatialQueryGetAll = [](const CDat_t &) -> bool { return true; };
        RTree_t::const_query_iterator it;
        it = rtree.qbegin(boost::geometry::index::satisfies( RTreeSpatialQueryGetAll ));
        for( ; it != rtree.qend(); ++it){
            Segregated[it->CID.Raw].push_back( *it );
        }
    }

    std::cout << "There are " << Segregated.size() << " clusters" << std::endl;
    std::cout << "There are " << [&Segregated](void) -> size_t {
                                     size_t out = 0;
                                     for(const auto & x : Segregated) out += x.second.size();
                                     return out; }() 
                              << " elements after." << std::endl;
    std::cout << "(There should be " << BeforeCount << " )" << std::endl;



    //Write the filenames to 'playlist' files for easy viewing.
    const std::string base("/tmp/clusters/");
    boost::filesystem::create_directories(base);

    const auto PredicateSortOnFilename = [](const CDat_t &L, const CDat_t &R) -> bool {
        return (L.UserData < R.UserData);
    };

    for(auto & Cluster : Segregated){
        Cluster.second.sort(PredicateSortOnFilename);

        const std::string cluster_FN = base + std::to_string(Cluster.first);
        std::cout << "    Writing cluster '" << cluster_FN << "'" << std::endl;

        std::ofstream FO(cluster_FN);
        for(auto & FN : Cluster.second) FO << FN.UserData.native() << std::endl;
        FO.flush();
        FO.close();
    }
    std::cout << "Cluster playlists are in '" << base << "'" << std::endl;

    return 0;
}

