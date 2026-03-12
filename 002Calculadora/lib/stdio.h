#ifndef STDIO_H
#define STDIO_H

// Language Library Layer
// Minimal PRINT / READ (printf/scanf-like) without libc

// Supported format specifiers:
//   %s  string
//   %d  int
//   %f  fixed-point (2 decimals) stored as int (x100)

int PRINT(const char *fmt, ...);
int READ(const char *fmt, ...);

#endif // STDIO_H
