all:
	cd ClotProgrammingLanguage && \
	g++ -std=c++20 -o clot *.cpp && \
	./clot
