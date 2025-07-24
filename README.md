# raycandle
simple library for plotting candlesticks
api similar to matplotlib

## installation

1. clone raylib into raycandle folder and build statically

 ```bash 
 mkdir raycandle && cd raycandle
 git clone --depth=1 https://github.com/raysan5/raylib.git raylib
 cd raylib
 mkdir build && cd build 
 cmake -DCMAKE_INSTALL_PREFIX=../../local -DBUILD_SHARED_LIBS=OFF  -DCMAKE_BUILD_TYPE=Release  ..
 make -j8
 make install
 cd ../..
 rm -rf raylib
 mv local raylib
 ```
 
 2. clone raycandle and build with raylib
 
 ```bash
 git clone https://github.com/fskamau/raycandle.git raycandle
 cd raycandle/cray
 make 
 
 ```
 
 
