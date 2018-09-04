all: file-vtf
	

file-vtf: dxt.h dxt.c vtf.h vtf.c gimp-plugin-vtf.c
	gcc -Wall -g -o file-vtf dxt.c vtf.c gimp-plugin-vtf.c `pkg-config --cflags --libs gimp-2.0 gimpui-2.0 gtk+-2.0`

clean:
	rm -f file-vtf
