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

#ifndef Y_LOG__
#define Y_LOG__

#include <stdint.h>
#include <pthread.h>

#define SHM_KEY 4550

extern struct trace_manager *g_trace_manager;

uint64_t
trace_cpu_time_now(void);

double
get_cpu_mhz(void);

/*
 * function pointer for view process to retrieve
 * content stored in the event data space
 * @arg addr
 *  addr to the data space
 * @arg content
 *  Any specific content one would like to pass
 *  out of the function, used for trigger normally
 */
typedef void (*content_retrieve)(void *addr, void *content);

/*
 * function pointer for target process to decide
 * if its trace point is triggered or not
 * @arg void
 *   pointer to trace data needed to make this
 *   decision
 * @return
 *   if triggered 1, not 0
 */
typedef int (*is_triggered_fn)(void *arg);

/* Branch prediction*/
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
 * Make sure these numbers are power of 2
 */
#define TRACE_POINT_LIST_SIZE 64
#define ENABLED_MASK_LEN ( TRACE_POINT_LIST_SIZE >> 3 )
#define CIRCULAR_BUFFER_SIZE 512

#define TRACE_POINT_REGISTER_MAP_SIZE ( TRACE_POINT_LIST_SIZE >> 8 )

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
    /* First Cache Line*/
    uint8_t is_enabled;

    /* Index of this TP in trace_point_list as well*/
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

    /* Increment per event*/
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

    /* Timestamp when first triggered*/
    uint64_t first_triggered_time;

    /*
     * The function pointer to the function
     * which is able to decode the data field
     */
    content_retrieve cr_fn;

    /* Type format of the recorded data*/
    struct trace_type type;

    /* ID of the thread executed through this TP*/
    pthread_t pthread_id;

    char name[TRACE_POINT_NAME_LEN];

    /* Location of TP,__FILE__, __LINE__, __func__*/
    char location[TRACE_POINT_LOCATION_LEN];
};

struct trace_event {
    /*ID of the TP recorded this event*/
    uint32_t trace_point_id;
    /* Absolute time stamp in CPU clock cycles. */
    uint64_t timestamp;
};

struct shared_mem_block {
    struct trace_event event;
    char data[SHARED_MEM_BLOCK_DATA_SIZE];
};


struct trace_manager {
    uint32_t trace_point_num;

    /*
     * Num of shared memory blocks left to store
     * events, 0 means no space
     * Used to stop tracing after special event
     * is triggered, make sure the event is
     * existing in the circular buffer
     */
    uint32_t event_num_left;

    /* Mask of trace_point_list to indicate enabled ones*/
    uint8_t enabled_trace_point_mask[ENABLED_MASK_LEN];


    /* List to store trace points*/
    struct trace_point trace_point_list[TRACE_POINT_LIST_SIZE];

    /* Circular buffer*/
    struct shared_mem_block buffer[TRACE_POINT_LIST_SIZE][CIRCULAR_BUFFER_SIZE]
    __attribute__((aligned(64)));
};


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
void
trace_manager_init(uint32_t key);


void
trace_manager_destroy(struct trace_manager **);


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
struct trace_manager *
trace_manager_connect(uint32_t key);


/*
 * Create a trace point
 * Register it to the trace manager
 * @arg name
 *     name of the trace point
 * @return
 *     pointer to the newly created trace point
 */
struct trace_point *
trace_point_create(const char *name);


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
void
set_trace_point(struct trace_point *tp, uint32_t track, const char *format,
                const char *file, const int line, const char *func);


void
register_content_retrieve_fn(struct trace_point *tp, content_retrieve fn);

void
register_trigger_fn(struct trace_point *tp, is_triggered_fn fn);


/*
 * Return all trace points which share the same type
 * to tps as well as its exact num
 * If none, return NULL
 */
void
find_tp_by_type(struct trace_manager *tm,
                const char *type, struct trace_point **tps, uint32_t *num);

/*
 * Return all trace points which share the same track
 * to tps as well as its exact num
 * If none, return NULL
 */
void
find_tp_by_track(struct trace_manager *tm,
                 uint32_t track, struct trace_point **tps, uint32_t *num);

/*
 * List all trace points existing
 * in the trace_manager trace_point list
 * Normally used by the view process
 * Print each point's information formatlly
 */
void
list_all_trace_point(struct trace_manager *tm);

void
list_enabled_trace_point(struct trace_manager *tm);

void
list_disabled_trace_point(struct trace_manager *tm);

/*
 * list num trace point(s) in tps
 */
void
list_point(struct trace_point **tps, uint32_t num);

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
void *
get_data_block(struct trace_point *tp);


/*
 * Get the first trace point in the
 * bucket of the tp_register_map
 * If there's no tp, return NULL
 */
struct trace_point *
get_first_tp_by_track(struct trace_manager *tm, uint32_t track);



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
 * Also use *name* to record data in the same scope, see example for
 * details
 */
#define SET_TRACE_POINT(track, format, name, ptr) \
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
        if (TP_IS_ENABLED(name)) { \
            ptr = (typeof(ptr))get_data_block(name); \
        } else { \
            goto STOP_RECORD_LABEL(name); \
        }

/*
 * Set one trace point which could be triggered by a
 * user defined trigger aka the return value of the
 * triggered is 1
 * Keep the triggered event in the middle of the circular
 * buffer
 */

#define SET_TRIGGER_TRACE_POINT(track, format, name, ptr) \
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
        if (TP_IS_ENABLED(name)) { \
            static __thread int trigger_flag = 0; \
            if (name->tr_fn(&name->shared_mem_block[name->event_seq % \
                                                    CIRCULAR_BUFFER_SIZE])) { \
                trigger_flag = 1; \
                name->first_triggered_time = trace_cpu_time_now(); \
            } \
            if (trigger_flag == 1 && name->event_left) { \
                ptr = (typeof(ptr))get_data_block(name); \
                name->event_left--; \
            } \
        } else { \
            goto STOP_RECORD_LABEL(name); \
        }

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

#endif
