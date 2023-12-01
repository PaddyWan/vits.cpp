run: build
	./build/main

build:
	mkdir -p build
	cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Release
	(cd build && make)

clean:
	rm -rf ./build

tests: build
	./build/tests

bench-simd: build
	./build/bench-simd

bench: build
	./build/bench --benchmark_time_unit=ms

.PHONY: all build run clean bench