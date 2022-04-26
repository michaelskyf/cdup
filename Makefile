all:
	mkdir build; cd build && cmake .. && make && cd ..

run:
	./build/gltest

clean:
	rm -rf build
