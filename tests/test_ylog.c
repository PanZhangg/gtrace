#include <stdlib.h>
#include "test_ylog.h"

int main(void)
{
    int number_failed;
    SRunner *runner = srunner_create(create_sample_suite());
    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
