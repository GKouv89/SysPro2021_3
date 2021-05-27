FLAGS = -g -c
MONITORS = -m 3 
SOCKBUFSIZE = -b 10
CYCLICBUFSIZE = -c 10
BFSIZE = -s 10
INPUTDIR = -i test_dir_smol
NUMTHREADS = -t 10
DEBUGOPTS = --trace-children=yes --track-origins=yes

COMMON = build/commonOps.o build/readWriteOps.o

all: travelMonitorClient monitorServer

build/%.o: src/%.c
	gcc $(FLAGS) $< -o $@

travelMonitorClient: build/travelMonitorClient.o $(COMMON) 
	gcc -o $@ $^

monitorServer: build/monitorServer.o $(COMMON)
	gcc -o $@ $^ 

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