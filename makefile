pep8unix: pep8 asem8 stripCR

pep8: pep8.cpp
	c++ -o pep8 pep8.cpp
	strip pep8
asem8: asem8.cpp
	c++ -o asem8 asem8.cpp
	strip asem8
stripCR: stripCR.cpp
	c++ -o stripCR stripCR.cpp
	strip stripCR
cleanall:
	rm pep8 asem8 stripCR
