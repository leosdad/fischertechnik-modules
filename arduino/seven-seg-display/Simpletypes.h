/*
  Simpletypes.h
*/
#ifndef simpletypes_h
#define simpletypes_h

#ifndef uint
#define uint unsigned int
#endif

#ifndef ulong
#define ulong unsigned long
#endif

#ifndef ULONG_MAX
#define ULONG_MAX			0xFFFFFFFFUL
#endif

#ifndef NUMITEMS
#define NUMITEMS(arg) ((uint) (sizeof (arg) / sizeof (arg[0])))
#endif

#endif // simpletypes_h
