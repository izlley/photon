#ifndef EXECGET_H
#define EXECGET_H

#include <event2/buffer.h>
#include "util-parser.h"
#include "common-macro.h"

class ExecGet {
 public:
   static int execGet(const char *aUri, struct evbuffer *aEvb);
};

#endif