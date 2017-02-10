#ifndef __MACRO_H_
#define __MACRO_H_

#define CALC_OFFSET(type, ptr, offset) (type) (((ULONG64) ptr) + offset)
#define CALC_OFFSET_DISP(type, base, offset, disp) (type)((DWORD)(base) + (DWORD)(offset) + disp)
#define CALC_DISP(type, offset, ptr) (type) (((ULONG64) offset) - (ULONG64) ptr)


//Helper function to align number down
template<typename T>
static inline T align_down(T x, uint32_t align)
{
	return x & ~(static_cast<T>(align) - 1);
}

//Helper function to align number up
template<typename T>
static inline T align_up(T x, uint32_t align)
{
	return (x & static_cast<T>(align - 1)) ? align_down(x, align) + static_cast<T>(align) : x;
}

//Helper template function to strip nullbytes in the end of string
template<typename T>
static void strip_nullbytes(std::basic_string<T>& str)
{
	while (!*(str.end() - 1) && !str.empty())
		str.erase(str.length() - 1);
}



#endif
