typedef struct {
    int size;
    char *bytes;
} __buffer_t;

__buffer_t *alloc_new_buffer(const char *buffer);
