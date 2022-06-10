all:
	mkdir build; cd build && cmake .. && make && cd ..

run:
	./build/cscan

clean:
	rm -rf build
