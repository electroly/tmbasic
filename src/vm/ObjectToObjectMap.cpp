#include "ObjectToObjectMap.h"

using namespace vm;

Kind ObjectToObjectMap::getKind() const {
    return Kind::kObjectToObjectMap;
}

size_t ObjectToObjectMap::getHash() const {
    return std::hash<size_t>{}(pairs.size());
}

bool ObjectToObjectMap::equals(const Object& other) const {
    if (other.getKind() != Kind::kObjectToObjectMap) {
        return false;
    }
    auto& otherMap = (const ObjectToObjectMap&)other;
    if (pairs.size() != otherMap.pairs.size()) {
        return false;
    }
    for (auto pair : pairs) {
        auto otherPair = otherMap.pairs.find(pair.first);
        if (otherPair == otherMap.pairs.end()) {
            return false;
        } else if (!pair.second->equals(*otherPair->second)) {
            return false;
        }
    }
    return true;
}
