#pragma once

#include "../common.h"
#include "vm/Object.h"
#include "vm/Value.h"

namespace vm {

class RecordBuilder {
   public:
    RecordBuilder(int numValues, int numObjects);
    immer::array_transient<Value> values;
    immer::array_transient<boost::local_shared_ptr<Object>> objects;
};

}  // namespace vm
