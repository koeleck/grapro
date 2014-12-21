.PHONY: all clean distclean


all: build/Makefile
	@make -C build -j
	@mkdir -p bin
	@cp build/grapro bin

build/Makefile: CMakeLists.txt
	@mkdir -p build
	@cd build; cmake ..

clean:
	make -C build clean

distclean:
	rm -rf build
