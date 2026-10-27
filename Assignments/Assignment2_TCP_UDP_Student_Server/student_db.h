/* student_db.h - CSV lookup shared by TCP and UDP servers */
#ifndef STUDENT_DB_H
#define STUDENT_DB_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CSV_FILE "students.csv"
typedef struct { char roll[32], name[64], dept[64], sem[16], cgpa[16]; } Student;

static void trim(char *s) {
    s[strcspn(s, "\r\n")] = 0;
    char *p = s; while (*p == ' ' || *p == '\t') p++;
    memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s); while (n && (s[n-1] == ' ' || s[n-1] == '\t')) s[--n] = 0;
}
/* 1 = found, 0 = not found, -1 = file error. First CSV line is a header. */
static int find_student(const char *roll_in, Student *out) {
    char roll[64]; snprintf(roll, sizeof roll, "%s", roll_in); trim(roll);
    FILE *f = fopen(CSV_FILE, "r");
    if (!f) { perror("fopen students.csv"); return -1; }
    char line[256]; int first = 1, found = 0;
    while (fgets(line, sizeof line, f)) {
        if (first) { first = 0; continue; }
        line[strcspn(line, "\r\n")] = 0;
        char *a = strtok(line, ","), *b = strtok(NULL, ","), *c = strtok(NULL, ","),
             *d = strtok(NULL, ","), *e = strtok(NULL, ",");
        if (a && e && strcmp(a, roll) == 0) {
            snprintf(out->roll, sizeof out->roll, "%s", a);  snprintf(out->name, sizeof out->name, "%s", b);
            snprintf(out->dept, sizeof out->dept, "%s", c);  snprintf(out->sem, sizeof out->sem, "%s", d);
            snprintf(out->cgpa, sizeof out->cgpa, "%s", e);  found = 1; break;
        }
    }
    fclose(f); return found;
}
#endif
