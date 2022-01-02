OBJS := buffer.o map.o http.o region.o route.o
TEST_OBJS := buffer.o map.o http.o region.o
CFLAGS := -luv -Wall -g3 -ggdb -O0
all: route

route: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

test_buffer: test_buffer.o $(TEST_OBJS)
	$(CC) -o $@ test_buffer.o $(TEST_OBJS) $(CFLAGS)

test_http: test_http.o
	$(CC) -o $@ http.o buffer.o map.o test_http.o $(CFLAGS)

test_region: test_region.o $(OBJS)
	$(CC) -o $@ $(TEST_OBJS) test_region.o $(CFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	@ rm -v $(OBJS) route
