#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv.h"
#include "table.h"
#include "output.h"
static void help(void)
{
    printf("csvstat - анализ csv-таблиц\n\n");
    printf("csvstat --help\n");
    printf("csvstat schema <файл> [опции]\n");
    printf("csvstat stats <файл> [опции]\n");
    printf("csvstat report <файл> <выходной файл> [опции]\n\n");
    printf("Опции:\n");
    printf("  -delimiter <символ>  чем разделяются поля (по умолчанию ,)\n");
    printf("  -scientific          дробные числа в научной записи\n");
}
// читает файл и набивает статистики в t
// печатает ошибку сама и возвращает 1 если что-то не так
static int read_table(const char *path, struct table *t, char delim)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "csvstat: не могу открыть файл %s\n", path);
        return 1;
    }
    size_t len = 0, nf = 0;
    int err = 0;
    int code = 0;
    char *line = NULL;
    char **fields = NULL;
    // первая строка - заголовок
    line = csv_read_line(f, &len, &err);
    if (err) { fprintf(stderr, "csvstat: ошибка чтения файла\n"); code = 1; }
    else if (line == NULL) { fprintf(stderr, "csvstat: файл пустой\n"); code = 1; }
    if (code == 0) {
        fields = csv_parse_fields(line, delim, &nf, &err);
        free(line);
        line = NULL;
        if (err == 1) fprintf(stderr, "csvstat: не хватило памяти\n");
        else if (err == 2) fprintf(stderr, "csvstat: битый заголовок\n");
        if (err) code = 1;
    }
    if (code == 0) {
        int h = table_set_header(t, fields, nf);
        csv_free_fields(fields, nf);
        fields = NULL;
        if (h == -1) { fprintf(stderr, "csvstat: не хватило памяти\n"); code = 1; }
        else if (h == 1) { fprintf(stderr, "csvstat: пустое или повторяющееся имя столбца\n"); code = 1; }
    }
    // дальше строки данных
    while (code == 0) {
        line = csv_read_line(f, &len, &err);
        if (err) { fprintf(stderr, "csvstat: ошибка чтения файла\n"); code = 1; break; }
        if (line == NULL)
            break; // файл кончился
        // физически пустые строки перед концом файла пропускаем,
        // а пустая строка в середине - это запись из одного пустого поля
        if (len == 0) {
            char *next = csv_read_line(f, &len, &err);
            if (err) { fprintf(stderr, "csvstat: ошибка чтения файла\n"); code = 1; break; }
            if (next == NULL) { free(line); line = NULL; break; }
            char *row[1];
            row[0] = NULL;
            if (table_add_row(t, row, 1) != 0) {
                fprintf(stderr, "csvstat: пустая строка в данных (не совпадает число полей)\n");
                code = 1;
                break;
            }
            free(line);
            line = next;
        }
        fields = csv_parse_fields(line, delim, &nf, &err);
        free(line);
        line = NULL;
        if (err == 1) { fprintf(stderr, "csvstat: не хватило памяти\n"); code = 1; break; }
        if (err == 2) { fprintf(stderr, "csvstat: некорректная строка данных\n"); code = 1; break; }
        if (table_add_row(t, fields, nf) != 0) {
            fprintf(stderr, "csvstat: в строке не то число полей\n");
            code = 1;
            break;
        }
        csv_free_fields(fields, nf);
        fields = NULL;
    }
    free(line);
    if (fields != NULL)
        csv_free_fields(fields, nf);
    fclose(f);
    return code;
}
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "csvstat: нет аргументов, запустите с --help\n");
        return 2;
    }
    if (strcmp(argv[1], "--help") == 0) {
        if (argc != 2) {
            fprintf(stderr, "csvstat: у --help не должно быть аргументов\n");
            return 2;
        }
        help();
        return 0;
    }
    // 0 - schema, 1 - stats, 2 - report
    int mode = -1;
    if (strcmp(argv[1], "schema") == 0) mode = 0;
    if (strcmp(argv[1], "stats") == 0) mode = 1;
    if (strcmp(argv[1], "report") == 0) mode = 2;
    if (mode == -1) {
        fprintf(stderr, "csvstat: неизвестный режим %s\n", argv[1]);
        return 2;
    }
    // у report два обязательных аргумента, у остальных один
    int opt_start = (mode == 2) ? 4 : 3;
    if (argc < opt_start) {
        fprintf(stderr, "csvstat: не хватает обязательных аргументов\n");
        return 2;
    }
    const char *inname = argv[2];
    const char *outname = (mode == 2) ? argv[3] : NULL;
    // повтор одной и той же - ошибка
    char delim = ',';
    int sci = 0;
    int have_delim = 0;
    for (int i = opt_start; i < argc; i++) {
        if (strcmp(argv[i], "-delimiter") == 0) {
            if (have_delim) {
                fprintf(stderr, "csvstat: -delimiter указан два раза\n");
                return 2;
            }
            if (i + 1 >= argc) {
                fprintf(stderr, "csvstat: после -delimiter нет символа\n");
                return 2;
            }
            if (strlen(argv[i + 1]) != 1) {
                fprintf(stderr, "csvstat: разделитель должен быть один символ\n");
                return 2;
            }
            delim = argv[i + 1][0];
            have_delim = 1;
            i++;
        } else if (strcmp(argv[i], "-scientific") == 0) {
            if (sci) {
                fprintf(stderr, "csvstat: -scientific указан два раза\n");
                return 2;
            }
            sci = 1;
        } else {
            fprintf(stderr, "csvstat: неизвестный параметр %s\n", argv[i]);
            return 2;
        }
    }
    struct table t;
    table_init(&t);
    if (read_table(inname, &t, delim) != 0) {
        table_free(&t);
        return 1;
    }
    table_finish_types(&t);
    int code = 0;
    if (mode == 2) {
        // файл открываю только когда всё посчитано
        FILE *out = fopen(outname, "w");
        if (out == NULL) {
            fprintf(stderr, "csvstat: не могу открыть файл %s для записи\n", outname);
            code = 1;
        } else {
            if (print_stats(out, &t, sci) != 0 || fclose(out) != 0) {
                fprintf(stderr, "csvstat: не удалось записать файл\n");
                remove(outname);
                code = 1;
            }
        }
    } else if (mode == 0) {
        if (print_schema(stdout, &t) != 0 || fflush(stdout) != 0) {
            fprintf(stderr, "csvstat: ошибка вывода\n");
            code = 1;
        }
    } else {
        if (print_stats(stdout, &t, sci) != 0 || fflush(stdout) != 0) {
            fprintf(stderr, "csvstat: ошибка вывода\n");
            code = 1;
        }
    }
    table_free(&t);
    return code;
}
