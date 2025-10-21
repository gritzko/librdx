#ifndef ABC_SAN_H
#define ABC_SAN_H

// ABC defines its own C dialect, including a bunch of nice macros
// that may clash with your names (cause no namespaces in C).
// To sanitize the C namespace please use this header.
// Regular ABC functions have such cryptic and formal names that I
// can't believe they will ever clash with anything -- gritzko

#undef call
#undef done
#undef sane
#undef nedo
#undef then
#undef fail
#undef try
#undef NULL
#undef pro
#undef fun
#undef con

#endif
