/*
 * Puzzles version numbering.
 */

#define STR1(x) #x
#define STR(x) STR1(x)

#if defined REVISION

char ver[] = "Revision: r" STR(REVISION);

#else

char ver[] = "Unknown version";

#endif

char stppc2x_ver[] = "STPPC2x v1.1";
