OBJS := buffer.o hash.o http.o region.o rex.o
TEST_OBJS := buffer.o hash.o http.o region.o
CFLAGS := -luv -Wall -g3 -ggdb -O0
all: rex

rex: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

test_buffer: test_buffer.o $(TEST_OBJS)
	$(CC) -o $@ test_buffer.o $(TEST_OBJS) $(CFLAGS)

test_http: test_http.o $(TEST_OBJS)
	$(CC) -o $@ http.o buffer.o hash.o region.o test_http.o $(CFLAGS)

test_region: test_region.o $(TEST_OBJS)
	$(CC) -o $@ $(TEST_OBJS) test_region.o $(CFLAGS)

test_hash: test_hash.o buffer.o hash.o region.o
	$(CC) -o $@ buffer.o hash.o region.o test_hash.o $(CFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	@ rm -v $(OBJS) rex
