

struct conduct{
    const char *name;
    size_t c;
    size_t a;
    void * buffer;
    int eof;
    int nbElement;
    pthread_mutex_t verrou;
    pthread_cond_t lectureOk;
    pthread_cond_t ecritureOk;
    int debut;
    int fin;
 
};

struct conduct *conduct_create(const char *name, size_t a, size_t c);
struct conduct *conduct_open(const char *name);
ssize_t conduct_read(struct conduct *c, void *buf, size_t count);
ssize_t conduct_write(struct conduct *c, const void *buf, size_t count);
int conduct_write_eof(struct conduct *c);
void conduct_close(struct conduct *conduct);
void conduct_destroy(struct conduct *conduct);
