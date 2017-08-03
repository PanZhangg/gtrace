CC=gcc
CFLAGS=-c -O2 -g -Wall -Wno-strict-aliasing
LDFLAGS=-lcheck -lsubunit -lrt -lpthread -lm -lmenu -lpanel -lncurses

#all: run_tests example_target ylog_view
all: example_target ylog_view


#run_tests: test_ylog.o ylog.o pktcraft.o
#$(CC) $^ -o $@ $(LDFLAGS)

example_target: example_target.o ylog.o
	$(CC) $^ -g -o $@ $(LDFLAGS)

ylog_view: ylog_view.o ylog.o
	$(CC) $^ -o $@ $(LDFLAGS)

#test_ylog.o: test_ylog.c
#$(CC) $(CFLAGS) $< -o $@

ylog.o: ../ylog.c
	$(CC) $(CFLAGS) $< -o $@

pktcraft.o: pktcraft.c
	$(CC) $(CFLAGS) $< -o $@

example_target.o :example_target.c
	$(CC) $(CFLAGS) $< -o $@

ylog_view.o: ylog_view.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o example_target ylog_view
