/*
 * Author Name: Zhang Pan<dazhangpan@gmail.com>
 * Finish Date:
 *
 * Generic purpose trace module
 * High performance and flexible
 * Used in debugging phase and
 * operation and maintence monitoring
 * after product deployment
 */

#ifndef __Y_LOG__H__
#define __Y_LOG__H__

#include <stdint.h>
#include <pthread.h>

#define SHM_KEY 4552

/*===========================
 * gtrace viewer global variables
 *==========================*/

#define OUTPUT_BUFFER_MAX_SIZE 1024
char g_output_buffer[OUTPUT_BUFFER_MAX_SIZE];

extern struct trace_manager *g_trace_manager;

uint64_t trace_cpu_time_now(void);

double get_cpu_mhz(void);

/*
 * function pointer for view process to retrieve
 * content stored in the event data space
 * @arg addr
 *  addr to the data space
 * @arg content
 *  Any specific content one would like to pass
 *  out of the function, used for trigger normally
 * @arg string_output
 *  User customlized string to generate human
 *  readable content by ylog_view
 */
typedef void (*content_retrieve) (void *addr, void *content);

/*
 * function pointer for target process to decide
 * if its trace point is triggered or not
 * @arg void
 *   pointer to trace data needed to make this
 *   decision
 * @return
 *   if triggered 1, not 0
 */
typedef int (*is_triggered_fn) (void *arg);

/*
 * Branch prediction
 */
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif

/*
 * unit for SIZE and LEN: bytes
 */
#define TRACE_POINT_NAME_LEN 24
#define TRACE_POINT_LOCATION_LEN 32
#define TRACE_TYPE_FORMAT_LEN 32

/*
 * Together with the 8 bytes data makes 64 bytes
 */
#define MONITOR_POINT_STRING_BUFFER_LEN 56
#define MP_LIST_LEN 256

/*
 * 50 + tigger_times(8) + unit(8) = 64 bytes
 */
#define PERF_POINT_STRING_BUFFER_LEN 48
#define PERF_POINT_DATA_LEN 128
#define PERF_POINT_LIST_LEN 256

/*
 * Make sure these numbers are power of 2
 */
#define TRACE_POINT_LIST_SIZE 64
#define ENABLED_MASK_LEN ( TRACE_POINT_LIST_SIZE >> 3 )
#define CIRCULAR_BUFFER_SIZE 512

#define TRACE_POINT_REGISTER_MAP_SIZE ( TRACE_POINT_LIST_SIZE >> 8 )

#define MAX_TRACK_NUM 32
#define MAX_TRACK_DIGITS 4
#define TITLE_LINES 2

/*
 * This num is selected for fitting each shared_mem_block into one cache line
 * When user wants more space to store the trace data, make sure the size
 * of shared_mem_block is a multiply of 64
 *
 * TODO: Adjust this num automatically
 */
#define SHARED_MEM_BLOCK_DATA_SIZE 48

struct trace_type {
    char format[TRACE_TYPE_FORMAT_LEN];
};

struct trace_point {
    /*
     * First Cache Line
     */
    uint8_t is_enabled;

    /*
     * Index of this TP in trace_point_list as well
     */
    uint32_t trace_point_id;

    /*
     * Track_id is an identifier defined by user
     * to orgnize data
     * The data recorded from trace points which
     * share the same track_id could be orgnized
     * and displayed together
     */
    uint32_t track_id;

    /*
     * Trigger_id is an identifier to orgnize
     * trace points
     * The trace points share the same trigger_id
     * could be actived together by one trigger
     * event defined by the user
     */
    uint32_t trigger_id;

    /*
     * Increment per event
     */
    uint64_t event_seq;
    /*
     * addr of the circular buffer belonging to this tp
     * in two different processes resepectively
     */
    struct shared_mem_block *target_buffer;
    struct shared_mem_block *view_buffer;

    /*
     * function pointer to the func which decides if this
     * trace point is triggered or not
     */
    is_triggered_fn tr_fn;

    /*
     * Event num left to store in the circular buffer
     * Used when triggered, make sure the triggered
     * event as well as its preorder and successor are
     * in the buffer
     * if it's 0, stop record
     */
    uint32_t event_left;

    /*
     * Timestamp when first triggered
     */
    uint64_t first_triggered_time;

    /*
     * The function pointer to the function
     * which is able to decode the data field
     */
    content_retrieve cr_fn;

    /*
     * Type format of the recorded data
     */
    struct trace_type type;

    /*
     * ID of the thread executed through this TP
     */
    pthread_t pthread_id;

    char name[TRACE_POINT_NAME_LEN];

    /*
     * Location of TP,__FILE__, __LINE__, __func__
     */
    char location[TRACE_POINT_LOCATION_LEN];
    /*
     * Increment per read has to be apart with seq_event due to elimination of
     * false-sharing
     */
    uint64_t read_seq;
};

struct trace_event {
    /*
     * ID of the TP recorded this event
     */
    uint32_t trace_point_id;
    /*
     * Absolute time stamp in CPU clock cycles.
     */
    uint64_t timestamp;
};

struct shared_mem_block {
    struct trace_event event;
    char data[SHARED_MEM_BLOCK_DATA_SIZE];
};

/*
 * Type of monitor point
 * Can not "trace" the history of a
 * value, only the current value is
 * provided. The size of value being
 * monitored is maximum 8 bytes
 */
struct monitor_point {
    int64_t data;
    char string_buffer[MONITOR_POINT_STRING_BUFFER_LEN];
};

struct perf_point_data {
    uint32_t count;
    uint64_t timestamp;
};

struct perf_point {
    uint64_t trigger_times;
    char string_buffer[PERF_POINT_STRING_BUFFER_LEN];
    char unit[8];
    struct perf_point_data data[PERF_POINT_DATA_LEN];
};

struct trace_manager {
    uint32_t trace_point_num;
    uint32_t monitor_point_num;
    uint32_t perf_point_num;

    /*
     * Mask of trace_point_list to indicate enabled ones
     */
    uint8_t enabled_trace_point_mask[ENABLED_MASK_LEN];

    double cpu_freq_mhz;

    /*
     * List to store trace points
     */
    struct trace_point trace_point_list[TRACE_POINT_LIST_SIZE];

    /*
     * Circular buffer
     */
    struct shared_mem_block buffer[TRACE_POINT_LIST_SIZE][CIRCULAR_BUFFER_SIZE]
        __attribute__ ((aligned(64)));

    /*
     * Monitor points buffer
     */
    struct monitor_point mp_list[MP_LIST_LEN]
        __attribute__ ((aligned(64)));

    /*
     * Perf points buffer
     */
    struct perf_point perf_list[PERF_POINT_LIST_LEN]
        __attribute__ ((aligned(64)));

} __attribute__ ((aligned(64)));

/*
 * Init trace_manager including the circular
 * buffer in shared memory
 * Used in the process to be traced(target process)
 *
 * @arg key
 *     key to specify shared memory
 * @return
 *     void
 *     Note:
 *     There should be only one trace manager which
 *     is g_trace_manager
 */
void trace_manager_init(uint32_t key);

void trace_manager_destroy(struct trace_manager **);

/*
 * Connect to the shared memory created by
 * target process
 * Used in the view process
 *
 * @arg key
 *     key to specify shared memory
 * @return
 *     Pointer to the trace_manager existing in
 *     the shared memory aka g_trace_manager
 */
struct trace_manager *trace_manager_connect(uint32_t key);

/*
 * Create a trace point
 * Register it to the trace manager
 * @arg name
 *     name of the trace point
 * @return
 *     pointer to the newly created trace point
 */
struct trace_point *trace_point_create(const char *name);

/*
 * set trace point in interested location
 * of the traced process
 * @arg track
 *    Track ID sepcified by user
 * @arg format
 *    const string to define the format of
 *    the recored data
 * @arg tp
 *    pointer to the trace point to be set
 * @arg file, func, line
 *    Location of this tp
 * @return
 *    void
 */

void set_trace_point(struct trace_point *tp, uint32_t track,
                     const char *format, const char *file, const int line,
                     const char *func);

void register_content_retrieve_fn(struct trace_point *tp, content_retrieve fn);

void register_trigger_fn(struct trace_point *tp, is_triggered_fn fn);

/*
 * Return all tracks contains in all trace points
 *
 * Result will be stored in the tracks array
 * Number of tracks will be stored in num
 */
void find_all_tracks(struct trace_manager *tm,
                     uint32_t * array, uint32_t * num);

/*
 * Return all trace points which share the same type
 * to tps as well as its exact num
 * If none, return NULL
 */
void find_tp_by_type(struct trace_manager *tm,
                     const char *type, struct trace_point **tps,
                     uint32_t * num);

/*
 * Return all trace points which share the same track
 * to tps as well as its exact num
 * If none, return NULL
 */
void find_tp_by_track(struct trace_manager *tm,
                      uint32_t track, struct trace_point **tps,
                      uint32_t * num);

/*
 * Return all trace points existing
 * to tps as well as its exact num
 * If none, return NULL
 */
void find_all_tps(struct trace_manager *tm,
                  struct trace_point **tps, uint32_t * num);

/*
 * List all trace points existing
 * in the trace_manager trace_point list
 * Normally used by the view process
 * Print each point's information formatlly
 */
void list_all_trace_point(struct trace_manager *tm, char **output);

void list_enabled_trace_point(struct trace_manager *tm, char **output);

void list_disabled_trace_point(struct trace_manager *tm, char **output);

/*
 * list num trace point(s) in tps
 */
void list_point(struct trace_point **tps, uint32_t num, char **output);

/*
 * Get mem block address of a trace point
 * in order to record data
 * @arg tp
 *    pointer to the trace point which needs the
 *    memory space to record data
 * @ return
 *    pointer to the memory start addr of the mem
 *    block which can be converted to any
 *    user appreciated data types
 */
void *get_data_block(struct trace_point *tp);

void *get_prev_data_block(struct trace_point *tp);

/*
 * Get the first trace point in the
 * bucket of the tp_register_map
 * If there's no tp, return NULL
 */
struct trace_point *get_first_tp_by_track(struct trace_manager *tm,
                                          uint32_t track);

/*
 * Get the life cycle of an object stores in
 * one trace point and its track
 */
struct shared_mem_block **get_life_cycle(struct trace_point *tp);

struct monitor_point *monitor_point_create(char *name);

void set_monitor_point(struct monitor_point *mp, const char *file,
                       const int line, const char *func, const char *name);

struct perf_point *perf_point_create(char *name);

void set_perf_point(struct perf_point *pp, const char *file, const int line,
                    const char *func, const char *name, const char *unit);

uint32_t *get_perf_point_data_block(struct perf_point *pp);

#define TP_IS_ENABLED(tp) ( tp->is_enabled )

#define ENABLE_TP(tp) \
        do { \
            tp->is_enabled = 1; \
            g_trace_manager->enabled_trace_point_mask[tp->trace_point_id / 8] |= \
            (1 << tp->trace_point_id % 8); \
        } while(0)

#define DISABLE_TP(tp) \
        do { \
            tp->is_enabled = 0; \
            g_trace_manager->enabled_trace_point_mask[tp->trace_point_id / 8] &= \
            ~(1 << tp->trace_point_id % 8); \
        } while(0)

/*
 * Set one trace point with a unique name in one unique scope of
 * the source code in the target system
 * Otherwise the compiler will complain redefination error of *name*
 * The static *name* is used to make sure one trace point is setup
 * only once by per thread
 * Additional arguments are for user record function(urf) use
 * Also use *name* to record data in the same scope, see example for
 * details
 */
#define SET_TRACE_POINT(track, format, name, type, urf, ...) \
        static __thread struct trace_point *name = NULL; \
        if (unlikely(name == NULL)) { \
            name = trace_point_create(#name); \
            if (name == NULL) { \
                goto STOP_RECORD_LABEL(name); \
            } \
            set_trace_point(name, track, format, \
                            __FILE__, __LINE__, __func__); \
            ENABLE_TP(name); \
        } \
        type *ptr = NULL; \
        if (TP_IS_ENABLED(name)) { \
            ptr = (type *)get_data_block(name); \
        } else { \
            goto STOP_RECORD_LABEL(name); \
        } \
        urf(ptr, ##__VA_ARGS__); \
        stop_record_##name:

/*
 * Set one trace point which could be triggered by a
 * user defined trigger aka the return value of the
 * tr_fn is 1
 * Keep the triggered event in the middle of the circular
 * buffer
 */
#define SET_TRIGGER_TRACE_POINT(track, format, name, type, usr_tr_fn, urf, ...) \
        static __thread struct trace_point *name = NULL; \
        if (unlikely(name == NULL)) { \
            name = trace_point_create(#name); \
            if (name == NULL) { \
                goto STOP_RECORD_LABEL(name); \
            } \
            set_trace_point(name, track, format, \
                            __FILE__, __LINE__, __func__); \
            name->tr_fn = usr_tr_fn; \
            ENABLE_TP(name); \
        } \
        type *ptr = NULL; \
        if (TP_IS_ENABLED(name)) { \
            static __thread int trigger_flag = 0; \
            if (trigger_flag == 0) { \
                ptr = (type *)get_data_block(name); \
            } else { \
                if (name->event_left > 0) { \
                    name->event_left--; \
                    ptr = (type *)get_data_block(name); \
                } else { \
                    ptr = (type *)&name->target_buffer[0].data; \
                } \
            } \
            if (trigger_flag == 0 && name->tr_fn(get_prev_data_block(name))) { \
                trigger_flag = 1; \
                name->first_triggered_time = trace_cpu_time_now(); \
            } \
        } else { \
            goto STOP_RECORD_LABEL(name); \
        } \
        urf(ptr, ##__VA_ARGS__); \
        stop_record_##name:

/*
 * Every SET_TRACE_POINT macro should followed by a STOP_RECORD macro
 * See example for details
 */

#define STOP_RECORD_LABEL(name) \
stop_record_##name

#define STOP_RECORD(name) \
stop_record_##name: ;

/*
 * Register content retrieve function to all trace point(s)
 * share the same "type". Typically it is the name of the
 * user defined data struct recored by one or multiple
 * trace point(s)
 * See example in unit test for details
 */
#define REGISTER_CONTENT_RETRIEVE_FN_FOR_TYPE(tm, type, fn) \
    do { \
        uint32_t num; \
        struct trace_point *tps[TRACE_POINT_LIST_SIZE]; \
        find_tp_by_type(tm, type, tps, &num); \
        if (num != 0) { \
            int i = 0; \
            for (; i < num; i++) { \
                register_content_retrieve_fn(tps[i], fn); \
            } \
        } \
    } while(0)

/*
 * Retrieve content from a trace point's
 * index-th buffer via user defined cr_fn
 *
 * @arg content is an output argument
 * space must be pre-alloc
 */
#define RETRIEVE_TP_CONTENT(TP, buf_index, content) \
    do { \
        if (TP->cr_fn) { \
            TP->read_seq++; \
            TP->cr_fn((void *)&TP->view_buffer[buf_index].data, content); \
        } \
    } while(0)

/*
 * Set a monitor point
 *
 * This kind of points are used to store the current value of
 * an interested element
 *
 * @arg name is the value one would like to
 * monitor
 * TODO: May need RCU lock when write/read
 * data
 */
#define SET_MONITOR_POINT(name) \
        static __thread struct monitor_point *mp##name = NULL; \
        if (unlikely(mp##name == NULL)) { \
            mp##name = monitor_point_create(#name); \
            if (mp##name == NULL) { \
                goto STOP_RECORD_LABEL(name); \
            } \
            set_monitor_point(mp##name, __FILE__, __LINE__, __func__, #name); \
        } \
        *(typeof(name) *)(&mp##name->data) = name; \
        stop_record_##name:

/*
 * Set a perf point
 *
 * This kind of points are used to measure
 * the frequency of execution of interested elements
 *
 * @name: an unique name of this perf point
 * @count: amount of the interested element executed once,
 *  say, 1 or 5 packets once
 * @unit: performance unit, e.g. pps/bps etc.
 */
#define SET_PERF_POINT(name, count, unit) \
        static __thread struct perf_point *mp##name = NULL; \
        if (unlikely(mp##name == NULL)) { \
            mp##name = perf_point_create(#name); \
            if (mp##name == NULL) { \
                goto STOP_RECORD_LABEL(name); \
            } \
            set_perf_point(mp##name, __FILE__, __LINE__, __func__, #name, #unit); \
        } \
        uint32_t *_p = get_perf_point_data_block(mp##name); \
        *_p = (uint32_t)count; \
        stop_record_##name:

/*
 * Use a threshold to decrease the sampling frequency
 * Handy to eliminate heavy measurement overhead when
 * profiling high frequency executions
 */
#define SET_THRESHOLD_PERF_POINT(name, count, unit, threshold) \
        static __thread struct perf_point *mp##name = NULL; \
        if (unlikely(mp##name == NULL)) { \
            mp##name = perf_point_create(#name); \
            if (mp##name == NULL) { \
                goto STOP_RECORD_LABEL(name); \
            } \
            set_perf_point(mp##name, __FILE__, __LINE__, __func__, #name, #unit); \
        } \
        static uint32_t name##sub_total; \
        name##sub_total += count; \
        if (name##sub_total >= threshold) { \
            uint32_t *_p = get_perf_point_data_block(mp##name); \
            *_p = (uint32_t)name##sub_total; \
            name##sub_total = 0; \
        } \
        stop_record_##name:

/*
 * For users to set their own customized trace display
 * string, used in user.h
 */
#define SET_USER_CONTENT_STRING(...) \
        sprintf(g_output_buffer, ##__VA_ARGS__);

#endif
