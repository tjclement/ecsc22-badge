#ifndef __INTERPOLATOR_H
#define __INTERPOLATOR_H

void keygen(char *key);
void interpolarize(io_ro_32 accum0, io_ro_32 base0, io_ro_32 accum1, io_ro_32 base1);
char *hash(char *key, const char *input, char *text);

#endif