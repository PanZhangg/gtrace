#include <check.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "../ylog.h"
#include "pktcraft.h"

START_TEST (cpu_time_test)
{
    uint64_t start, end;
    start = trace_cpu_time_now();
    end = trace_cpu_time_now();
    printf("start: %ld\n", start);
    printf("end: %ld\n", end);
    ck_assert(start < end);
}
END_TEST

START_TEST (trace_manager_init_test)
{
    ck_assert(g_trace_manager == NULL);
    trace_manager_init(7784);
    ck_assert(g_trace_manager != NULL);
    #ifdef UNIT_TEST
    ck_assert(g_trace_manager->trace_point_num == 49);
    #endif
    trace_manager_destroy(&g_trace_manager);
    ck_assert(g_trace_manager == NULL);
}
END_TEST

START_TEST (tp_create_test)
{
    trace_manager_init(7785);
    #ifdef UNIT_TEST
    g_trace_manager->trace_point_num = 0;
    #endif
    struct trace_point *tp = trace_point_create("test_tp");
    ck_assert_int_eq(tp->trace_point_id, 0);
    ck_assert(g_trace_manager->trace_point_num == 1);
    ck_assert_str_eq(tp->name, "test_tp");
    ck_assert(pthread_equal(tp->pthread_id, pthread_self()) != 0);
    ck_assert_str_eq(g_trace_manager->trace_point_list[0].name, "test_tp");
    trace_manager_destroy(&g_trace_manager);
}
END_TEST

START_TEST (tp_enable_disable_test)
{
    trace_manager_init(7786);
    #ifdef UNIT_TEST
    g_trace_manager->trace_point_num = 0;
    #endif
    struct trace_point *tp = trace_point_create("test_tp1");
    ENABLE_TP(tp);
    ck_assert(tp->is_enabled == 1);
    //printf("mask: %d\n",(int)g_trace_manager->enabled_trace_point_mask[0]);
    ck_assert(g_trace_manager->enabled_trace_point_mask[0] == 1);
    struct trace_point *tp1 = trace_point_create("test_tp2");
    ENABLE_TP(tp1);
    ck_assert(tp->is_enabled == 1);
    //printf("mask: %d\n",(int)g_trace_manager->enabled_trace_point_mask[0]);
    ck_assert(g_trace_manager->enabled_trace_point_mask[0] == 3);
    DISABLE_TP(tp);
    //printf("mask: %d\n",(int)g_trace_manager->enabled_trace_point_mask[0]);
    list_enabled_trace_point(g_trace_manager);
    ck_assert(g_trace_manager->enabled_trace_point_mask[0] == 2);
    trace_manager_destroy(&g_trace_manager);
}
END_TEST

START_TEST (tp_set_test)
{
    trace_manager_init(7787);
    #ifdef UNIT_TEST
    g_trace_manager->trace_point_num = 0;
    #endif
    //struct trace_point *tp = trace_point_create("test_tp");
    //set_trace_point(tp, 4, "A Test TP has been triggered",
                    //__FILE__, __LINE__, __func__);
    int i = 0;
    void *ptr;
    for (i = 0; i < 2; i++) { /* SET_TRACE_POINT should only executed once*/
        SET_TRACE_POINT(4, "A Test TP has been triggered", test_tp1, ptr);
        printf("location: %s\n", test_tp1->location);
        STOP_RECORD(test_tp1);
    }
    SET_TRACE_POINT(4, "A Test TP has been triggered", test_tp2, ptr);
    printf("location: %s\n", test_tp2->location);
    STOP_RECORD(test_tp2);
    ck_assert(g_trace_manager->trace_point_num == 2);
    trace_manager_destroy(&g_trace_manager);
}
END_TEST

START_TEST (get_record_mem_test)
{
    trace_manager_init(7789);
    #ifdef UNIT_TEST
    g_trace_manager->trace_point_num = 0;
    #endif
    struct user_defined_struct {
        int a;
        int b;
    } *u;
    SET_TRACE_POINT(4, "A Test TP has been triggered", test_tp, u);
    u->a = 1;
    u->b = 2;
    STOP_RECORD(test_tp);
    int read_a = (*(struct user_defined_struct *)&(test_tp->target_buffer[0].data)).a;
    int read_b = (*(struct user_defined_struct *)&(test_tp->target_buffer[0].data)).b;
    list_all_trace_point(g_trace_manager);
    ck_assert_int_eq(read_a, 1);
    ck_assert_int_eq(read_b, 2);
    trace_manager_destroy(&g_trace_manager);
}
END_TEST

struct user_defined_struct {
        int a;
        int b;
} *u;

START_TEST (track_hash_test)
{
    trace_manager_init(7790);
    #ifdef UNIT_TEST
    g_trace_manager->trace_point_num = 0;
    #endif
    int read_a;
    int read_b;
    struct trace_point *hash_tp;
    SET_TRACE_POINT(4, "A Test TP has been triggered", test_tp, u);
    u->a = 1;
    u->b = 2;
    STOP_RECORD(test_tp);
    hash_tp = get_first_tp_by_track(g_trace_manager, 4);
    read_a = (*(struct user_defined_struct *)&(hash_tp->target_buffer[0].data)).a;
    read_b = (*(struct user_defined_struct *)&(hash_tp->target_buffer[0].data)).b;
    list_all_trace_point(g_trace_manager);
    ck_assert_int_eq(read_a, 1);
    ck_assert_int_eq(read_b, 2);
    trace_manager_destroy(&g_trace_manager);
}
END_TEST

void user_defined_cr_fn(void *addr, void *content)
{
    struct user_defined_struct *u = (struct user_defined_struct *)addr;
    fprintf(stdout, "a is: %d, b is: %d\n",
            u->a, u->b);
}

START_TEST (cr_fn_test)
{
    trace_manager_init(7791);
    #ifdef UNIT_TEST
    g_trace_manager->trace_point_num = 0;
    #endif
    SET_TRACE_POINT(4, "user_defined_struct", test_tp, u);
    u->a = 1;
    u->b = 2;
    STOP_RECORD(test_tp);
    REGISTER_CONTENT_RETRIEVE_FN_FOR_TYPE(g_trace_manager, "user_defined_struct",
                                          user_defined_cr_fn);
    test_tp->cr_fn((void *)&test_tp->target_buffer[0].data, NULL);
    trace_manager_destroy(&g_trace_manager);
}
END_TEST

void *
thread_test_fn(void *arg)
{
    int i = 0;
    for (; i < 10; i++) {
        SET_TRACE_POINT(4, "user_defined_struct", test_tp, u);
        u->a = i;
        u->b = i * 2;
        STOP_RECORD(test_tp);
    }
    return NULL;
}

START_TEST (multi_thread_test)
{
    trace_manager_init(7792);
    #ifdef UNIT_TEST
    g_trace_manager->trace_point_num = 0;
    #endif

    pthread_t tid[2];

    int i = 0;
    for (; i < 2; i++) {
        if (pthread_create(&tid[i], NULL, thread_test_fn, NULL) != 0) {
            fprintf(stderr, "thread create failed\n");
            break;
         }
    }

    sleep(1); /*wait 1s for thread_test_fn to set trace point*/

    REGISTER_CONTENT_RETRIEVE_FN_FOR_TYPE(g_trace_manager, "user_defined_struct",
                                          user_defined_cr_fn);
    list_enabled_trace_point(g_trace_manager);
    struct trace_point *tps[5];
    uint32_t num;
    //find_tp_by_track(g_trace_manager , 4, tps, &num);
    find_tp_by_type(g_trace_manager , "user_defined_struct", tps, &num);
    list_point(tps, num);
    ck_assert_int_eq(num, 2);
    if (tps[0]->cr_fn != NULL) {
        tps[0]->cr_fn((void *)&tps[0]->target_buffer[1].data, NULL);
    }
    if (tps[1]->cr_fn != NULL) {
        tps[1]->cr_fn((void *)&tps[1]->target_buffer[2].data, NULL);
    }
    for (i = 0; i < 2; i++) {
        if (pthread_join(tid[i], NULL) != 0) {
            fprintf(stderr, "thread join failed\n");
            break;
        }
    }
    trace_manager_destroy(&g_trace_manager);
}
END_TEST

START_TEST (tcp_pkt_gen_test)
{
    size_t len;
    void* TCPPacket;
    void* IPPacket;
    int srcPort = 5448;
    int destPort = 80;
    int packetSize = 100;
    char *srcHost = "10.33.0.112";
    char *destHost = "10.33.0.110";
    struct in_addr sIP;
    struct in_addr dIP;
    ck_assert(addrconvert(srcHost, &sIP) == 1);
    ck_assert(addrconvert(destHost, &sIP) == 1);
    /*20 equals the len of IP hdr without options*/
    TCPPacket = gentcppackethdr(srcPort, destPort, packetSize - 20, &len);
    ck_assert(TCPPacket != NULL);
    IPPacket = genippackethdr(TCPPacket, len, sIP, dIP, IPPROTO_TCP);
    ck_assert(IPPacket != NULL);
    free(TCPPacket);
    free(IPPacket);
}
END_TEST

START_TEST (get_cpu_mhz_test)
{
    double mhz = get_cpu_mhz();
    printf("CPU Freq: %lf\n", mhz);
}
END_TEST

Suite *create_sample_suite(void)
{
    Suite *suite = suite_create("ylog suite");
    TCase *test_case = tcase_create("functional test case");
    tcase_add_test(test_case, cpu_time_test);
    tcase_add_test(test_case, trace_manager_init_test);
    tcase_add_test(test_case, tp_create_test);
    tcase_add_test(test_case, tp_enable_disable_test);
    tcase_add_test(test_case, tp_set_test);
    tcase_add_test(test_case, get_record_mem_test);
    tcase_add_test(test_case, tcp_pkt_gen_test);
    tcase_add_test(test_case, track_hash_test);
    tcase_add_test(test_case, cr_fn_test);
    tcase_add_test(test_case, multi_thread_test);
    tcase_add_test(test_case, get_cpu_mhz_test);
    suite_add_tcase(suite, test_case);
    return suite;
}
