.PHONY: all clean distclean


all: build/Makefile
	@make -C build -j
	@mkdir -p bin
	@cp build/grapro bin

build/Makefile: CMakeLists.txt src/framework/vars.def
	@rm -rf build
	@mkdir -p build
	@cd build; cmake ..

clean:
	make -C build clean
	rm -rf cache imgui.ini

distclean:
	rm -rf build cache imgui.ini
