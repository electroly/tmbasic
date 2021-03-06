#pragma once

#include "../common.h"
#include "vm/Object.h"
#include "vm/RecordBuilder.h"
#include "vm/Value.h"

namespace vm {

class Record : public Object {
   public:
    const immer::array<boost::local_shared_ptr<Object>> objects;
    const immer::array<Value> values;
    Record(const Record& source, int valueIndex, Value newValue);
    Record(const Record& source, int objectIndex, const boost::local_shared_ptr<Object>& newObject);
    explicit Record(RecordBuilder* builder);
    ObjectType getObjectType() const override;
    size_t getHash() const override;
    bool equals(const Object& other) const override;
};

}  // namespace vm
