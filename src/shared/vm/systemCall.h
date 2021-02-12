#pragma once

#include "../../common.h"
#include "shared/vm/Value.h"
#include "shared/vm/Object.h"
#include "shared/vm/constants.h"

namespace vm {

enum class SystemCall {
    kAvailableLocales,  // AvailableLocales() as List of String
    kCharacters1,       // Characters(input as String) as List of String
    kCharacters2,       // Characters(input as String, locale as String) as List of String
    kChr,               // Chr(input as Number) as String
};

class SystemCallInput {
   public:
    const std::array<Value, kValueStackSize>& valueStack;
    const std::array<boost::local_shared_ptr<Object>, kObjectStackSize>& objectStack;
    int valueStackIndex;
    int objectStackIndex;
    SystemCallInput(
        const std::array<Value, kValueStackSize>& valueStack,
        const std::array<boost::local_shared_ptr<Object>, kObjectStackSize>& objectStack,
        int valueStackIndex,
        int objectStackIndex);
};

class SystemCallResult {
   public:
    Value a = {};
    boost::local_shared_ptr<Object> x = nullptr;
    int popValues = 0;
    int popObjects = 0;
    bool hasError = false;
    std::string errorMessage = "";
    int errorCode = 0;
};

SystemCallResult systemCall(SystemCall which, const SystemCallInput& input);

}  // namespace vm