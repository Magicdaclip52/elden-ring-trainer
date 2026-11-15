#pragma once

#include "core/Types.h"

#include <vector>

namespace er {

struct RcBlob {
    std::vector<u8> data;
    bool ok = false;
};

RcBlob loadResource(int id);

}  // namespace er
