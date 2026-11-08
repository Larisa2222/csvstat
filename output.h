#ifndef OUTPUT_H
#define OUTPUT_H
#include <stdio.h>
#include "table.h"
int print_schema(FILE *out, const struct table *t);
int print_stats(FILE *out, const struct table *t, int sci);
#endif
