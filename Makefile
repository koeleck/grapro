.PHONY: all clean distclean


all: build/Makefile
	@make -C build
	@mkdir -p bin
	@cp build/grapro bin

build/Makefile: CMakeLists.txt
	@mkdir -p build
	@cd build; cmake ..

clean:
	make -C build clean
	rm -rf cache imgui.ini

distclean:
	rm -rf build cache imgui.ini
