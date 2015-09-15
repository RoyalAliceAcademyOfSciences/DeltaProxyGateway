dpgateway: dpgateway.o
	$(CC) $(LDFLAGS) dpgateway.o -o dpgateway -lnetfilter_queue
dpgateway.o: dpgateway.c
	$(CC) $(CFLAGS) -c dpgateway.c
# remove object files and executable when user executes "make clean"
clean:
	rm *.o dpgateway