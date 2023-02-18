#ifndef _VERSION_H
#define _VERSION_H

#define stringify(s) _stringifyDo(s)
#define _stringifyDo(s) #s

const char *project() { return stringify(MAKE_SRC_NAM); }
const char *version() { return stringify(MAKE_SRC_TAG); }
const char *commit() { return stringify(MAKE_SRC_REV); }
const char *branch() { return stringify(MAKE_SRC_BRH); }

#endif // _VERSION_H
