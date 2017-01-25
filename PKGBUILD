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
   'boost-libs' # Boost.geometry specifically (which is header-only).
)
# conflicts=()
# replaces=()
# backup=()
# install='foo.install'
source=("git+https://gitlab.com/hdeanclark/YgorClustering.git")
md5sums=('SKIP')
sha1sums=('SKIP')

#build() {
#  # Nothing to build...
#}

package() {
  mkdir -p "${pkgdir}"/usr/include/
  install -Dm644 "${srcdir}/YgorClustering/src/"*hpp "${pkgdir}"/usr/include/
}

# vim:set ts=2 sw=2 et:
