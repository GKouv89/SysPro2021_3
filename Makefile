FLAGS = -g -c
MONITORS = -m 3 
SOCKBUFSIZE = -b 10
CYCLICBUFSIZE = -c 10
BFSIZE = -s 10
INPUTDIR = -i test_dir_smol
NUMTHREADS = -t 10

all: travelMonitorClient monitorServer

build/%.o: src/%.c
	gcc $(FLAGS) $< -o $@

travelMonitorClient: build/travelMonitorClient.o  
	gcc -o $@ $^

monitorServer: build/monitorServer.o
	gcc -o $@ $^ 

run: 
	./travelMonitorClient $(MONITORS) $(SOCKBUFSIZE) $(CYCLICBUFSIZE) $(BFSIZE) $(INPUTDIR) $(NUMTHREADS)

clean_log:
	rm -rf log_file.*

clean:
	rm -rf build/* travelMonitorClient monitorServer