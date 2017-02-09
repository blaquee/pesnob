/*
íâ?â ?????à?ë? ?áå????ª ?à??áâ???ï?â á???? ???ªãî ???ëâªã LZW- (??? ??å?????)
ª???à?????ï
- ª???à?????? ??â?ª????
- ? ª?ç?áâ?? á????àï ?á???ì?ã?âáï ??à??? (ª??â?ªáâ????-? åã? ??? ????â...)
- ???á???ì?ã??ë? í?????âë á????àï ã???ïîâáï
- á?????ì?ë? áâà?ª? ?????ïîâáï ????à??? ã???? ??à???
- ????à? ã???? á????îâáï å?ää?????
- à???â??â ????????, ?à?â ??åà??? ???ïâ?
- ??ëâ ??ª???? çâ? ??âì ª??ë å?ää????? ???????? ? ? ???ìè??áâ?? á?ãç???
ª?? ?é? ? ???ìè? áâ?????âáï.
- ?à? ?âª?îç????? å?ää???? ª??ë á????îâáï ª?ª ? ª?ª??-â? â?? LZW, ?? 8,9,
10 ? â.?. ??â ?? ????à, ? â?ª ?? ç?áâª? á????àï

(c) 1999 Z0MBiE, http://z0mbie.cjb.net
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <io.h>
#include <time.h>

#define USE_HUFF

#pragma extref _floatconvert

#define TREE_MAX        65536    // ??ªá. ª??-?? í?-â?? ??à???
#define TREE_DEPTH      8        // ??ªá. ????????áâì ??à???
#define TREE_HINT_COUNT 1        // ?????ã?: 1 áâà?ª? ?? 8 ª??., ???ç?
#define TREE_HINT_RANGE 8192     //            ã????âì áâà?ªã ?? ??à???
#define TREE_FILT_RANGE 1024     // ç?à?? áª??ìª? ???â ??????ïâì ??à???

#undef USE_HUFF                 // á????âì ª??ë å?ää?????, ????ë? ?âáâ??

/*
#define min(a,b)        ((a)<(b)?(a):(b))
#define max(a,b)        ((a)>(b)?(a):(b))
*/

typedef struct _node             // ã??? ??à???. ??ª?â?àë? í?-âë ?? ?á???ì?ãîâáï
{
	unsigned long       count;          // áª??ìª? à?? áâà?ª? ?áâà?ç???áì ? â?ªáâ?
	unsigned long       hint;           // áª??ìª? áâà?ª ???????/á??â?
	unsigned long       lastoffs;       // ??á?????? ?ääá?â áâà?ª?
	unsigned long       level;          // ãà????ì ????????áâ? ã??? = ????? áâà?ª?
	unsigned long       sym;            // char, â?ªãé?? á?????
	unsigned long       index;          // ????à ã??? ??à???
	struct _node        *prev;          // ?à??ë?ãé?? ã???
	unsigned long       nextcount;      // ç?á?? ???ã???ëå í?????â?? ? next
	struct _node*       (*next)[256];   // NULL ??? ãª???â??? ?? á???ãîé?? í?-âë
} node;

node* root;                     // ª?à??ì
unsigned long nodecount;                // ç?á?? ã???? ??? ª?â?àë? ?ë?????? ???ïâì
unsigned long tree_indexcount;          // ç?á?? ã???? ??à???
node* tree_index[TREE_MAX];     // ??áá?? ãª???â???? ?? ?á? ã??ë ??à???

node* tree_allocnode()          // ?ë????âì ã???
{
	nodecount++;
	node* x = (node*)calloc(1, sizeof(node));
	assert(x != NULL);
	return x;
};

void tree_deallocnode(node** x) // ã??âì ã???
{
	assert(*x != NULL);
	if ((*x)->next != NULL)
	{
		assert((*x)->nextcount == 0);
		free((*x)->next);
	};
	free(*x);
	*x = NULL;
	nodecount--;
};

node* tree_addnode(node** x, unsigned long i)  // ??????âì ã??? ?? ã?? áãé?áâ?ãîé???
{
	assert(*x != NULL);
	if ((*x)->next == NULL)
	{
		//node** tmp = (node**)calloc(256, 4);
		//assert(tmp != NULL);
		//memcpy((*x)->next, tmp, 256 * 4);
		(*x)->next = (node*(*)[256])calloc(256, 4);
	}
	(*x)->nextcount++;
	(*(*x)->next)[i] = tree_allocnode();
	(*(*x)->next)[i]->prev = *x;
	(*(*x)->next)[i]->sym = i;
	(*(*x)->next)[i]->level = (*x)->level + 1;
	(*(*x)->next)[i]->index = tree_indexcount++;
	tree_index[(*(*x)->next)[i]->index] = (*(*x)->next)[i];
	return (*(*x)->next)[i];
};

void tree_delnode(node** x)            // ã????âì ã??? ? ?á?å ??á?????ª??
{
	assert(*x != NULL);
	if ((*x)->next != NULL)
		for (int i = 0; i<256; i++)
			if ((*(*x)->next)[i] != NULL)
				tree_delnode(&(*(*x)->next)[i]);
	assert((*x)->nextcount == 0);
	if ((*x)->prev != NULL)
		(*x)->prev->nextcount--;
	tree_deallocnode(x);
};

void tree_init()                       // ???æ??????à???âì root
{
	nodecount = 0;
	root = tree_allocnode();
	tree_indexcount = 0;
	memset(tree_index, 0, sizeof(tree_index));
};

void tree_init1()                      // ???æ??????à???âì ??à?ë? ãà????ì
{
	for (int i = 0; i<256; i++)
		tree_addnode(&root, i);
};

void tree_done()                       // ?áî ???ïâì ?á??????âì ª å?à??
{
	tree_delnode(&root);
	assert(nodecount == 0);
};

unsigned char* tree_node2string(node* x)        // ã??? --> áâà?ª?
{
	assert(x != NULL);
	static unsigned char s[TREE_DEPTH + 1];
	memset(s, 0, sizeof(s));
	while (x->prev != NULL)
	{
		s[x->level - 1] = x->sym;
		x = x->prev;
	};
	return s;
};

node* tree_string2node(unsigned char* s)        // áâà?ª? --> ã???
{
	assert(s != NULL);
	node* x = root;
	node* p;
	while (x != NULL)
	{
		p = x;
		if (*x->next == NULL) x = NULL; else x = (*x->next)[*s++];
	};
	return p;
};

unsigned long tree_thisoffs;                   // â?ªãé?? ?äá?â ? ä????

void tree_update(unsigned char* s)              // ??????âì áâà?ªã ? ??à???
{
	assert(s != NULL);
	node** x = &root;
	for (int i = 0; i<TREE_DEPTH; i++)
	{
		if (((*x)->next == NULL) ||
			((*(*x)->next)[*(s + i)] == NULL))
			tree_addnode(x, *(s + i));
		x = &(*(*x)->next)[*(s + i)];
		(*x)->count++;
		(*x)->lastoffs = tree_thisoffs;
	};
};

void tree_filter_node(node** x)        // ä??ìâà???âì ã??ë ??ç???ï á ???????
{
	assert(*x != NULL);

	if (*x != NULL)
		if ((*x)->next != NULL)
			for (int i = 0; i<256; i++)
				if ((*(*x)->next)[i] != NULL)
					tree_filter_node(&(*(*x)->next)[i]);

	assert(*x != NULL);

	if (((*x)->hint + 1)*TREE_HINT_RANGE<
		(tree_thisoffs - (*x)->lastoffs)*TREE_HINT_COUNT)
	{
		//    printf("kill: hint=%i range=%i [%s]\n",(*x)->hint, tree_thisoffs-(*x)->lastoffs,tree_node2string(*x));
		tree_delnode(x);
	};
};

void tree_filter()                     // ä??ìâà???âì ??à???
{
	if (root->next != NULL)
		for (int i = 0; i<256; i++)
			if ((*root->next)[i] != NULL)
				if ((*root->next)[i]->next != NULL)
					for (int j = 0; j<256; j++)
						if ((*(*root->next)[i]->next)[j] != NULL)
							tree_filter_node(&(*(*root->next)[i]->next)[j]);
};

void tree_enum_node(node* x)    // ??à??ã??à???âì ????ªáë(????à?) ã????
{
	assert(x != NULL);
	x->index = tree_indexcount++;
	tree_index[x->index] = x;
	if (x->next != NULL)
		for (int i = 0; i<256; i++)
			if ((*x->next)[i] != NULL)
				tree_enum_node((*x->next)[i]);
};

void tree_enum()                // ??à??ã??à???âì ?á? ????ªáë
{
	tree_indexcount = 0;
	memset(tree_index, 0, sizeof(tree_index));
	tree_enum_node(root);
	assert(tree_indexcount == nodecount);
};

void tree_dump_node(node* x)    // ???? ã???
{
	assert(x != NULL);
	/*if (x->prev == NULL)
		//printf("[ROOT]\n");
	else
		//printf("[%s] hint=%i lastoffs=%i level=%i index=%i nextcount=%i\n",
		//tree_node2string(x), (int)x->hint, (int)x->lastoffs, (int)x->level, (int)x->index, (int)x->nextcount);
		*/
	if (x->next != NULL)
		for (int i = 0; i<256; i++)
			if ((*x->next)[i] != NULL)
				tree_dump_node((*x->next)[i]);
};

void tree_dump()                // ???? ??à???
{
	tree_dump_node(root);
};

#ifdef USE_HUFF

/* ?á? ??ï å?ää?????áª??? ??à??? */
/* ????ªá?æ?ï ?à??????ì?ëå ª???? á???????â á ?á????ë? ??à???? */

unsigned long huff_max0;                // ??ç??ì??? ç?á?? ª????
unsigned long huff_count[TREE_MAX * 2];   // ç?á?? ???â?à???? ª???
unsigned long huff_next[TREE_MAX * 2];   // á???ãîé?? í?-â
unsigned long huff_prev0[TREE_MAX * 2];   // ?à??ë?ãé??-???ë? í?-â
unsigned long huff_prev1[TREE_MAX * 2];   // ?à??ë?ãé??-?à??ë? í?-â
unsigned long huff_bit[TREE_MAX * 2];   // ??â (0=???ë?, 1=?à??ë?)
unsigned long huff_max;                 // â?ªãé?? ç?á?? í?-â? ??à???
unsigned long huff_min;                 // â?ªãé?? ???ç???? ???????ì???? huff_count[i]
unsigned long huff_left;                // ????à ªà?????? ?????? ??á??????????? í?-â?

void huff_init()
{
};

unsigned long huff_findmin()            // ???áª í?-â? á ???????ì?ë? ???ç?????
{
	unsigned long mi = 0xFFFFFFFF;
	unsigned long mv = 0xFFFFFFFF;
	while (huff_next[huff_left] != 0) huff_left++;
	for (unsigned long i = huff_left; i<huff_max; i++)    // 0=root -- not used
		if (huff_next[i] == 0)
			if (huff_count[i]<mv)
			{
				mi = i;
				mv = huff_count[i];
				if (mv <= huff_min) break;
			};
	huff_min = mv;
	return mi;
};

void huff_build()               // ¯®áâà®¨âì å ää¬ ­®¢áª®¥ ¤¥à¥¢®
{
	huff_max0 = tree_indexcount;
	huff_max = tree_indexcount;
	for (unsigned long i = 0; i<tree_indexcount; i++)
		huff_count[i] = tree_index[i]->count * tree_index[i]->level;

	memset(huff_next, 0, sizeof(huff_next));
	huff_min = 0;
	huff_left = 1;

	for (;;)
	{
		unsigned long a = huff_findmin();
		huff_next[a] = huff_max;
		assert(a != 0xFFFFFFFF);
		unsigned long b = huff_findmin();
		if (b == 0xFFFFFFFF)
		{
			huff_next[a] = 0;
			assert(a == huff_max - 1);
			break;
		};
		huff_next[b] = huff_max;
		huff_prev0[huff_max] = a;
		huff_prev1[huff_max] = b;
		huff_count[huff_max] = huff_count[a] + huff_count[b];
		huff_bit[a] = 0;
		huff_bit[b] = 1;
		huff_max++;
	};
};

#endif

void showstat(unsigned long max, unsigned long i, unsigned long o)
{
	unsigned char s[41];
	s[40] = 0;
	unsigned long ti = i * 40 / max;
	unsigned long to = o * 40 / max;
	if (ti<to) { ti ^= to; to ^= ti; ti ^= to; };
	for (int j = 0; j<40; j++)
	{
		s[j] = 0xB0;
		if (j <= ti) s[j] = 0xB1;
		if (j <= to) s[j] = 0xB2;
	};
	//printf("[%s] %5.1f%% %6.2fx %5i-->%5i %6i#\x0D",
		//s, (float)max(i, o) * 100 / max, (float)o / (i + 1), (int)i, (int)o, (int)nodecount);
};

inline unsigned long ln2(unsigned long x)
{
	unsigned long y = 8;
	if (x >= 256) y++;
	if (x >= 512) y++;
	if (x >= 1024) y++;
	if (x >= 2048) y++;
	if (x >= 4096) y++;
	if (x >= 8192) y++;
	if (x >= 16384) y++;
	if (x >= 32768) y++;
	return y;
};

/* § ¯ ª®¢ âì ¡ãä¥à */

void encode_buf(unsigned char ibuf[], unsigned long isize, unsigned char obuf[], unsigned long *osize)
{
	assert(ibuf != NULL);
	assert(obuf != NULL);

	tree_init();
	tree_init1();
	tree_enum();

#ifdef USE_HUFF
	huff_init();
	huff_build();
#endif

	*osize = 0;
	unsigned long ocode = 0, olen = 0;
	unsigned long i, c;
	for (i = 0; i<isize; )
	{
		if (clock() - c>CLK_TCK / 4)
		{
			c = clock();
			showstat(isize, i, *osize);
		};

		tree_thisoffs = i;                // â¥ªãé¨© ®äá¥â
		node *x;
		if (i<isize - TREE_DEPTH + 1)
			x = tree_string2node(&ibuf[i]); // á¤¥« âì ¨§ áâà®ª¨ ª®¤
		else
			x = (*root->next)[ibuf[i]];
		assert(x != NULL);

#ifdef USE_HUFF
		while (x->index >= huff_max0) x = x->prev;
		assert(x != NULL);
#endif

		unsigned long code, len;

#ifdef USE_HUFF
		unsigned long index = x->index;             // á¦ âì ª®¤ å ää¬ ­®¬
		code = len = 0;
		while (huff_next[index] != 0)
		{
			code <<= 1;
			code |= huff_bit[index];
			len++;
			index = huff_next[index];
		};

#else
		code = x->index;
		len = ln2(tree_indexcount);
#endif

		ocode |= code << olen;                // ¤®¡ ¢¨âì ¢ ¢ëå®¤­®© ¯®â®ª
		olen += len;
		while (olen >= 8)
		{
			obuf[(*osize)++] = ocode & 255;
			ocode >>= 8;
			olen -= 8;
		};

		x->hint++;

		for (int j = 0; j<x->level; j++)    //  ¯¤¥©â¨¬ á«®¢ àì
		{
			if (i >= TREE_DEPTH - 1)
				if (nodecount<TREE_MAX)
					tree_update(&ibuf[i - TREE_DEPTH + 1]);
			i++;

			if ((i%TREE_FILT_RANGE) == 0)
			{
				tree_filter();                // ç¨áâ¨¬ á«®¢ àì
				tree_enum();
#ifdef USE_HUFF
				huff_build();
#endif
			};

		};

	};
	if (olen != 0) obuf[(*osize)++] = ocode;

	showstat(isize, i, *osize);

	tree_done();
};

/* à á¯ ª®¢ âì ¡ãä¥à */

void decode_buf(unsigned char ibuf[], unsigned long isize, unsigned char obuf[], unsigned long osize)
{
	assert(ibuf != NULL);
	assert(obuf != NULL);

	tree_init();
	tree_init1();
	tree_enum();

#ifdef USE_HUFF
	huff_init();
	huff_build();
#endif

	unsigned long i = 0, icode = 0, ilen = 0;
	unsigned long o, c;

	for (o = 0; (o<osize) && (i<isize); )
	{
		if (clock() - c>CLK_TCK / 4)
		{
			c = clock();
			showstat(osize, i, o);
		};

		tree_thisoffs = o;

		unsigned long index;

#ifdef USE_HUFF
		index = huff_max - 1;                 // ç¨â ¥¬ ¯® ¡¨â ¬,
		do                                // ¨¤¥¬ ¯® å ää¬ ­®¢áª®¬ã ¤¥à¥¢ã,
		{                       // ¯®«ãç ¥¬ ¨­¤¥ªá áâà®ª¨ ¢ ®á­®¢­®¬ ¤¥à¥¢¥
			if (ilen == 0)
			{
				icode |= ibuf[i++] << ilen;
				ilen += 8;
			};
			if ((icode & 1) == 0)
				index = huff_prev0[index];
			else
				index = huff_prev1[index];
			icode >>= 1;
			ilen--;
		} while (index >= huff_max0);
#else
		unsigned long len = ln2(tree_indexcount);   // ¨­ ç¥ ¯à®áâ® ç¨â ¥¬ ¨­¤¥ªá
		while (ilen<len)
		{
			icode |= ibuf[i++] << ilen;
			ilen += 8;
		};
		index = icode&((1 << len) - 1);
		icode >>= len;
		ilen -= len;
#endif

		node* x = tree_index[index];      // ¯®«ãç ¥¬ ã§¥«
		assert(x != NULL);

		memcpy(&obuf[o], tree_node2string(x), x->level); // ¤®¡ ¢«ï¥¬ áâà®ªã

		x->hint++;

		for (int j = 0; j<x->level; j++)    //  ¯¤¥©â¨¬ á«®¢ àì
		{
			if (o >= TREE_DEPTH - 1)
				if (nodecount<TREE_MAX)
					tree_update(&obuf[o - TREE_DEPTH + 1]);
			o++;

			if ((o%TREE_FILT_RANGE) == 0)
			{
				tree_filter();                // ç¨áâ¨¬ á«®¢ àì
				tree_enum();
#ifdef USE_HUFF
				huff_build();
#endif
			};

		};

	};

	showstat(osize, i, o);

	tree_done();
};