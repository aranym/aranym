 /*
  * UAE - The Un*x Amiga Emulator
  * 
  * "SSSSSYYYMMMETTTTTRIICCC MMMMMMULTIIIIIII PPPPPPENGGGGUIIIIN!!!!!"
  *   -- David S. Miller
  * 
  * Symmetric Multi Penguin support - of course this also works on single
  * penguin machines, but it's kind of pointless there.
  * 
  * This is a rough, simpleminded draft - expect changes when it gets ported 
  * to other systems, and/or rewritten by someone who has experience with this
  * kind of thing. This is just to get started and to see how this works out.
  *
  * Copyright 1997 Bernd Schmidt
  */

#include <pthread.h>
#include <semaphore.h>

/* Sempahores. We use POSIX semaphores; if you are porting this to a machine
 * with different ones, make them look like POSIX semaphores. */
typedef sem_t uae_sem_t;
#define uae_sem_init sem_init
#define uae_sem_post sem_post
#define uae_sem_wait sem_wait
#define uae_sem_trywait sem_trywait
#define uae_sem_getvalue sem_getvalue

typedef union {
    int i;
    uae_u32 u32;
    void *pv;
} uae_pt;

/* These currently require the maximum size to be known at initialization
 * time, but it wouldn't be hard to use a "normal" pipe as an extension once the
 * user-level one gets full.
 * We queue up to chunks pieces of data before signalling the other thread to
 * avoid overhead. */

typedef struct {
    uae_sem_t lock;
    uae_sem_t reader_wait;
    uae_sem_t writer_wait;
    uae_pt *data;
    int size, chunks;
    volatile int rdp, wrp;
    volatile int writer_waiting;
    volatile int reader_waiting;
} smp_comm_pipe;

static __inline__ void init_comm_pipe (smp_comm_pipe *p, int size, int chunks)
{
    p->data = (uae_pt *)malloc (size*sizeof (uae_pt));
    p->size = size;
    p->chunks = chunks;
    p->rdp = p->wrp = 0;
    p->reader_waiting = 0;
    p->writer_waiting = 0;
    sem_init (&p->lock, 0, 1);
    sem_init (&p->reader_wait, 0, 0);
    sem_init (&p->writer_wait, 0, 0);
}

static __inline__ void maybe_wake_reader (smp_comm_pipe *p, int no_buffer)
{
    if (p->reader_waiting
	&& (no_buffer || ((p->wrp - p->rdp + p->size) % p->size) >= p->chunks))
    {
	p->reader_waiting = 0;
	sem_post (&p->reader_wait);
    }
}

static __inline__ void write_comm_pipe_pt (smp_comm_pipe *p, uae_pt data, int no_buffer)
{
    int nxwrp = (p->wrp + 1) % p->size;

    if (p->reader_waiting) {
	/* No need to do all the locking */
	p->data[p->wrp] = data;
	p->wrp = nxwrp;
	maybe_wake_reader (p, no_buffer);
	return;
    }
    
    sem_wait (&p->lock);
    if (nxwrp == p->rdp) {
	/* Pipe full! */
	p->writer_waiting = 1;
	sem_post (&p->lock);
	/* Note that the reader could get in between here and do a
	 * sem_post on writer_wait before we wait on it. That's harmless.
	 * There's a similar case in read_comm_pipe_int_blocking. */
	sem_wait (&p->writer_wait);
	sem_wait (&p->lock);
    }
    p->data[p->wrp] = data;
    p->wrp = nxwrp;
    maybe_wake_reader (p, no_buffer);
    sem_post (&p->lock);
}

static __inline__ uae_pt read_comm_pipe_pt_blocking (smp_comm_pipe *p)
{
    uae_pt data;

    sem_wait (&p->lock);
    if (p->rdp == p->wrp) {
	p->reader_waiting = 1;
	sem_post (&p->lock);
	sem_wait (&p->reader_wait);
	sem_wait (&p->lock);
    }
    data = p->data[p->rdp];
    p->rdp = (p->rdp + 1) % p->size;

    /* We ignore chunks here. If this is a problem, make the size bigger in the init call. */
    if (p->writer_waiting) {
	p->writer_waiting = 0;
	sem_post (&p->writer_wait);
    }
    sem_post (&p->lock);
    return data;
}

static __inline__ int comm_pipe_has_data (smp_comm_pipe *p)
{
    return p->rdp != p->wrp;
}

static __inline__ int read_comm_pipe_int_blocking (smp_comm_pipe *p)
{
    uae_pt foo = read_comm_pipe_pt_blocking (p);
    return foo.i;
}
static __inline__ uae_u32 read_comm_pipe_u32_blocking (smp_comm_pipe *p)
{
    uae_pt foo = read_comm_pipe_pt_blocking (p);
    return foo.u32;
}

static __inline__ void *read_comm_pipe_pvoid_blocking (smp_comm_pipe *p)
{
    uae_pt foo = read_comm_pipe_pt_blocking (p);
    return foo.pv;
}

static __inline__ void write_comm_pipe_int (smp_comm_pipe *p, int data, int no_buffer)
{
    uae_pt foo;
    foo.i = data;
    write_comm_pipe_pt (p, foo, no_buffer);
}

static __inline__ void write_comm_pipe_u32 (smp_comm_pipe *p, int data, int no_buffer)
{
    uae_pt foo;
    foo.u32 = data;
    write_comm_pipe_pt (p, foo, no_buffer);
}

static __inline__ void write_comm_pipe_pvoid (smp_comm_pipe *p, void *data, int no_buffer)
{
    uae_pt foo;
    foo.pv = data;
    write_comm_pipe_pt (p, foo, no_buffer);
}

typedef pthread_t penguin_id;
#define BAD_PENGUIN -1

static __inline__ int start_penguin (void *(*f) (void *), void *arg, penguin_id *foo)
{
    return pthread_create (foo, 0, f, arg);
}
#define UAE_PENGUIN_EXIT pthread_exit(0)
