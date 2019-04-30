lib=allocator.so

# Set the following to '0' to disable log messages:
DEBUG ?= 0

CFLAGS += -Wall -g -pthread -fPIC -shared
LDFLAGS +=

$(lib): allocator.c
	$(CC) $(CFLAGS) $(LDFLAGS) -DDEBUG=$(DEBUG) allocator.c -o $@

clean:
	rm -f $(lib) $(obj)


# Tests --

test: DEBUG=0
test: clean $(lib) ./tests/run_tests
	./tests/run_tests $(run)

testupdate: testclean test

./tests/run_tests:
	git submodule update --init --remote

testclean:
	rm -rf tests
