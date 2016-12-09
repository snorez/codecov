all: codecov.so

codecov.so: codecov.c codecov.h
	gcc -fPIC -rdynamic -shared codecov.c -o libcodecov.so

clean:
	rm -rf *.so
	rm -rf core
