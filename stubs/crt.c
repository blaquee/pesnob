//#pragma function(memset)
#pragma code_seg(".stub$b")
void * __cdecl memset(
	void *dst,
	int val,
	unsigned int count
)
{
	void *start = dst;

	while (count--) {
		*(char *)dst = (char)val;
		dst = (char *)dst + 1;
	}

	return(start);
}

void * __cdecl __memcpy(
	void * dst,
	const void * src,
	unsigned int count
)
{
	void * ret = dst;

	/*
	* copy from lower addresses to higher addresses
	*/
	while (count--) {
		*(char *)dst = *(char *)src;
		dst = (char *)dst + 1;
		src = (char *)src + 1;
	}

	return(ret);
}

char* __strcpy(char* dst, const char* src)
{
	if (!dst || !src)
		return 0;
	char* temp = dst;
	while ((*dst++ = *src++) != '\0')
		;
	return temp;
}

int __strlen(const char* str)
{
	const char* tmp;
	if (!str)
		return -1;
	tmp = str;
	int count = 0;
	while (tmp++ != '\0')
		count++;
	return count;
}
int __strncmp(const char* s1, const char* s2, unsigned int n)
{
	for (; n > 0; s1++, s2++, n--)
		if (*s1 != *s2)
			return((*(unsigned char*)s1 < *(unsigned char*)s2) ? -1 : 1);
		else if (*s1 == '\0')
			return 0;
	return 0;
}