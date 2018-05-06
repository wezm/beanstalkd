#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "dat.h"

/* static uint64 next_id = 1; */

/* static int cur_prime = 0; */

/* static job all_jobs_init[12289] = {0}; */
/* static job *all_jobs = all_jobs_init; */
/* static size_t all_jobs_cap = 12289; [> == primes[0] <] */
/* static size_t all_jobs_used = 0; */

/* static int hash_table_was_oom = 0; */

static void rehash(JobStore *store, bool is_upscaling);

static int
_get_job_hash_index(const JobStore *const store, uint64 job_id)
{
    return job_id % store->all_jobs_cap;
}

JobStore *
job_store_new(void)
{
    JobStore *store = calloc(1, sizeof(JobStore));
    if (!store) return twarnx("OOM"), (JobStore *) 0;

    store->next_id = 1;
    store->all_jobs = calloc(primes[store->cur_prime], sizeof(struct job)); // 12289 == primes[0]
    if (!store->all_jobs) {
        twarnx("OOM");

        free(store);
        return NULL;
    }
    store->all_jobs_cap = primes[store->cur_prime];

    return store;
}

static void
store_job(JobStore *store, job j)
{
    int index = 0;

    index = _get_job_hash_index(store, j->r.id);

    j->ht_next = store->all_jobs[index];
    store->all_jobs[index] = j;
    store->all_jobs_used++;

    /* accept a load factor of 4 */
    if (store->all_jobs_used > (store->all_jobs_cap << 2)) rehash(store, true);
}

static void
rehash(JobStore *store, bool is_upscaling)
{
    job *old = store->all_jobs;
    size_t old_cap = store->all_jobs_cap, old_used = store->all_jobs_used, i;
    int old_prime = store->cur_prime;
    int d = is_upscaling ? 1 : -1;

    if (store->cur_prime + d >= NUM_PRIMES) return;
    if (store->cur_prime + d < 0) return;
    if (is_upscaling && store->hash_table_was_oom) return;

    store->cur_prime += d;

    store->all_jobs_cap = primes[store->cur_prime];
    store->all_jobs = calloc(store->all_jobs_cap, sizeof(job));
    if (!store->all_jobs) {
        twarnx("Failed to allocate %zu new hash buckets", store->all_jobs_cap);
        store->hash_table_was_oom = true;
        store->cur_prime = old_prime;
        store->all_jobs = old;
        store->all_jobs_cap = old_cap;
        store->all_jobs_used = old_used;
        return;
    }
    store->all_jobs_used = 0;
    store->hash_table_was_oom = false;

    for (i = 0; i < old_cap; i++) {
        while (old[i]) {
            job j = old[i];
            old[i] = j->ht_next;
            j->ht_next = NULL;
            store_job(store, j);
        }
    }

    free(old);
}

job
job_find(const JobStore *const store, uint64 job_id)
{
    job jh = NULL;
    int index = _get_job_hash_index(store, job_id);

    for (jh = store->all_jobs[index]; jh && jh->r.id != job_id; jh = jh->ht_next);

    return jh;
}

job
allocate_job(int body_size)
{
    job j;

    j = malloc(sizeof(struct job) + body_size);
    if (!j) return twarnx("OOM"), (job) 0;

    memset(j, 0, sizeof(struct job));
    j->r.created_at = nanoseconds();
    j->r.body_size = body_size;
    j->next = j->prev = j; /* not in a linked list */
    return j;
}

job
make_job_with_id(JobStore *store, uint pri, int64 delay, int64 ttr,
                 int body_size, tube tube, uint64 id)
{
    job j;

    j = allocate_job(body_size);
    if (!j) return twarnx("OOM"), (job) 0;

    if (id) {
        j->r.id = id;
        if (id >= store->next_id) store->next_id = id + 1;
    } else {
        j->r.id = store->next_id++;
    }
    j->r.pri = pri;
    j->r.delay = delay;
    j->r.ttr = ttr;

    store_job(store, j);

    TUBE_ASSIGN(j->tube, tube);

    return j;
}

static void
job_hash_free(JobStore *store, job j)
{
    job *slot;

    slot = &store->all_jobs[_get_job_hash_index(store, j->r.id)];
    while (*slot && *slot != j) slot = &(*slot)->ht_next;
    if (*slot) {
        *slot = (*slot)->ht_next;
        store->all_jobs_used--;
    }

    // Downscale when the hashmap is too sparse
    if (store->all_jobs_used < (store->all_jobs_cap >> 4)) rehash(store, false);
}

void
job_free(JobStore *store, job j)
{
    if (j) {
        TUBE_ASSIGN(j->tube, NULL);
        if (j->r.state != Copy) job_hash_free(store, j);
    }

    free(j);
}

void
job_setheappos(void *j, int pos)
{
    ((job)j)->heap_index = pos;
}

int
job_pri_less(void *ax, void *bx)
{
    job a = ax, b = bx;
    if (a->r.pri < b->r.pri) return 1;
    if (a->r.pri > b->r.pri) return 0;
    return a->r.id < b->r.id;
}

int
job_delay_less(void *ax, void *bx)
{
    job a = ax, b = bx;
    if (a->r.deadline_at < b->r.deadline_at) return 1;
    if (a->r.deadline_at > b->r.deadline_at) return 0;
    return a->r.id < b->r.id;
}

job
job_copy(job j)
{
    job n;

    if (!j) return NULL;

    n = malloc(sizeof(struct job) + j->r.body_size);
    if (!n) return twarnx("OOM"), (job) 0;

    memcpy(n, j, sizeof(struct job) + j->r.body_size);
    n->next = n->prev = n; /* not in a linked list */

    n->file = NULL; /* copies do not have refcnt on the wal */

    n->tube = 0; /* Don't use memcpy for the tube, which we must refcount. */
    TUBE_ASSIGN(n->tube, j->tube);

    /* Mark this job as a copy so it can be appropriately freed later on */
    n->r.state = Copy;

    return n;
}

const char *
job_state(job j)
{
    if (j->r.state == Ready) return "ready";
    if (j->r.state == Reserved) return "reserved";
    if (j->r.state == Buried) return "buried";
    if (j->r.state == Delayed) return "delayed";
    return "invalid";
}

int
job_list_any_p(job head)
{
    return head->next != head || head->prev != head;
}

job
job_remove(job j)
{
    if (!j) return NULL;
    if (!job_list_any_p(j)) return NULL; /* not in a doubly-linked list */

    j->next->prev = j->prev;
    j->prev->next = j->next;

    j->prev = j->next = j;

    return j;
}

void
job_insert(job head, job j)
{
    if (job_list_any_p(j)) return; /* already in a linked list */

    j->prev = head->prev;
    j->next = head;
    head->prev->next = j;
    head->prev = j;
}

uint64
total_jobs(const JobStore *const store)
{
    return store->next_id - 1;
}

/* for unit tests */
size_t
get_all_jobs_used(const JobStore *const store)
{
    return store->all_jobs_used;
}
