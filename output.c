#include "output.h"
#include <math.h>
// печатает имя в кавычках, кавычка внутри имени удваивается
static void print_name(FILE *out, const char *name)
{
    fputc('"', out);
    for (size_t i = 0; name[i] != '\0'; i++) {
        if (name[i] == '"')
            fputc('"', out);
        fputc(name[i], out);
    }
    fputc('"', out);
}
// вещественное число с 6 знаками после точки
static void print_real(FILE *out, long double v, int sci)
{
    if (v == 0.0L)
        v = fabsl(v); // чтобы не печаталось -0.000000
    if (sci)
        fprintf(out, "%.6Le", v);
    else
        fprintf(out, "%.6Lf", v);
}
int print_schema(FILE *out, const struct table *t)
{
    fprintf(out, "rows %lld\n", t->rows);
    fprintf(out, "columns %zu\n", t->ncols);
    for (size_t i = 0; i < t->ncols; i++) {
        fprintf(out, "%zu ", i);
        print_name(out, t->cols[i].name);
        fprintf(out, " %s\n", type_name(t->cols[i].type));
    }
    return ferror(out) ? -1 : 0;
}
int print_stats(FILE *out, const struct table *t, int sci)
{
    fprintf(out, "rows %lld\n", t->rows);
    fprintf(out, "columns %zu\n", t->ncols);
    for (size_t i = 0; i < t->ncols; i++) {
        const struct column *c = &t->cols[i];
        fprintf(out, "column %zu\n", i);
        fprintf(out, "name ");
        print_name(out, c->name);
        fputc('\n', out);
        fprintf(out, "type %s\n", type_name(c->type));
        if (c->type == TYPE_NULL) {
            // у NULL-столбца других статистик нет
            fprintf(out, "rows %lld\n", t->rows);
            continue;
        }
        fprintf(out, "nulls %lld\n", c->nulls);
        if (c->type == TYPE_INT) {
            long double mean = (long double)c->isum / (long double)c->icnt;
            fprintf(out, "min %lld\n", c->imin);
            fprintf(out, "max %lld\n", c->imax);
            fprintf(out, "mean ");
            print_real(out, mean, sci);
            fputc('\n', out);
            fprintf(out, "positive %lld\n", c->ipos);
            fprintf(out, "negative %lld\n", c->ineg);
            fprintf(out, "even %lld\n", c->ieven);
        } else if (c->type == TYPE_DOUBLE) {
            long double mean = c->dsum / (long double)c->dcnt;
            fprintf(out, "min ");
            print_real(out, c->dmin, sci);
            fputc('\n', out);
            fprintf(out, "max ");
            print_real(out, c->dmax, sci);
            fputc('\n', out);
            fprintf(out, "mean ");
            print_real(out, mean, sci);
            fputc('\n', out);
            fprintf(out, "positive %lld\n", c->dpos);
            fprintf(out, "negative %lld\n", c->dneg);
            fprintf(out, "integer %lld\n", c->dint);
        } else { // STRING
            long double mean = c->slensum / (long double)c->scnt;
            fprintf(out, "min_length %lld\n", c->smin);
            fprintf(out, "max_length %lld\n", c->smax);
            fprintf(out, "mean_length ");
            print_real(out, mean, sci);
            fputc('\n', out);
            fprintf(out, "empty %lld\n", c->sempty);
        }
    }
    return ferror(out) ? -1 : 0;
}
