#include "table.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
void table_init(struct table *t)
{
    t->cols = NULL;
    t->ncols = 0;
    t->rows = 0;
}
void table_free(struct table *t)
{
    if (t->cols == NULL)
        return;
    for (size_t i = 0; i < t->ncols; i++)
        free(t->cols[i].name);
    free(t->cols);
    t->cols = NULL;
    t->ncols = 0;
}
int table_set_header(struct table *t, char **fields, size_t nf)
{
    t->cols = calloc(nf, sizeof(struct column));
    if (t->cols == NULL)
        return -1;
    t->ncols = nf;
    for (size_t i = 0; i < nf; i++) {
        char *name = fields[i];
        // имя не может быть пустым, NULL в csv значит пустое поле
        if (name == NULL || name[0] == '\0')
            return 1;
        // имена не должны повторяться 
        for (size_t j = 0; j < i; j++)
            if (strcmp(t->cols[j].name, name) == 0)
                return 1;
            size_t n = strlen(name) + 1;
        t->cols[i].name = malloc(n);
        if (t->cols[i].name == NULL)
            return -1;
        memcpy(t->cols[i].name, name, n);
    }
    return 0;
}
// проверяет что s это целое вида [+-]?цифры и влезает в long long
static int is_int_val(const char *s, long long *out)
{
    size_t i = 0;
    if (s[i] == '+' || s[i] == '-')
        i++;
    if (!isdigit((unsigned char)s[i]))
        return 0;
    while (isdigit((unsigned char)s[i]))
        i++;
    if (s[i] != '\0')
        return 0;
    errno = 0;
    long long v = strtoll(s, NULL, 10);
    if (errno == ERANGE)
        return 0; // не влезло в диапазон - значит это строка
    *out = v;
    return 1;
}
// вещественное вида [+-]?цифры.цифры (5. и .5 вещественными не считаются)
static int is_double_val(const char *s, double *out)
{
    size_t i = 0;
    if (s[i] == '+' || s[i] == '-')
        i++;
    if (!isdigit((unsigned char)s[i]))
        return 0;
    while (isdigit((unsigned char)s[i]))
        i++;
    if (s[i] != '.')
        return 0;
    i++;
    if (!isdigit((unsigned char)s[i]))
        return 0;
    while (isdigit((unsigned char)s[i]))
        i++;
    if (s[i] != '\0')
        return 0;
    double v = strtod(s, NULL);
    if (!isfinite(v))
        return 0; // слишком большое число - тоже строка
    *out = v;
    return 1;
}
// добавляет одно значение в статистики столбца (val == NULL это пустое поле)
static void add_val(struct column *c, const char *val)
{
    if (val == NULL) {
        c->nulls++;
        return;
    }
    c->nonnull++;
    // длина нужна для любых типов
    size_t len = strlen(val);
    c->scnt++;
    if (c->scnt == 1 || (long long)len < c->smin)
        c->smin = (long long)len;
    if (c->scnt == 1 || (long long)len > c->smax)
        c->smax = (long long)len;
    c->slensum += (long double)len;
    if (len == 0)
        c->sempty++;
    long long n = 0;
    int iv = is_int_val(val, &n);
    if (iv) {
        c->icnt++;
        c->isum += n;
        if (c->icnt == 1) {
            c->imin = n;
            c->imax = n;
        } else {
            if (n < c->imin) c->imin = n;
            if (n > c->imax) c->imax = n;
        }
        if (n > 0) c->ipos++;
        if (n < 0) c->ineg++;
        if (n % 2 == 0) c->ieven++;
    }
    // целое число это тоже вещественное, поэтому double
    // собираю у всего что похоже на число
    double d = 0.0;
    int is_num = iv;
    if (!is_num)
        is_num = is_double_val(val, &d);
    if (is_num && iv)
        d = (double)n;
    if (is_num) {
        c->dcnt++;
        c->dsum += (long double)d;
        if (c->dcnt == 1) {
            c->dmin = d;
            c->dmax = d;
        } else {
            if (d < c->dmin) c->dmin = d;
            if (d > c->dmax) c->dmax = d;
        }
        // -0.0 и 0.0 не считаются ни положительными ни отрицательными
        if (d > 0.0) c->dpos++;
        else if (d < 0.0) c->dneg++;
        if (d == floor(d))
            c->dint++;
    }
}
int table_add_row(struct table *t, char **fields, size_t nf)
{
    if (nf != t->ncols)
        return 1;
    t->rows++;
    for (size_t i = 0; i < nf; i++)
        add_val(&t->cols[i], fields[i]);
    return 0;
}
void table_finish_types(struct table *t)
{
    for (size_t i = 0; i < t->ncols; i++) {
        struct column *c = &t->cols[i];
        if (c->nonnull == 0)
            c->type = TYPE_NULL;
        else if (c->icnt == c->nonnull)
            c->type = TYPE_INT;
        else if (c->dcnt == c->nonnull)
            c->type = TYPE_DOUBLE;
        else
            c->type = TYPE_STRING;
    }
}
const char *type_name(int type)
{
    if (type == TYPE_NULL) return "NULL";
    if (type == TYPE_INT) return "INT";
    if (type == TYPE_DOUBLE) return "DOUBLE";
    return "STRING";
}
