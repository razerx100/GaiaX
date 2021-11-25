#ifndef __S_UTILITY_HPP__
#define __S_UTILITY_HPP__

#ifdef BUILD_GAIAX
#define GAIAX_DLL __declspec(dllexport)
#else
#define GAIAX_DLL __declspec(dllimport)
#endif

struct GAIAX_DLL SRect {
	long left;
	long top;
	long right;
	long bottom;
};
#endif
