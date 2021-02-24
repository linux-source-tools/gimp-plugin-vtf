all: file-vtf
	

file-vtf: vtf.h vtf.cpp gimp-plugin-vtf.cpp
	g++ -Wall -g -Wno-write-strings -o file-vtf vtf.cpp gimp-plugin-vtf.cpp `pkg-config --cflags --libs gimp-2.0 gimpui-2.0 gtk+-2.0` -Ilibsquish/include -Llibsquish/lib -lsquish -lboost_iostreams

clean:
	rm -f file-vtf
