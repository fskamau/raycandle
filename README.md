n# raycandle
simple library for plotting candlesticks
api similar to matplotlib

## installation

1. clone raylib into raycandle folder and build statically
 - Note: No need to reinstall if you already have a local static fPIC build

 ```bash 
 mkdir raycandle && cd raycandle
 git clone --depth=1 https://github.com/raysan5/raylib.git Raylib
 cd Raylib
 mkdir build && cd build 
 cmake -DCMAKE_INSTALL_PREFIX=../../raylib -DBUILD_SHARED_LIBS=OFF  -DCMAKE_BUILD_TYPE=Release  -DCMAKE_POSITION_INDEPENDENT_CODE=ON  ..
 make -j8
 make install
 cd ../..
 rm -rf Raylib
 ```
 
 2. clone raycandle and build with raylib
 
 ```bash

(
  set -e
  git clone https://github.com/fskamau/raycandle.git raycandle
  cd raycandle/raycandle
  make
  mv build/* ../
  cd ..
  rm -r raycandle
) 
```
3. now raylib can be removed safely

rm -r raylib

4. run pip install
 
