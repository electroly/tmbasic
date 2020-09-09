#pragma once

#include "Object.h"
#include <boost/smart_ptr/local_shared_ptr.hpp>
#include <vector>

namespace vm {

class ObjectList : public Object {
   public:
    const std::vector<boost::local_shared_ptr<Object>> items;
    virtual Kind getKind() const;
    virtual size_t getHash() const;
    virtual bool equals(const Object& other) const;
};

}  // namespace vm
