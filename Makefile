OBJS := buffer.o map.o http.o route.o
TEST_OBJS := buffer.o test_buffer.o
CFLAGS := -luv -Wall -g -ggdb -O0
all: route

route: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

test_buffer: $(TEST_OBJS)
	$(CC) -o $@ $(TEST_OBJS) $(CFLAGS)
	./test_buffer

test_http: test_http.o $(OBJS)
	$(CC) -o $@ http.o buffer.o map.o test_http.o $(CFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	@ rm -v $(OBJS) route
