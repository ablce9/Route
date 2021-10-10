OBJS := buffer.o route.o
CFLAGS := -luv -Wall -g -ggdb -O0
all: route

route: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	@ rm -v $(OBJS) route
