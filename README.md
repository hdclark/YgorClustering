
[![Latest Release DOI](https://zenodo.org/badge/89645920.svg)](https://zenodo.org/badge/latestdoi/89645920)

## About

This is a C++ library for clustering data. It uses Boost.Geometry R\*-trees for
fast indexing. The DBSCAN implementation can cluster 20 million 2D datum in
around an hour, and 20 thousand in seconds.

At the moment, only (vanilla) DBSCAN is implemented. It is based on the article
"A Density-Based Algorithm for Discovering Clusters" by Ester, Kriegel, Sander,
and Xu in 1996. DBSCAN is generally regarded as a solid, reliable clustering
technique compared with techniques such as k-means (which is, for example,
unable to cluster concave clusters).

Other clustering techniques are planned.


## Dependencies

This library requires Boost.Geometry (also header only) version >= 1.57.0.
Earlier versions will most likely still work but have not been tested.


## Installation

This library is header-only.
- It does not need to be linked.
- It has no run-time dependencies.
- It can be installed to, e.g., /usr/include/, or kept in-source as needed.

A helper script can be used to build a package for Arch Linux or install
directly (all other distributions).


## License and Copying

All materials herein which may be copywrited, where applicable, are. Copyright
2015, 2017, 2018, 2019, 2020, 2021, 2022 hal clark. See the LICENSE file for
details about the license. Informally, YgorClustering is available under a
GPLv3+ license.

All liability is herefore disclaimed. The person(s) who use this source and/or
software do so strictly under their own volition. They assume all associated
liability for use and misuse, including but not limited to damages, harm,
injury, and death which may result, including but not limited to that arising
from unforeseen and unanticipated implementation defects.

