#include "csv.h"
#include <stdlib.h>
#include <string.h>
char *csv_read_line(FILE *f, size_t *len, int *err)
{
    *err = 0;
    size_t cap = 64;
    size_t n = 0;
    char *buf = malloc(cap);
    if (buf == NULL) {
        *err = 1;
        return NULL;
    }
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '\n')
            break;
        if (n + 2 > cap) {
            char *tmp = realloc(buf, cap * 2);
            if (tmp == NULL) {
                free(buf);
                *err = 1;
                return NULL;
            }
            buf = tmp;
            cap = cap * 2;
        }
        buf[n] = (char)c;
        n++;
    }
    if (ferror(f)) {
        free(buf);
        *err = 1;
        return NULL;
    }
    if (c == EOF && n == 0) {
        free(buf); // файл кончился, строк больше нет
        return NULL;
    }
    buf[n] = '\0';
    *len = n;
    return buf;
}
// докидывает память под массив полей, если он кончился
static void free_partial(char **fields, size_t n)
{
    for (size_t i = 0; i < n; i++)
        free(fields[i]);
    free(fields);
}
char **csv_parse_fields(const char *line, char delim, size_t *nf, int *err)
{
    *err = 0;
    size_t cap = 8;
    size_t n = 0;
    char **fields = malloc(cap * sizeof(char *));
    if (fields == NULL) {
        *err = 1;
        return NULL;
    }
    size_t i = 0;
    for (;;) {
        while (line[i] == ' ' || line[i] == '\t') // пробелы перед полем не в счёт
            i++;
            char *val = NULL;
            if (line[i] == '"') {
            // поле в кавычках, кавычки внутри удваиваются
            i++;
            size_t vcap = 16;
            size_t vn = 0;
            char *vbuf = malloc(vcap);
            if (vbuf == NULL) {
                free_partial(fields, n);
                *err = 1;
                return NULL;
            }
            int closed = 0;
            while (line[i] != '\0') {
                if (line[i] == '"') {
                    if (line[i + 1] == '"') {
                        // две кавычки = одна кавычка в значении
                        if (vn + 2 > vcap) {
                            char *tmp = realloc(vbuf, vcap * 2);
                            if (tmp == NULL) { free(vbuf); free_partial(fields, n); *err = 1; return NULL; }
                            vbuf = tmp;
                            vcap = vcap * 2;
                        }
                        vbuf[vn] = '"';
                        vn++;
                        i += 2;
                    } else {
                        i++;
                        closed = 1;
                        break;
                    }
                } else {
                    if (vn + 2 > vcap) {
                        char *tmp = realloc(vbuf, vcap * 2);
                        if (tmp == NULL) { free(vbuf); free_partial(fields, n); *err = 1; return NULL; }
                        vbuf = tmp;
                        vcap = vcap * 2;
                    }
                    vbuf[vn] = line[i];
                    vn++;
                    i++;
                }
            }
            if (!closed) {
                free(vbuf); // кавычка не закрылась
                free_partial(fields, n);
                *err = 2;
                return NULL;
            }
            // после закрывающей кавычки могут быть только пробелы до разделителя
            while (line[i] == ' ' || line[i] == '\t')
                i++;
            if (line[i] != '\0' && line[i] != delim) {
                free(vbuf);
                free_partial(fields, n);
                *err = 2;
                return NULL;
            }
            vbuf[vn] = '\0';
            val = vbuf;
        } else {
            // обычное поле до разделителя
            size_t start = i;
            while (line[i] != '\0' && line[i] != delim) {
                if (line[i] == '"') { // кавычка внутри обычного поля нельзя
                    free_partial(fields, n);
                    *err = 2;
                    return NULL;
                }
                i++;
            }
            size_t end = i;
            while (end > start && (line[end - 1] == ' ' || line[end - 1] == '\t'))
                end--; // хвостовые пробелы тоже не в счёт
            if (end > start) {
                val = malloc(end - start + 1);
                if (val == NULL) {
                    free_partial(fields, n);
                    *err = 1;
                    return NULL;
                }
                memcpy(val, line + start, end - start);
                val[end - start] = '\0';
            }
            // если val остался NULL - поле пустое
        }
        if (n >= cap) {
            char **tmp = realloc(fields, cap * 2 * sizeof(char *));
            if (tmp == NULL) {
                free(val);
                free_partial(fields, n);
                *err = 1;
                return NULL;
            }
            fields = tmp;
            cap = cap * 2;
        }
        fields[n] = val;
        n++;
        if (line[i] == '\0')
            break;
        i++; // перескакиваем разделитель
    } 
    *nf = n;
    return fields;
}
void csv_free_fields(char **fields, size_t nf)
{
    if (fields == NULL)
        return;
    for (size_t i = 0; i < nf; i++)
        free(fields[i]);
    free(fields);
}
