FLAGS = -g -c

all: travelMonitorClient monitorServer

build/%.o: src/%.c
	gcc $(FLAGS) $< -o $@

travelMonitorClient: build/travelMonitorClient.o  
	gcc -o $@ $^

monitorServer: build/monitorServer.o
	gcc -o $@ $^ 

clean_log:
	rm -rf log_file.*

clean:
	rm -rf build/* travelMonitorClient monitorServer