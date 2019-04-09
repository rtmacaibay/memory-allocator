lib=allocator

# Set the following to '0' to disable log messages:
debug=1

CFLAGS += -Wall -g -pthread -fPIC -shared
LDFLAGS +=

src=allocator.c
obj=$(src:.c=.o)

$(lib).so: $(obj)
	$(CC) $(CFLAGS) $(LDFLAGS) -DDEBUG=$(debug) $(obj) -o $@

allocator.o: allocator.c

clean:
	rm -f $(lib) $(obj)


# Tests --

test: $(bin) ./tests/run_tests
	./tests/run_tests $(run)

testupdate: testclean test

./tests/run_tests:
	git submodule update --init --remote

testclean:
	rm -rf tests
