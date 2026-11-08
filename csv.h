#ifndef CSV_H
#define CSV_H
#include <stdio.h>
#include <stddef.h>
// читает одну строку файла без \n на конце
// возвращает malloc-строку или NULL если файл кончился
char *csv_read_line(FILE *f, size_t *len, int *err);
// режет строку на поля по разделителю 
// пустое поле (или из одних пробелов) - это NULL в массиве
// 0 ок, 1 нет памяти, 2 битая строка
char **csv_parse_fields(const char *line, char delim, size_t *nf, int *err);
void csv_free_fields(char **fields, size_t nf);
#endif
