FLAGS = -g -c
MONITORS = -m 3 
SOCKBUFSIZE = -b 10
CYCLICBUFSIZE = -c 10
BFSIZE = -s 10
INPUTDIR = -i unbalanced_load
NUMTHREADS = -t 10
DEBUGOPTS = --trace-children=yes --track-origins=yes
LINK = -lm

TRAVELOBJ = build/travelMonitorCommands.o
COMMON = build/commonOps.o build/readWriteOps.o build/hashmap.o build/bucketlist.o build/requests.o build/virusRequest.o build/country.o build/citizen.o  build/virus.o build/bloomfilter.o build/setofbfs.o build/skiplist.o build/linkedlist.o build/dateOps.o build/readWriteOps.o
MONITOROBJ = build/inputParsing.o build/monitorServerCommands.o

all: travelMonitorClient monitorServer

build/%.o: src/%.c
	gcc $(FLAGS) $< -o $@

travelMonitorClient: build/travelMonitorClient.o $(COMMON) $(TRAVELOBJ)
	gcc -o $@ $^ $(LINK)

monitorServer: build/monitorServer.o $(COMMON) $(MONITOROBJ)
	gcc -o $@ $^ $(LINK)

run: 
	./travelMonitorClient $(MONITORS) $(SOCKBUFSIZE) $(CYCLICBUFSIZE) $(BFSIZE) $(INPUTDIR) $(NUMTHREADS)

run_debug:
	valgrind $(DEBUGOPTS) ./travelMonitorClient $(MONITORS) $(SOCKBUFSIZE) $(CYCLICBUFSIZE) $(BFSIZE) $(INPUTDIR) $(NUMTHREADS)

run_find_leaks:
	valgrind $(DEBUGOPTS) --leak-check=full ./travelMonitorClient $(MONITORS) $(SOCKBUFSIZE) $(CYCLICBUFSIZE) $(BFSIZE) $(INPUTDIR) $(NUMTHREADS)

clean_log:
	rm -rf log_file.*

clean:
	rm -rf build/* travelMonitorClient monitorServer