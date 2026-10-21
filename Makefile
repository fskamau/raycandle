.PHONY: all install clean
all:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	cmake --build build --config Release --parallel
install: all
	cmake --install build --prefix .
clean:
	cmake -E rm -rf build
