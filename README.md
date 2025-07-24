# raycandle
simple library for plotting candlesticks
api similar to matplotlib

## installation


1. clone the repo 
```bash 
  git clone https://github.com/fskamau/raycandle.git raycandle
  cd raycandle
  ```
  
2. clone raylib and build it statically with POC

 ```bash 
 git clone --depth=1 https://github.com/raysan5/raylib.git Raylib
 cd Raylib
 mkdir build && cd build 
 cmake -DCMAKE_INSTALL_PREFIX=../../raylib -DBUILD_SHARED_LIBS=OFF  -DCMAKE_BUILD_TYPE=Release  -DCMAKE_POSITION_INDEPENDENT_CODE=ON  ..
 make -j8
 make install
 cd ../..
 rm -rf Raylib
 ```
 
 3. build raycandle with raylib 
 ```bash
(
 set -e
 cd raycandle/craycandle
 make
 mv build/* ../
 cd ..
 rm -r craycandle
 set +e
) 
```
3. now, raylib can be removed safely
```
rm -r raylib
```
4. run pip install
 ```bash 
  pip install .
 ```
	 
