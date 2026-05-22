#include "CappIR.h"

VirtualReg IRFunction::allocate_vreg() {
    return VirtualReg(next_vreg_id++);
}
