pkgname=clot-lang
pkgver=0.1.0
pkgrel=1
pkgdesc="Clot programming language interpreter"
arch=('x86_64')
url='https://github.com/jclot/ClotLang'
license=('MIT')
depends=()
source=("ClotProgrammingLanguage/*")
sha256sums=('SKIP')

build() {
  cd "${srcdir}/ClotProgrammingLanguage"
  g++ -std=c++17 -O2 -pipe -flto *.cpp -o clot
}

package() {
  install -Dm755 "${srcdir}/ClotProgrammingLanguage/clot" "${pkgdir}/usr/bin/clot"
  install -Dm644 "${srcdir}/ClotProgrammingLanguage/test.clot" "${pkgdir}/usr/share/clot/test.clot"
}
