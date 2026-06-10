#include "hybrid_print.h"

#include "example_support.h"

#include <string.h>

void hybrid_print(const char *text) {
    example_print(text);
}

void hybrid_print_pass(const char *example_name) {
    example_print("EXAMPLE_");
    example_print(example_name);
    example_print(": PASS\n");
}
