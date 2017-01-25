# Maintainer: Hal Clark <gmail.com[at]hdeanclark>
pkgname=ygorclustering
pkgver=20170125_102000
pkgver() {
  date +%Y%m%d_%H%M%S
}
pkgrel=1

pkgdesc="C++ header-only library for data clustering."
url="http://www.halclark.ca"
arch=('x86_64' 'i686' 'armv7h')
license=('unknown')
depends=(
   'boost-libs' # Boost.geometry specifically (which is header-only).
)
# conflicts=()
# replaces=()
# backup=()
# install='foo.install'
source=("git+https://gitlab.com/hdeanclark/YgorClustering.git")
md5sums=('SKIP')
sha1sums=('SKIP')

#options=(!strip staticlibs)
options=(strip staticlibs)

build() {
  # Nothing to build...
}

package() {
  install -Dm644 "${srcdir}"*hpp /usr/include/
}

# vim:set ts=2 sw=2 et:
