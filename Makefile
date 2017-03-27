CMP = /afs/nd.edu/user14/csesoft/new/bin/g++
CLASS = config
CLASS2 = data_queue
MAIN = main
EXEC = site-tester

$(EXEC): $(CLASS).o $(CLASS2).o $(MAIN).o
	$(CMP) -Wall $(CLASS).o $(CLASS2).o  $(MAIN).o -o $(EXEC) -lcurl

$(CLASS).o: $(CLASS).cpp $(CLASS).h
	$(CMP) -Wall -c $(CLASS).cpp -o $(CLASS).o

$(CLASS2).o: $(CLASS2).cpp $(CLASS2).h
	$(CMP) -Wall -c $(CLASS2).cpp -o $(CLASS2).o

$(MAIN).o: $(MAIN).cpp $(CLASS).h $(CLASS).h
	$(CMP) -Wall -c $(MAIN).cpp -o $(MAIN).o

clean:
	rm *.o
	rm $(EXEC)
