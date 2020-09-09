#pragma once

#include "Kind.h"
#include <boost/smart_ptr/local_shared_ptr.hpp>
#include <cstddef>

namespace vm {

class Object {
   public:
    virtual Kind getKind() const = 0;
    virtual std::size_t getHash() const = 0;
    virtual bool equals(const Object& other) const = 0;
};

}  // namespace vm

bool operator==(const boost::local_shared_ptr<vm::Object>& lhs, const boost::local_shared_ptr<vm::Object>& rhs);

namespace std {

template <>
struct hash<boost::local_shared_ptr<vm::Object>> {
    std::size_t operator()(const boost::local_shared_ptr<vm::Object>& k) const noexcept { return k->getHash(); }
};

}  // namespace std
