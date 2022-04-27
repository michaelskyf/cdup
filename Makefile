all:
	mkdir build; cd build && cmake .. && make && cd ..

run:
	./build/cdup

clean:
	rm -rf build
