#include "Interpreter.h"
#include "List.h"
#include "Map.h"
#include "Opcode.h"
#include "Optional.h"
#include "Record.h"
#include "String.h"
#include "date.h"
#include "systemCall.h"
#include "util/decimal.h"

namespace vm {

static uint16_t readUint16(const uint8_t* ptr) {
    uint16_t value = 0;
    memcpy(&value, ptr, sizeof(uint16_t));
    return value;
}

static uint32_t readUint32(const uint8_t* ptr) {
    uint32_t value = 0;
    memcpy(&value, ptr, sizeof(uint32_t));
    return value;
}

static int64_t readInt64(const uint8_t* ptr) {
    int64_t value = 0;
    memcpy(&value, ptr, sizeof(int64_t));
    return value;
}

static int16_t readInt16(const uint8_t* ptr) {
    int16_t value = 0;
    memcpy(&value, ptr, sizeof(int16_t));
    return value;
}

Interpreter::Interpreter(Program* program, std::istream* consoleInputStream, std::ostream* consoleOutputStream)
    : _program(program), _consoleInputStream(consoleInputStream), _consoleOutputStream(consoleOutputStream) {
    initSystemCalls();
    initDate();
}

void Interpreter::init(int procedureIndex) {
    _callStack = {};
    _recordBuilderStack = {};
    _objectListBuilderStack = {};
    _valueListBuilderStack = {};
    _procedure = _program->procedures[procedureIndex].get();
    _a = {};
    _b = {};
    _x = nullptr;
    _y = nullptr;
    _z = nullptr;
    _valueStackIndex = kValueStackSize;
    _objectStackIndex = kObjectStackSize;

    decimal::context = decimal::IEEEContext(decimal::DECIMAL128);
}

bool Interpreter::run(int maxCycles) {
    // use local variables for our registers with the hope that the compiler chooses to put them into actual cpu
    // registers and avoids writing them to memory
    const auto* procedure = _procedure;
    const auto* instructions = &procedure->instructions;
    auto instructionIndex = _instructionIndex;
    auto a = _a;
    auto b = _b;
    auto x = _x;
    auto y = _y;
    auto z = _z;
    auto vsi = _valueStackIndex;
    auto osi = _objectStackIndex;

    // if we have a fatal interpreter error (akin to a program crash) then set the error here so it can be thrown
    // after saving our state
    const char* fatalError = nullptr;

    auto more = true;   // return value indicating whether the caller needs to call run() again
    auto stop = false;  // whether to end the loop

    for (int cycle = 0; !stop && cycle < maxCycles; cycle++) {
        switch (static_cast<Opcode>(instructions->at(instructionIndex))) {
            case Opcode::kExit:
                instructionIndex++;
                more = false;
                stop = true;
                break;

            case Opcode::kLoadConstantA:
                // ABBBBBBBB; A: opcode, B: constant
                a.num = readInt64(&instructions->at(instructionIndex + 1));
                instructionIndex += /*A*/ 1 + /*B*/ 8;
                break;

            case Opcode::kLoadConstantB:
                // ABBBBBBBB; A: opcode, B: constant
                b.num = readInt64(&instructions->at(instructionIndex + 1));
                instructionIndex += /*A*/ 1 + /*B*/ 8;
                break;

            case Opcode::kLoadConstantStringX: {
                // ABBBBC...C; A: opcode,  B: string length (no NUL), C: string (no NUL)
                auto stringLength = readUint32(&instructions->at(instructionIndex + 1));
                x = boost::make_local_shared<String>(&instructions->at(instructionIndex + 5), stringLength);
                instructionIndex += /*A*/ 1 + /*B*/ 4 + /*C*/ stringLength;
                break;
            }

            case Opcode::kLoadConstantStringY: {
                // ABBBBC...C; A: opcode,  B: string length (no NUL), C: string (no NUL)
                auto stringLength = readUint32(&instructions->at(instructionIndex + 1));
                y = boost::make_local_shared<String>(&instructions->at(instructionIndex + 5), stringLength);
                instructionIndex += /*A*/ 1 + /*B*/ 4 + /*C*/ stringLength;
                break;
            }

            case Opcode::kLoadConstantStringZ: {
                // ABBBBC...C; A: opcode,  B: string length (no NUL), C: string (no NUL)
                auto stringLength = readUint32(&instructions->at(instructionIndex + 1));
                z = boost::make_local_shared<String>(&instructions->at(instructionIndex + 5), stringLength);
                instructionIndex += /*A*/ 1 + /*B*/ 4 + /*C*/ stringLength;
                break;
            }

            case Opcode::kStoreA: {
                // ABB; A: opcode, B: index
                auto index = readInt16(&instructions->at(instructionIndex + 1));
                _valueStack.at(vsi + index) = a;
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kStoreB: {
                // ABB; A: opcode, B: index
                auto index = readInt16(&instructions->at(instructionIndex + 1));
                _valueStack.at(vsi + index) = b;
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kStoreX: {
                // ABB; A: opcode, B: index
                auto index = readInt16(&instructions->at(instructionIndex + 1));
                _objectStack.at(osi + index) = x;
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kStoreY: {
                // ABB; A: opcode, B: index
                auto index = readInt16(&instructions->at(instructionIndex + 1));
                _objectStack.at(osi + index) = y;
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kStoreZ: {
                // ABB; A: opcode, B: index
                auto index = readInt16(&instructions->at(instructionIndex + 1));
                _objectStack.at(osi + index) = z;
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kLoadA: {
                // ABB; A: opcode, B: index
                auto index = readInt16(&instructions->at(instructionIndex + 1));
                a = _valueStack.at(vsi + index);
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kLoadB: {
                // ABB; A: opcode, B: index
                auto index = readInt16(&instructions->at(instructionIndex + 1));
                b = _valueStack.at(vsi + index);
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kLoadX: {
                // ABB; A: opcode, B: index
                auto index = readInt16(&instructions->at(instructionIndex + 1));
                x = _objectStack.at(osi + index);
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kLoadY: {
                // ABB; A: opcode, B: index
                auto index = readInt16(&instructions->at(instructionIndex + 1));
                y = _objectStack.at(osi + index);
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kLoadZ: {
                // ABB; A: opcode, B: index
                auto index = readInt16(&instructions->at(instructionIndex + 1));
                z = _objectStack.at(osi + index);
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kPushValues: {
                // ABB; A: opcode, B: count
                auto count = readUint16(&instructions->at(instructionIndex + 1));
                vsi -= count;
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kPushObjects: {
                // ABB; A: opcode, B: count
                auto count = readUint16(&instructions->at(instructionIndex + 1));
                osi -= count;
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kPopValues: {
                // ABB; A: opcode, B: count
                auto count = readUint16(&instructions->at(instructionIndex + 1));
                auto endIndex = vsi + count;
                assert(endIndex <= kValueStackSize);
                for (auto i = vsi; i < endIndex; i++) {
                    _valueStack.at(i).num = 0;
                }
                vsi = endIndex;
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kPopObjects: {
                // ABB; A: opcode, B: count
                auto count = readUint16(&instructions->at(instructionIndex + 1));
                auto endIndex = osi + count;
                assert(endIndex <= kObjectStackSize);
                for (auto i = osi; i < endIndex; i++) {
                    _objectStack.at(i) = nullptr;
                }
                osi = endIndex;
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kClearX:
                x = nullptr;
                instructionIndex++;
                break;

            case Opcode::kClearY:
                y = nullptr;
                instructionIndex++;
                break;

            case Opcode::kClearZ:
                z = nullptr;
                instructionIndex++;
                break;

            case Opcode::kSetAFromB:
                a = b;
                instructionIndex++;
                break;

            case Opcode::kSetBFromA:
                b = a;
                instructionIndex++;
                break;

            case Opcode::kSetXFromY:
                x = y;
                instructionIndex++;
                break;

            case Opcode::kSetYFromX:
                y = x;
                instructionIndex++;
                break;

            case Opcode::kAOrB:
                a.setBoolean(a.getBoolean() || b.getBoolean());
                instructionIndex++;
                break;

            case Opcode::kAAndB:
                a.setBoolean(a.getBoolean() && b.getBoolean());
                instructionIndex++;
                break;

            case Opcode::kAEqualsB:
                a.setBoolean(a.num == b.num);
                instructionIndex++;
                break;

            case Opcode::kANotEqualsB:
                a.setBoolean(a.num != b.num);
                instructionIndex++;
                break;

            case Opcode::kALessThanB:
                a.setBoolean(a.num < b.num);
                instructionIndex++;
                break;

            case Opcode::kALessThanEqualsB:
                a.setBoolean(a.num <= b.num);
                instructionIndex++;
                break;

            case Opcode::kAGreaterThanB:
                a.setBoolean(a.num > b.num);
                instructionIndex++;
                break;

            case Opcode::kAGreaterThanEqualsB:
                a.setBoolean(a.num >= b.num);
                instructionIndex++;
                break;

            case Opcode::kAAddB:
                a.num += b.num;
                instructionIndex++;
                break;

            case Opcode::kASubtractB:
                a.num -= b.num;
                instructionIndex++;
                break;

            case Opcode::kAMultiplyB:
                a.num *= b.num;
                instructionIndex++;
                break;

            case Opcode::kADivideB:
                a.num /= b.num;
                instructionIndex++;
                break;

            case Opcode::kAModuloB: {
                auto intA = a.getInt64();
                auto intB = b.getInt64();
                a.num = static_cast<decimal::Decimal>(intA % intB);
                instructionIndex++;
                break;
            }

            case Opcode::kAEqualsConstant: {
                // ABBBBBBBB; A: opcode, B: int64 constant
                int64_t constant = readInt64(&instructions->at(instructionIndex + 1));
                a.setBoolean(a.num == constant);
                instructionIndex += /*A*/ 1 + /*B*/ 8;
                break;
            }

            case Opcode::kBEqualsConstant: {
                // ABBBBBBBB; A: opcode, B: int64 constant
                int64_t constant = readInt64(&instructions->at(instructionIndex + 1));
                a.setBoolean(b.num == constant);
                instructionIndex += /*A*/ 1 + /*B*/ 8;
                break;
            }

            case Opcode::kStringXEqualsY:
                assert(x != nullptr);
                assert(y != nullptr);
                assert(x->getObjectType() == ObjectType::kString);
                assert(y->getObjectType() == ObjectType::kString);
                a.setBoolean((dynamic_cast<String&>(*x).value == dynamic_cast<String&>(*y).value) != 0);
                instructionIndex++;
                break;

            case Opcode::kStringXConcatenateY:
                assert(x != nullptr);
                assert(y != nullptr);
                assert(x->getObjectType() == ObjectType::kString);
                assert(y->getObjectType() == ObjectType::kString);
                x = boost::make_local_shared<String>(dynamic_cast<String&>(*x).value + dynamic_cast<String&>(*y).value);
                instructionIndex++;
                break;

            case Opcode::kJump: {
                // ABBBB; A: opcode, B: index
                auto index = readUint32(&instructions->at(instructionIndex + 1));
                instructionIndex = index;
                break;
            }

            case Opcode::kBranchIfA:
                // ABBBB; A: opcode, B: index
                if (a.getBoolean()) {
                    auto index = readUint32(&instructions->at(instructionIndex + 1));
                    instructionIndex = index;
                } else {
                    instructionIndex += /*A*/ 1 + /*B*/ 4;
                }
                break;

            case Opcode::kBranchIfNotA:
                // ABBBB; A: opcode, B: index
                if (!a.getBoolean()) {
                    auto index = readUint32(&instructions->at(instructionIndex + 1));
                    instructionIndex = index;
                } else {
                    instructionIndex += /*A*/ 1 + /*B*/ 4;
                }
                break;

            case Opcode::kCall: {
                // ABBCD
                // A: opcode
                // B: procedure index
                auto callProcedureIndex = readUint16(&instructions->at(instructionIndex + 1));
                assert(callProcedureIndex >= 0);
                assert(callProcedureIndex < _program->procedures.size());
                auto& callProcedure = *_program->procedures[callProcedureIndex];
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                _callStack.push(CallFrame(procedure, instructionIndex, vsi, osi));
                procedure = &callProcedure;
                instructions = &callProcedure.instructions;
                instructionIndex = 0;
                break;
            }

            case Opcode::kSystemCall: {
                // ABBCD
                // A: opcode
                // B: system call index
                auto systemCallNumber = readUint16(&instructions->at(instructionIndex + 1));
                auto systemCallInput = SystemCallInput(_valueStack, _objectStack, vsi, osi);
                auto result = systemCall(static_cast<SystemCall>(systemCallNumber), systemCallInput);
                a = result.a;
                x = std::move(result.x);
                if (result.hasError) {
                    _hasError = result.hasError;
                    _errorMessage = boost::make_local_shared<String>(result.errorMessage);
                    _errorCode.num = result.errorCode;
                }
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kReturn: {
                auto returnResult = returnFromProcedure(vsi, osi);
                procedure = returnResult.procedure;
                instructions = returnResult.instructions;
                instructionIndex = returnResult.instructionIndex;
                vsi = returnResult.valueStackIndex;
                osi = returnResult.objectStackIndex;
                break;
            }

            case Opcode::kSetError:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kString);
                _errorMessage = x;
                _errorCode = a;
                _hasError = true;
                instructionIndex++;
                break;

            case Opcode::kClearError:
                _errorMessage = nullptr;
                _errorCode.num = 0;
                _hasError = false;
                instructionIndex++;
                break;

            case Opcode::kBubbleError:
                assert(_errorMessage != nullptr);
                _hasError = true;
                instructionIndex++;
                break;

            case Opcode::kReturnIfError:
                if (_hasError) {
                    auto returnResult = returnFromProcedure(vsi, osi);
                    procedure = returnResult.procedure;
                    instructions = returnResult.instructions;
                    instructionIndex = returnResult.instructionIndex;
                    vsi = returnResult.valueStackIndex;
                    osi = returnResult.objectStackIndex;
                } else {
                    instructionIndex++;
                }
                break;

            case Opcode::kPopBranchIfError:
                // ABBCCDDDD; A: opcode, B: num values, C: num objects, D: index
                if (_hasError) {
                    auto popValues = readUint16(&instructions->at(instructionIndex + 1));
                    if (popValues > 0) {
                        auto endIndex = vsi + popValues;
                        assert(endIndex <= kValueStackSize);
                        for (int i = 0; i < popValues; i++) {
                            _valueStack.at(vsi + i).num = 0;
                        }
                        vsi = endIndex;
                    }
                    auto popObjects = readUint16(&instructions->at(instructionIndex + 3));
                    if (popObjects > 0) {
                        auto endIndex = osi + popObjects;
                        assert(endIndex <= kObjectStackSize);
                        for (int i = 0; i < popObjects; i++) {
                            _objectStack.at(osi + i) = nullptr;
                        }
                        osi = endIndex;
                    }
                    auto jumpTarget = readUint32(&instructions->at(instructionIndex + 5));
                    instructionIndex = jumpTarget;
                } else {
                    instructionIndex += /*A*/ 1 + /*B*/ 2 + /*C*/ 2 + /*D*/ 4;
                }
                break;

            case Opcode::kBranchIfNotError:
                // ABBBB; A: opcode, B: index
                if (!_hasError) {
                    auto index = readUint32(&instructions->at(instructionIndex + 1));
                    instructionIndex = index;
                } else {
                    instructionIndex += /*A*/ 1 + /*B*/ 4;
                }
                break;

            case Opcode::kLoadErrorMessageX:
                assert(_errorMessage != nullptr);
                x = _errorMessage;
                instructionIndex++;
                break;

            case Opcode::kLoadErrorCodeA:
                a = _errorCode;
                instructionIndex++;
                break;

            case Opcode::kRecordBuilderBegin: {
                // ABBCC; A: opcode, B: num values, C: num objects
                auto numValues = readUint16(&instructions->at(instructionIndex + 1));
                auto numObjects = readUint16(&instructions->at(instructionIndex + 3));
                _recordBuilderStack.push(RecordBuilder(numValues, numObjects));
                instructionIndex += /*A*/ 1 + /*B*/ 2 + /*C*/ 2;
                break;
            }

            case Opcode::kRecordBuilderStoreA: {
                // ABB; A: opcode, B: index
                assert(!_recordBuilderStack.empty());
                auto& recordBuilder = _recordBuilderStack.top();
                auto index = readUint16(&instructions->at(instructionIndex + 1));
                assert(index < recordBuilder.values.size());
                recordBuilder.values.set(index, a);
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kRecordBuilderStoreX: {
                // ABB; A: opcode, B: index
                assert(!_recordBuilderStack.empty());
                auto& recordBuilder = _recordBuilderStack.top();
                auto index = readUint16(&instructions->at(instructionIndex + 1));
                assert(index < recordBuilder.objects.size());
                assert(x != nullptr);
                recordBuilder.objects.set(index, x);
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kRecordBuilderEnd:
                assert(!_recordBuilderStack.empty());
                x = boost::make_local_shared<Record>(&_recordBuilderStack.top());
                _recordBuilderStack.pop();
                instructionIndex++;
                break;

            case Opcode::kRecordLoadA: {
                // ABB; A: opcode, B: index
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kRecord);
                auto index = readUint16(&instructions->at(instructionIndex + 1));
                auto& record = dynamic_cast<Record&>(*x);
                assert(index < record.values.size());
                a = record.values[index];
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kRecordLoadX: {
                // ABB; A: opcode, B: index
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kRecord);
                auto index = readUint16(&instructions->at(instructionIndex + 1));
                auto& record = dynamic_cast<Record&>(*x);
                assert(index < record.objects.size());
                x = record.objects[index];
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kRecordStoreA: {
                // ABB; A: opcode, B: index
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kRecord);
                auto index = readUint16(&instructions->at(instructionIndex + 1));
                auto& record = dynamic_cast<Record&>(*x);
                assert(index < record.values.size());
                x = boost::make_local_shared<Record>(record, index, a);
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kRecordStoreY: {
                // ABB; A: opcode, B: index
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kRecord);
                auto index = readUint16(&instructions->at(instructionIndex + 1));
                auto& record = dynamic_cast<Record&>(*x);
                assert(index < record.objects.size());
                assert(y != nullptr);
                x = boost::make_local_shared<Record>(record, index, y);
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kValueListBuilderBegin: {
                _valueListBuilderStack.push(ValueListBuilder());
                instructionIndex++;
                break;
            }

            case Opcode::kValueListBuilderAddA:
                assert(!_valueListBuilderStack.empty());
                _valueListBuilderStack.top().items.push_back(a);
                instructionIndex++;
                break;

            case Opcode::kValueListBuilderEnd:
                assert(!_valueListBuilderStack.empty());
                x = boost::make_local_shared<ValueList>(&_valueListBuilderStack.top());
                _valueListBuilderStack.pop();
                instructionIndex++;
                break;

            case Opcode::kValueListGet: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueList);
                auto& valueList = dynamic_cast<ValueList&>(*x);
                a = valueList.items.at(static_cast<size_t>(a.getInt64()));
                instructionIndex++;
                break;
            }

            case Opcode::kValueListSet: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueList);
                auto& valueList = dynamic_cast<ValueList&>(*x);
                x = boost::make_local_shared<ValueList>(valueList, false, static_cast<size_t>(a.getInt64()), b);
                instructionIndex++;
                break;
            }

            case Opcode::kValueListCount: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueList);
                auto& valueList = dynamic_cast<ValueList&>(*x);
                a.num = valueList.items.size();
                instructionIndex++;
                break;
            }

            case Opcode::kValueListInsert: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueList);
                auto& valueList = dynamic_cast<ValueList&>(*x);
                x = boost::make_local_shared<ValueList>(valueList, true, static_cast<size_t>(a.getInt64()), a);
                instructionIndex++;
                break;
            }

            case Opcode::kValueListRemove: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueList);
                auto& valueList = dynamic_cast<ValueList&>(*x);
                x = boost::make_local_shared<ValueList>(valueList, static_cast<size_t>(a.getInt64()));
                instructionIndex++;
                break;
            }

            case Opcode::kObjectListBuilderBegin: {
                _objectListBuilderStack.push(ObjectListBuilder());
                instructionIndex++;
                break;
            }

            case Opcode::kObjectListBuilderAddX:
                assert(!_objectListBuilderStack.empty());
                _objectListBuilderStack.top().items.push_back(x);
                instructionIndex++;
                break;

            case Opcode::kObjectListBuilderEnd:
                assert(!_objectListBuilderStack.empty());
                x = boost::make_local_shared<ObjectList>(&_objectListBuilderStack.top());
                _objectListBuilderStack.pop();
                instructionIndex++;
                break;

            case Opcode::kObjectListGet: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectList);
                auto& objectList = dynamic_cast<ObjectList&>(*x);
                x = objectList.items.at(static_cast<size_t>(a.getInt64()));
                instructionIndex++;
                break;
            }

            case Opcode::kObjectListSet: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectList);
                auto& objectList = dynamic_cast<ObjectList&>(*x);
                x = boost::make_local_shared<ObjectList>(objectList, false, static_cast<size_t>(a.getInt64()), y);
                instructionIndex++;
                break;
            }

            case Opcode::kObjectListCount: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectList);
                auto& objectList = dynamic_cast<ObjectList&>(*x);
                a.num = objectList.items.size();
                instructionIndex++;
                break;
            }

            case Opcode::kObjectListInsert: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectList);
                auto& objectList = dynamic_cast<ObjectList&>(*x);
                x = boost::make_local_shared<ObjectList>(objectList, true, static_cast<size_t>(a.getInt64()), y);
                instructionIndex++;
                break;
            }

            case Opcode::kObjectListRemove: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectList);
                auto& objectList = dynamic_cast<ObjectList&>(*x);
                x = boost::make_local_shared<ObjectList>(objectList, static_cast<size_t>(a.getInt64()));
                instructionIndex++;
                break;
            }

            case Opcode::kValueToValueMapNew:
                x = boost::make_local_shared<ValueToValueMap>();
                instructionIndex++;
                break;

            case Opcode::kValueToValueMapTryGet: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueToValueMap);
                const auto* found = dynamic_cast<ValueToValueMap&>(*x).pairs.find(a);
                if (found == nullptr) {
                    b.setBoolean(false);
                } else {
                    b.setBoolean(true);
                    a = *found;
                }
                instructionIndex++;
                break;
            }

            case Opcode::kValueToValueMapCount:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueToValueMap);
                a.num = dynamic_cast<ValueToValueMap&>(*x).pairs.size();
                instructionIndex++;
                break;

            case Opcode::kValueToValueMapSet:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueToValueMap);
                x = boost::make_local_shared<ValueToValueMap>(dynamic_cast<ValueToValueMap&>(*x), a, b);
                instructionIndex++;
                break;

            case Opcode::kValueToValueMapRemove:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueToValueMap);
                x = boost::make_local_shared<ValueToValueMap>(dynamic_cast<ValueToValueMap&>(*x), a);
                instructionIndex++;
                break;

            case Opcode::kValueToValueMapKeys:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueToValueMap);
                x = dynamic_cast<ValueToValueMap&>(*x).keys();
                instructionIndex++;
                break;

            case Opcode::kValueToValueMapValues:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueToValueMap);
                x = dynamic_cast<ValueToValueMap&>(*x).values();
                instructionIndex++;
                break;

            case Opcode::kValueToObjectMapNew:
                x = boost::make_local_shared<ValueToObjectMap>();
                instructionIndex++;
                break;

            case Opcode::kValueToObjectMapTryGet: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueToObjectMap);
                const auto* found = dynamic_cast<ValueToObjectMap&>(*x).pairs.find(a);
                if (found == nullptr) {
                    b.setBoolean(false);
                } else {
                    b.setBoolean(true);
                    x = *found;
                }
                instructionIndex++;
                break;
            }

            case Opcode::kValueToObjectMapCount:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueToObjectMap);
                a.num = dynamic_cast<ValueToObjectMap&>(*x).pairs.size();
                instructionIndex++;
                break;

            case Opcode::kValueToObjectMapSet:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueToObjectMap);
                x = boost::make_local_shared<ValueToObjectMap>(dynamic_cast<ValueToObjectMap&>(*x), a, y);
                instructionIndex++;
                break;

            case Opcode::kValueToObjectMapRemove:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueToObjectMap);
                x = boost::make_local_shared<ValueToObjectMap>(dynamic_cast<ValueToObjectMap&>(*x), a);
                instructionIndex++;
                break;

            case Opcode::kValueToObjectMapKeys:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueToObjectMap);
                x = dynamic_cast<ValueToObjectMap&>(*x).keys();
                instructionIndex++;
                break;

            case Opcode::kValueToObjectMapValues:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kValueToObjectMap);
                x = dynamic_cast<ValueToObjectMap&>(*x).values();
                instructionIndex++;
                break;

            case Opcode::kObjectToValueMapNew:
                x = boost::make_local_shared<ObjectToValueMap>();
                instructionIndex++;
                break;

            case Opcode::kObjectToValueMapTryGet: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectToValueMap);
                const auto* found = dynamic_cast<ObjectToValueMap&>(*x).pairs.find(y);
                if (found == nullptr) {
                    b.setBoolean(false);
                } else {
                    b.setBoolean(true);
                    a = *found;
                }
                instructionIndex++;
                break;
            }

            case Opcode::kObjectToValueMapCount:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectToValueMap);
                a.num = dynamic_cast<ObjectToValueMap&>(*x).pairs.size();
                instructionIndex++;
                break;

            case Opcode::kObjectToValueMapSet:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectToValueMap);
                x = boost::make_local_shared<ObjectToValueMap>(dynamic_cast<ObjectToValueMap&>(*x), y, a);
                instructionIndex++;
                break;

            case Opcode::kObjectToValueMapRemove:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectToValueMap);
                x = boost::make_local_shared<ObjectToValueMap>(dynamic_cast<ObjectToValueMap&>(*x), y);
                instructionIndex++;
                break;

            case Opcode::kObjectToValueMapKeys:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectToValueMap);
                x = dynamic_cast<ObjectToValueMap&>(*x).keys();
                instructionIndex++;
                break;

            case Opcode::kObjectToValueMapValues:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectToValueMap);
                x = dynamic_cast<ObjectToValueMap&>(*x).values();
                instructionIndex++;
                break;

            case Opcode::kObjectToObjectMapNew:
                x = boost::make_local_shared<ObjectToObjectMap>();
                instructionIndex++;
                break;

            case Opcode::kObjectToObjectMapTryGet: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectToObjectMap);
                const auto* found = dynamic_cast<ObjectToObjectMap&>(*x).pairs.find(y);
                if (found == nullptr) {
                    b.setBoolean(false);
                } else {
                    b.setBoolean(true);
                    x = *found;
                }
                instructionIndex++;
                break;
            }

            case Opcode::kObjectToObjectMapCount:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectToObjectMap);
                a.num = dynamic_cast<ObjectToObjectMap&>(*x).pairs.size();
                instructionIndex++;
                break;

            case Opcode::kObjectToObjectMapSet:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectToObjectMap);
                x = boost::make_local_shared<ObjectToObjectMap>(dynamic_cast<ObjectToObjectMap&>(*x), y, z);
                instructionIndex++;
                break;

            case Opcode::kObjectToObjectMapRemove:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectToObjectMap);
                x = boost::make_local_shared<ObjectToObjectMap>(dynamic_cast<ObjectToObjectMap&>(*x), y);
                instructionIndex++;
                break;

            case Opcode::kObjectToObjectMapKeys:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectToObjectMap);
                x = dynamic_cast<ObjectToObjectMap&>(*x).keys();
                instructionIndex++;
                break;

            case Opcode::kObjectToObjectMapValues:
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kObjectToObjectMap);
                x = dynamic_cast<ObjectToObjectMap&>(*x).values();
                instructionIndex++;
                break;

            case Opcode::kValueOptionalNewMissing:
                x = boost::make_local_shared<ValueOptional>();
                instructionIndex++;
                break;

            case Opcode::kValueOptionalNewPresent:
                x = boost::make_local_shared<ValueOptional>(a);
                instructionIndex++;
                break;

            case Opcode::kObjectOptionalNewMissing:
                x = boost::make_local_shared<ObjectOptional>();
                instructionIndex++;
                break;

            case Opcode::kObjectOptionalNewPresent:
                assert(x != nullptr);
                x = boost::make_local_shared<ObjectOptional>(x);
                instructionIndex++;
                break;

            case Opcode::kValueGlobalStore: {
                // ABB; A: opcode, B: index
                auto index = readUint16(&instructions->at(instructionIndex + 1));
                _program->globalValues.at(index) = a;
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kValueGlobalLoad: {
                // ABB; A: opcode, B: index
                auto index = readUint16(&instructions->at(instructionIndex + 1));
                a = _program->globalValues.at(index);
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kObjectGlobalStore: {
                // ABB; A: opcode, B: index
                auto index = readUint16(&instructions->at(instructionIndex + 1));
                _program->globalObjects.at(index) = x;
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kObjectGlobalLoad: {
                // ABB; A: opcode, B: index
                auto index = readUint16(&instructions->at(instructionIndex + 1));
                x = _program->globalObjects.at(index);
                instructionIndex += /*A*/ 1 + /*B*/ 2;
                break;
            }

            case Opcode::kStringMid: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kString);
                auto& str = dynamic_cast<String&>(*x);
                auto midOffset = a.getInt64();
                auto midLength = b.getInt64();
                if (midOffset < 0) {
                    midLength += midOffset;
                    midOffset = 0;
                }
                if (midOffset < static_cast<int64_t>(str.value.length()) && midLength > 0) {
                    x = boost::make_local_shared<String>(icu::UnicodeString(str.value, midOffset, midLength));
                } else {
                    x = boost::make_local_shared<String>(icu::UnicodeString());
                }
                instructionIndex++;
                break;
            }

            case Opcode::kStringIndexOf: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kString);
                assert(y != nullptr);
                assert(y->getObjectType() == ObjectType::kString);
                auto& haystack = dynamic_cast<String&>(*x);
                auto startIndex = a.getInt64();
                if (startIndex >= static_cast<int64_t>(haystack.value.length())) {
                    a.num = -1;
                } else {
                    if (startIndex < 0) {
                        startIndex = 0;
                    }
                    auto& needle = dynamic_cast<String&>(*y);
                    a.num = haystack.value.indexOf(needle.value, startIndex);
                }
                instructionIndex++;
                break;
            }

            case Opcode::kStringAsc: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kString);
                auto& str = dynamic_cast<String&>(*x);
                decimal::Decimal strLength = str.value.length();
                auto intA = a.getInt64();
                if (intA >= 0 && intA < strLength) {
                    a.num = static_cast<int>(str.value[intA]);
                } else {
                    a.num = -1;
                }
                instructionIndex++;
                break;
            }

            case Opcode::kStringPrint: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kString);
                auto& str = dynamic_cast<String&>(*x);
                *_consoleOutputStream << str.toUtf8();
                instructionIndex++;
                break;
            }

            case Opcode::kStringInputLine: {
                std::string str;
                std::getline(*_consoleInputStream, str);
                x = boost::make_local_shared<String>(str);
                instructionIndex++;
                break;
            }

            case Opcode::kNumberToString:
                x = boost::make_local_shared<String>(a.getString());
                instructionIndex++;
                break;

            case Opcode::kStringToNumber: {
                assert(x != nullptr);
                assert(x->getObjectType() == ObjectType::kString);
                auto& str = dynamic_cast<String&>(*x);
                auto utf8 = str.toUtf8();
                if (std::regex_match(utf8, std::regex("^-?[0-9]+\\.?[0-9]*$"))) {
                    a.num = util::parseDecimalString(utf8);
                    b.num = 1;
                } else {
                    a.num = 0;
                    b.num = 0;
                }
                instructionIndex++;
                break;
            }

            default:
                std::cerr << "Unimplemented opcode: " << instructions->at(instructionIndex) << std::endl;
                assert(false);
                break;
        }
    }

    // write state back to memory for the next run call
    _procedure = procedure;
    _instructionIndex = instructionIndex;
    _a = a;
    _b = b;
    _x = x;
    _y = y;
    _z = z;
    _valueStackIndex = vsi;
    _objectStackIndex = osi;

    if (fatalError != nullptr) {
        throw std::runtime_error(fatalError);
    }

    return more;
}

Interpreter::ReturnResult::ReturnResult(
    const Procedure* procedure,
    const std::vector<uint8_t>* instructions,
    size_t instructionIndex,
    int valueStackIndex,
    int objectStackIndex)
    : procedure(procedure),
      instructions(instructions),
      instructionIndex(instructionIndex),
      valueStackIndex(valueStackIndex),
      objectStackIndex(objectStackIndex) {}

Interpreter::ReturnResult Interpreter::returnFromProcedure(int valueStackIndex, int objectStackIndex) {
    assert(!_callStack.empty());
    auto callFrame = _callStack.top();
    assert(valueStackIndex <= callFrame.valueStackIndex);
    for (int i = valueStackIndex; i < callFrame.valueStackIndex; i++) {
        _valueStack.at(i).num = 0;
    }
    assert(objectStackIndex <= callFrame.objectStackIndex);
    for (int i = objectStackIndex; i < callFrame.objectStackIndex; i++) {
        _objectStack.at(i) = nullptr;
    }
    _callStack.pop();
    return ReturnResult(
        callFrame.procedure, &callFrame.procedure->instructions, callFrame.instructionIndex, callFrame.valueStackIndex,
        callFrame.objectStackIndex);
}

}  // namespace vm
