#ifndef TABLE_H
#define TABLE_H
#include <stddef.h>
enum { TYPE_NULL, TYPE_INT, TYPE_DOUBLE, TYPE_STRING };
struct column {
    char *name;
    int type;
    long long nulls;  // сколько NULL в столбце
    long long nonnull; // сколько не-NULL значений
    // // для int 
    long long icnt, ipos, ineg, ieven;
    long long imin, imax;
    __int128 isum; // сумма в 128 битах, иначе переполняется на крайних значениях
    // // для double
    long long dcnt, dpos, dneg, dint;
    long double dsum, dmin, dmax;
    // для строк
    long long scnt, smin, smax, sempty;
    long double slensum;
};
struct table {
    struct column *cols;
    size_t ncols;
    long long rows;
};
void table_init(struct table *t);
void table_free(struct table *t);
// 0 ок, 1 битый заголовок, -1 нет памяти
int table_set_header(struct table *t, char **fields, size_t nf);
// 0 ок, 1 не совпало число полей
int table_add_row(struct table *t, char **fields, size_t nf);
void table_finish_types(struct table *t);
const char *type_name(int type);
#endif
