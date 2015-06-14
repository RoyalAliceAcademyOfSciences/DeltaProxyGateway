dpGateway: dpGateway.o
	$(CC) $(LDFLAGS) dpGateway.o -o dpGateway -lnfnetlink -lnetfilter_queue
dpGateway.o: dpGateway.c
	$(CC) $(CFLAGS) -c dpGateway.c
# remove object files and executable when user executes "make clean"
clean:
	rm *.o dpGateway