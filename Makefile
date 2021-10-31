OBJS := buffer.o map.o http_request.o route.o
TEST_OBJS := buffer.o test_buffer.o
CFLAGS := -luv -Wall -g -ggdb -O0
all: route

route: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

test: $(TEST_OBJS)
	$(CC) -o $@ $(TEST_OBJS) $(CFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	@ rm -v $(OBJS) route
