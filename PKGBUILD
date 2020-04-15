# Maintainer: Hal Clark <gmail.com[at]hdeanclark>
pkgname=ygorclustering
pkgver=20170125_125726
pkgver() {
  date +%Y%m%d_%H%M%S
}
pkgrel=1

pkgdesc="C++ header-only library for data clustering."
url="http://www.halclark.ca"
arch=('x86_64' 'i686' 'armv7h')
license=('GPLv3+')
depends=(
   'boost' # Boost.geometry specifically (which is header-only).
)
makedepends=(
   'cmake'
   'boost'
)
optdepends=(
   'boost' # User build header-only AND/OR library optional dependency.
)
# conflicts=()
# replaces=()
# backup=()
# install='foo.install'
#source=("git+https://gitlab.com/hdeanclark/YgorClustering.git")
#md5sums=('SKIP')
#sha1sums=('SKIP')

#options=(!strip staticlibs)
options=(strip staticlibs)
#PKGEXT='.pkg.tar' # Disable compression.

build() {
  # Nothing to build here, just prepping for install.
  #cmake "${pkgdir}" ...
  cmake \
    ../ \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_BUILD_TYPE=Release 
  make VERBOSE=1
}

package() {
  make DESTDIR="${pkgdir}" install
}

# vim:set ts=2 sw=2 et:
