#pragma once

#include "../common.h"

namespace compiler {

enum class TokenKind {
    kError,
    kEndOfLine,
    kEndOfFile,
    kIdentifier,
    kComment,

    // literals
    kBooleanLiteral,
    kNumberLiteral,
    kStringLiteral,

    // punctuation
    kLeftParenthesis,
    kRightParenthesis,
    kLeftBracket,
    kRightBracket,
    kLeftBrace,
    kRightBrace,
    kColon,
    kComma,
    kDot,
    kPlusSign,
    kMinusSign,
    kMultiplicationSign,
    kDivisionSign,
    kEqualsSign,
    kNotEqualsSign,
    kLessThanSign,
    kLessThanEqualsSign,
    kGreaterThanSign,
    kGreaterThanEqualsSign,

    // keywords
    kAnd,
    kAs,
    kBoolean,
    kBy,
    kCase,
    kCatch,
    kConst,
    kContinue,
    kDate,
    kDateTime,
    kDateTimeOffset,
    kDim,
    kDo,
    kEach,
    kElse,
    kEnd,
    kExit,
    kFalse,
    kFinally,
    kFor,
    kFrom,
    kFunction,
    kGroup,
    kIf,
    kIn,
    kInto,
    kJoin,
    kKey,
    kList,
    kLoop,
    kMap,
    kMod,
    kNext,
    kNot,
    kNumber,
    kOf,
    kOn,
    kOptional,
    kOr,
    kRecord,
    kRethrow,
    kReturn,
    kSelect,
    kStep,
    kSub,
    kString,
    kThen,
    kThrow,
    kTimeSpan,
    kTimeZone,
    kTo,
    kTrue,
    kTry,
    kType,
    kUntil,
    kWend,
    kWhere,
    kWhile,
    kWith,
};

}  // namespace compiler
