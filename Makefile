OBJS := string.o hash.o http.o region.o rex.o
TEST_OBJS := string.o hash.o http.o region.o
CFLAGS := -luv -Wall -g3 -ggdb -O0
all: rex

rex: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

test_string: test_string.o string.o hash.o region.o
	$(CC) -o $@ test_string.o string.o hash.o region.o $(CFLAGS)

test_http: test_http.o $(TEST_OBJS)
	$(CC) -o $@ http.o string.o hash.o region.o test_http.o $(CFLAGS)

test_region: test_region.o $(TEST_OBJS)
	$(CC) -o $@ $(TEST_OBJS) test_region.o $(CFLAGS)

test_hash: test_hash.o string.o hash.o region.o
	$(CC) -o $@ string.o hash.o region.o test_hash.o $(CFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	@ rm -v $(OBJS) rex
