#ifndef INCLUDE_MAL_H
#define INCLUDE_MAL_H

#include "debug.h"
#include "refcountedptr.h"
#include "string.h"
#include "validation.h"

#include <vector>

class malValue;
typedef RefCountedPtr<malValue>  malValuePtr;
typedef std::vector<malValuePtr> malValueVec;
typedef malValueVec::iterator    malValueIter;

class malEnv;
typedef RefCountedPtr<malEnv>     malEnvPtr;

// step*.cpp
extern malValuePtr APPLY(malValuePtr op,
                         malValueIter argsBegin, malValueIter argsEnd);
extern malValuePtr EVAL(malValuePtr ast, malEnvPtr env);
extern malValuePtr readline(const String& prompt);
extern String rep(const String& input, malEnvPtr env);

// Core.cpp
extern void installCore(malEnvPtr env);

// Reader.cpp
extern malValuePtr readStr(const String& input);

malEnvPtr malInit();
String malRepl(const String& input, malEnvPtr env);

#endif // INCLUDE_MAL_H
