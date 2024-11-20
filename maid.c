#define BUFFER_IMPLEMENTATION
#include "buffer.h"
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

bool readline(FILE *f, buffer_t *buf) {
  buffer_clear(buf);

  for (;;) {
    char *ptr = &buf->bytes[buf->length];
    char *line = fgets(ptr, buf->capacity - buf->length, f);
    buf->length += strlen(ptr);
    if (!line)
      return false;
    if (buffer_get(buf, buf->length - 1) == '\n')
      return true;
    buffer_reserve(buf, 1);
  }
}

bool isBlank(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

bool isBlankLine(char *line) {
  for (; *line; line++) {
    if (!isBlank(*line))
      return false;
  }

  return true;
}

char *trimStart(char *text) {
  while (isBlank(*text))
    text++;
  return text;
}

void trimEnd(char *text, size_t length) {
  char *end = &text[length];
  while (isBlank(end[-1]))
    end--;
  *end = 0;
}

void trimWord(char *text) {
  for (; *text; text++) {
    if (isBlank(*text)) {
      *text = 0;
      return;
    }
  }
}

bool isFencedCode(char *line) {
  if (*line == '`' || *line == '~')
    return line[1] == *line && line[2] == *line;
  return false;
}

int fencedCode(char *line) {
  int fence = *line;
  if (fence == '`' || fence == '~') {
    int end = 1;
    while (*++line == fence)
      end++;
    return end;
  }
  return 0;
}

bool codeEnd(char *line, char fence, int count) {
  for (int i = 0; i < count; i++) {
    if (*line++ != fence)
      return false;
  }
  return true;
}

int headingNumber(char *line) {
  int h = 0;
  while (*line == '#')
    h++, line++;

  return h;
}

enum lang {
  LANG_SHELL,
  LANG_JAVASCRIPT,
  LANG_HASKELL,
};

typedef struct task task_t;

struct task {
  char *name, *description, *code;
  enum lang lang;
  task_t *next;
};

task_t *newTask(task_t *next, char *name) {
  task_t *task = malloc(sizeof(task_t));
  task->name = malloc(strlen(name = trimStart(name)));
  strcpy(task->name, name);
  task->description = NULL;
  task->code = NULL;
  task->lang = LANG_SHELL;
  task->next = next;

  return task;
}

void setLang(task_t *task, char *lang) {
  lang = trimStart(lang);
  trimWord(lang);

  if (strcmp(lang, "sh") == 0) {
    task->lang = LANG_SHELL;
  } else {
    fprintf(stderr, "Unknown language: %s\n", lang);
    exit(1);
  }
}

task_t *reverseTasks(task_t *head) {
  task_t *rev = NULL;
  while (head) {
    task_t *tail = head->next;
    head->next = rev;
    rev = head;
    head = tail;
  }
  return rev;
}

char *magic = "<!-- maid-tasks -->\n";

int main() {
  FILE *f = fopen("README.md", "r");

  int level = 0;
  int state = 0;
  int heading = 0;
  task_t *task = NULL;
  buffer_t buf = buffer_create(0);
  while (readline(f, &buf)) {
    char *line = buf.bytes;

    if (isBlankLine(line)) {
      continue;
    } else if ((level = headingNumber(line))) {
      if (state == 1 && level > heading) {
        state = 2;
        trimEnd(buf.bytes, buf.length);
        task = newTask(task, &line[level]);
      } else {
        state = 0;
        heading = level;
      }
    } else if ((level = fencedCode(line))) {
      char fence = *line;
      if (state == 3 || state == 2) {
        state = 1;
        buffer_t code = buffer_create(0);
        setLang(task, &line[level]);
        while (readline(f, &buf) && !codeEnd(buf.bytes, fence, level))
          buffer_extend(&code, buf.bytes, buf.length);

        task->code = code.bytes;
      } else {
        while (readline(f, &buf) && !codeEnd(buf.bytes, fence, level))
          ;
      }
    } else if (!strcmp(line, magic)) {
      state = 1;
    } else {
      if (state == 2) {
        state = 3;
        buffer_t desc = buffer_create(buf.length);
        do {
          trimEnd(buf.bytes, buf.length);
          buffer_extend(&desc, buf.bytes, buf.length);
          buffer_push(&desc, ' ');
        } while (readline(f, &buf) && !isBlankLine(buf.bytes));

        task->description = desc.bytes;
      } else {
        while (readline(f, &buf) && !isBlankLine(buf.bytes))
          ;
      }
    }
  }

  if (ferror(f)) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  task = reverseTasks(task);

  printf("Tasks available:\n");
  while (task) {
    printf("  %s: %s\n", task->name, task->description);
    task = task->next;
  }
}
