//
// Created by Ahmad Jamsari on 31/05/24.
//

#include "directives.h"

#include <stdlib.h>

INST int_directive(const char *remainingLine) {
    return strtol(remainingLine, NULL, 0);
}
