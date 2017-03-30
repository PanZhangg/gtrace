#ifndef __YLOG_VIEW__H__
#define __YLOG_VIEW__H__

#include <stdio.h>
#include <stdint.h>
#include "../ylog.h"
#include "../user.h"

/*===========================
 * gtrace viewer interface
 *==========================*/

struct trace_manager *
ylog_view_init();

#endif
