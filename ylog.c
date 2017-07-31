#include "ylog.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define CACHE_LINE_BYTES 64

struct trace_manager *g_trace_manager = NULL;

uint64_t
trace_cpu_time_now(void)
{
    union {
        uint64_t v;
        struct {
            uint32_t lo;
            uint32_t hi;
        };
    } tsc;

    asm volatile ("rdtsc":"=a" (tsc.lo), "=d"(tsc.hi));

    return tsc.v;
}

double
get_cpu_mhz(void)
{
    FILE *f;
    char buf[256];
    double mhz = 0.0;

    f = fopen("/proc/cpuinfo", "r");
    if (!f) {
        fprintf(stderr, "Can not open cpuinfo file\n");
        return 0;
    }
    while (fgets(buf, sizeof(buf), f)) {
        double m;
        int rc;

        rc = sscanf(buf, "cpu MHz : %lf", &m);
        if (rc > 0) {
            if (mhz == 0.0) {
                mhz = m;
                break;
            }
        }
    }
    fclose(f);
    return mhz;
}

static struct trace_manager *
map_shm(uint32_t key)
{
    struct trace_manager *tm;

    if (key <= 0) {
        fprintf(stderr, "value of key is less than zero\n");
        return NULL;
    }
    key_t shm_key = (key_t) key;
    int shm_id = shmget(shm_key, sizeof(struct trace_manager), IPC_CREAT);

    if (shm_id == -1) {
        perror("shm create failed\n");
        return NULL;
    }
    tm = (struct trace_manager *)shmat(shm_id, NULL, 0);
    return tm;
}

void
trace_manager_init(uint32_t key)
{
    g_trace_manager = map_shm(key);
    if (g_trace_manager == NULL) {
        fprintf(stderr, "map shared memory failed\n");
        exit(0);
    }
    memset(g_trace_manager, 0, sizeof(struct trace_manager));

    /*
     * Initialize value other than 0
     */
    g_trace_manager->cpu_freq_mhz = get_cpu_mhz();
}

void
trace_manager_destroy(struct trace_manager **tm)
{
    if (tm == NULL || *tm == NULL) {
        fprintf(stderr, "trace manager is NULL\n");
        return;
    }
    if (shmdt(*tm) == -1) {
        perror("shm detach error\n");
        return;
    }
    *tm = NULL;
}

static void
tp_buffer_remap(struct trace_manager *tm)
{
    int i = 0;

    for (; i < TRACE_POINT_LIST_SIZE; i++) {
        tm->trace_point_list[i].view_buffer = tm->buffer[i];
    }
}

struct trace_manager *
trace_manager_connect(uint32_t key)
{
    struct trace_manager *tm = map_shm(key);

    if (tm == NULL) {
        fprintf(stderr, "map shared memory failed\n");
        exit(0);
    }
    tp_buffer_remap(tm);
    return tm;
}

struct trace_point *
trace_point_create(const char *name)
{
    /*
     * starts from 0
     */
    uint32_t index =
        __sync_fetch_and_add(&g_trace_manager->trace_point_num, 1);
    if (g_trace_manager->trace_point_num > TRACE_POINT_LIST_SIZE) {
        fprintf(stderr, "No more space, can not create trace point%s\n", name);
        return NULL;
    }
    struct trace_point *tp = &g_trace_manager->trace_point_list[index];

    tp->trace_point_id = index;
    tp->track_id = 0;
    tp->event_left = CIRCULAR_BUFFER_SIZE / 2;
    tp->trigger_id = 0;
    tp->cr_fn = NULL;
    tp->tr_fn = NULL;
    tp->pthread_id = pthread_self();
    tp->event_seq = 0;
    tp->target_buffer =
        (struct shared_mem_block *)&(g_trace_manager->buffer[index]);
    tp->first_triggered_time = 0;
    uint32_t name_len = strlen(name);

    if (name_len >= TRACE_POINT_NAME_LEN) {
        name_len = TRACE_POINT_NAME_LEN - 1;
    }
    strncpy(tp->name, name, name_len);
    return tp;
}

void
set_trace_point(struct trace_point *tp, uint32_t track, const char *format,
                const char *file, const int line, const char *func)
{
    tp->track_id = track;
    uint8_t format_len = strlen(format);

    if (format_len >= TRACE_TYPE_FORMAT_LEN) {
        format_len = TRACE_TYPE_FORMAT_LEN - 1;
        fprintf(stderr, "Format buffer too short!");
    }
    strncpy(tp->type.format, format, format_len);
    tp->track_id = track;
    snprintf(tp->location, sizeof(tp->location), "%s%s%d%s%s",
             file, ": ", line, ", in ", func);
}

void
register_content_retrieve_fn(struct trace_point *tp, content_retrieve fn)
{
    tp->cr_fn = fn;
}

void
register_trigger_fn(struct trace_point *tp, is_triggered_fn fn)
{
    tp->tr_fn = fn;
}

void
find_tp_by_type(struct trace_manager *tm,
                const char *type, struct trace_point **tps, uint32_t * num)
{
    if (tps == NULL || num == NULL) {
        fprintf(stderr, "invalid parameter for return\n");
        return;
    }
    int i = 0;
    uint32_t tmp_num = 0;

    for (; i < tm->trace_point_num; i++) {
        if (!strcmp((const char *)tm->trace_point_list[i].type.format, type)) {
            tps[tmp_num] = &tm->trace_point_list[i];
            tmp_num++;
        }
    }
    *num = tmp_num;
}

void
find_tp_by_track(struct trace_manager *tm,
                 uint32_t track, struct trace_point **tps, uint32_t * num)
{
    if (tps == NULL || num == NULL) {
        fprintf(stderr, "invalid parameter for return\n");
        return;
    }
    int i = 0;
    uint32_t tmp_num = 0;

    for (; i < tm->trace_point_num; i++) {
        if (track == tm->trace_point_list[i].track_id) {
            tps[tmp_num] = &tm->trace_point_list[i];
            tmp_num++;
        }
    }
    *num = tmp_num;
}

static void
print_trace_point(struct trace_point *tp, char *output)
{
    static char *status_string[2] = { "Disabled", "Enabled" };
    sprintf(output, "TP_ID: %4d\t%8s\t%6s\t%5d\t%13ld\t%s",
            tp->trace_point_id, tp->name,
            status_string[tp->is_enabled], tp->track_id,
            tp->event_seq, tp->location);
}

#define TITLE_LINES 2
#define PRINT_TRACE_POINT_TABLE_FIRST_LINE \
sprintf(output[0], "Trace Point\tName\t\tStatus\tTrack\tEvent Records\tLocation"); \
sprintf(output[1], "-------------------------------------\
-----------------------------------------"); \

void
list_all_trace_point(struct trace_manager *tm, char **output)
{
    PRINT_TRACE_POINT_TABLE_FIRST_LINE;
    int i = 0;

    for (; i < tm->trace_point_num; i++) {
        print_trace_point(&tm->trace_point_list[i], output[TITLE_LINES + i]);
    }
}

#define CHECK_ENABLE_MASK_BIT(tm, index, mod) \
    ( (tm->enabled_trace_point_mask[index] >> mod) & 1 )

static void
list_en_dis_trace_point(struct trace_manager *tm, uint8_t is_enable,
                        char **output)
{
    PRINT_TRACE_POINT_TABLE_FIRST_LINE;
    int i = 0;

    for (; i < tm->trace_point_num; i++) {
        uint32_t index = i / 8;
        uint32_t mod = i % 8;

        if (is_enable) {
            if (CHECK_ENABLE_MASK_BIT(tm, index, mod)) {
                print_trace_point(&tm->trace_point_list[i],
                                  output[i + TITLE_LINES]);
            }
        } else {
            if (!CHECK_ENABLE_MASK_BIT(tm, index, mod)) {
                print_trace_point(&tm->trace_point_list[i],
                                  output[i + TITLE_LINES]);
            }
        }
    }
}

void
list_enabled_trace_point(struct trace_manager *tm, char **output)
{
    list_en_dis_trace_point(tm, 1, output);
}

void
list_disabled_trace_point(struct trace_manager *tm, char **output)
{
    list_en_dis_trace_point(tm, 0, output);
}

void
list_point(struct trace_point **tps, uint32_t num, char **output)
{
    PRINT_TRACE_POINT_TABLE_FIRST_LINE;
    if (tps == NULL) {
        sprintf(output[3], "invalid parameter, tps is NULL\n");
        return;
    }
    int i = 0;

    for (; i < num; i++) {
        if (tps[i] != NULL) {
            print_trace_point(tps[i], output[i + TITLE_LINES]);
        } else {
            sprintf(output[i + TITLE_LINES], "point num is incorrect\n");
            break;
        }
    }
}

inline static struct shared_mem_block *
get_next_buffer_block(struct trace_point *tp)
{
    /*
     * event_seq will start with 1
     */
    tp->event_seq++;
    uint32_t next_index = tp->event_seq % CIRCULAR_BUFFER_SIZE;

    return (&tp->target_buffer[next_index]);
}

inline static struct shared_mem_block *
get_prev_buffer_block(struct trace_point *tp)
{
    uint64_t seq = tp->event_seq - 1;
    uint32_t prev_index = seq % CIRCULAR_BUFFER_SIZE;

    return (&tp->target_buffer[prev_index]);
}

static struct shared_mem_block *
get_last_buffer_block(struct trace_point *tp)
{
    uint64_t seq = tp->event_seq;
    uint32_t prev_index = seq % CIRCULAR_BUFFER_SIZE;

    return (&tp->target_buffer[prev_index]);
}

struct shared_mem_block **
get_life_cycle(struct trace_point *tp)
{
    /*
     * TODO
     */
    return NULL;
}

void *
get_data_block(struct trace_point *tp)
{
    struct shared_mem_block *b = get_next_buffer_block(tp);

    b->event.trace_point_id = tp->trace_point_id;
    b->event.timestamp = trace_cpu_time_now();
    return (void *)b->data;
}

void *
get_prev_data_block(struct trace_point *tp)
{
    struct shared_mem_block *b = get_prev_buffer_block(tp);

    return (void *)b->data;
}

struct trace_point *
get_first_tp_by_track(struct trace_manager *tm, uint32_t track)
{
    int i = 0;

    for (; i < tm->trace_point_num; i++) {
        if (tm->trace_point_list[i].track_id == track) {
            return &tm->trace_point_list[i];
        }
    }
    return NULL;
}

struct monitor_point *
monitor_point_create(char *name)
{
    uint32_t index =
        __sync_fetch_and_add(&g_trace_manager->monitor_point_num, 1);
    if (g_trace_manager->monitor_point_num > MP_LIST_LEN) {
        fprintf(stderr, "No more space, can not create monitor point%s\n",
                name);
        return NULL;
    }
    struct monitor_point *mp = &g_trace_manager->mp_list[index];

    return mp;
}

void
set_monitor_point(struct monitor_point *mp, const char *file, const int line,
                  const char *func, const char *name)
{
    snprintf(mp->string_buffer, sizeof(mp->string_buffer), "%s%s%s%s%s",
             file, ", ", func, ": ", name);
}

struct perf_point *
perf_point_create(char *name)
{
    uint32_t index = __sync_fetch_and_add(&g_trace_manager->perf_point_num, 1);

    if (g_trace_manager->perf_point_num > PERF_POINT_LIST_LEN) {
        fprintf(stderr, "No more space, can not create perf point%s\n", name);
        return NULL;
    }
    struct perf_point *pp = &g_trace_manager->perf_list[index];

    return pp;
}

void
set_perf_point(struct perf_point *pp, const char *file, const int line,
               const char *func, const char *name, const char *unit)
{
    snprintf(pp->string_buffer, sizeof(pp->string_buffer), "%s%s%s%s%s",
             file, ", ", func, ": ", name);
    snprintf(pp->unit, sizeof(pp->unit), "%s", unit);
    pp->trigger_times = 0;
}

uint32_t *
get_perf_point_data_block(struct perf_point *pp)
{
    struct perf_point_data *ppd =
        &pp->data[pp->trigger_times % PERF_POINT_DATA_LEN];
    pp->trigger_times++;
    ppd->timestamp = trace_cpu_time_now();
    return &ppd->count;
}
