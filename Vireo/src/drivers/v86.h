#ifndef V86_H
#define V86_H

#include "../io/multitasking.h"
#include "DriverHandler.h"
#include "../io/memory.h"


uint32_t v86_sgoff_to_linear(uint16_t segment, uint16_t offset);


#endif