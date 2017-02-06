/*
��?� ?????�?�? ?��????� ?�??��???�?� �???? ???��� ???��� LZW- (??? ??�?????)
�???�?????�
- �???�?????? ??�?�????
- ? �?�?��?? �????�� ?�???�?�?��� ??�??? (�??�?���????-? ��? ??? ????�...)
- ???�???�?�??�? �?????�� �????�� �???�����
- �?????�?�? ���?�? ?????����� ????�??? �???? ??�???
- ????�? �???? �????���� �?��?????
- �???�??� ????????, ?�?� ??��??? ???��?
- ??�� ??�???? ��? ??�� �??� �?��????? ???????? ? ? ???��??��?? �?��???
  �?? ?�? ? ???��? ��?????���.
- ?�? ?�?��????? �?��???? �??� �????���� �?� ? �?�??-�? �?? LZW, ?? 8,9,
  10 ? �.?. ??� ?? ????�, ? �?� ?? �?��? �????��

                                      (c) 1999 Z0MBiE, http://z0mbie.cjb.net
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <io.h>
#include <time.h>

#undef USE_HUFF

#pragma extref _floatconvert

#define TREE_MAX        65536    // ??��. �??-?? �?-�?? ??�???
#define TREE_DEPTH      8        // ??��. ????????��� ??�???
#define TREE_HINT_COUNT 1        // ?????�?: 1 ���?�? ?? 8 �??., ???�?
#define TREE_HINT_RANGE 8192     //            �????�� ���?�� ?? ??�???
#define TREE_FILT_RANGE 1024     // �?�?? �??�? ???� ??????��� ??�???

//#define USE_HUFF                 // �????�� �??� �?��?????, ????�? ?���??

typedef unsigned char   byte;
typedef unsigned short  word;
typedef unsigned long   dword;

/*
#define min(a,b)        ((a)<(b)?(a):(b))
#define max(a,b)        ((a)>(b)?(a):(b))
*/

typedef struct _node             // �??? ??�???. ??�?�?��? �?-�� ?? ?�???�?�����
  {
    dword       count;          // �??�? �?? ���?�? ?���?�???�� ? �?���?
    dword       hint;           // �??�? ���?� ???????/�??�?
    dword       lastoffs;       // ??�?????? ?���?� ���?�?
    dword       level;          // ��????� ????????��? �??? = ????? ���?�?
    dword       sym;            // char, �?���?? �?????
    dword       index;          // ????� �??? ??�???
    struct _node        *prev;          // ?�??�?��?? �???
    dword       nextcount;      // �?�?? ???�???�� �?????�?? ? next
    struct _node*       (*next)[256];   // NULL ??? �???�??? ?? �???���?? �?-��
  } node;

node* root;                     // �?�??�
dword nodecount;                // �?�?? �???? ??? �?�?��? ?�?????? ???���
dword tree_indexcount;          // �?�?? �???? ??�???
node* tree_index[TREE_MAX];     // ??��?? �???�???? ?? ?�? �??� ??�???

node* tree_allocnode()          // ?�????�� �???
  {
    nodecount++;
    node* x = (node*)calloc(1,sizeof(node));
    assert(x!=NULL);
    return x;
  };

void tree_deallocnode(node** x) // �??�� �???
  {
    assert(*x!=NULL);
    if ((*x)->next!=NULL)
    {
      assert((*x)->nextcount==0);
      free((*x)->next);
    };
    free(*x);
    *x=NULL;
    nodecount--;
  };

node* tree_addnode(node** x, dword i)  // ??????�� �??? ?? �?? ���?��?���???
  {
    assert(*x!=NULL);
    if ((*x)->next==NULL) 
		(*x)->next=(struct _node*)calloc(256,4);
    (*x)->nextcount++;
    (*(*x)->next)[i] = tree_allocnode();
    (*(*x)->next)[i]->prev = *x;
    (*(*x)->next)[i]->sym = i;
    (*(*x)->next)[i]->level = (*x)->level+1;
    (*(*x)->next)[i]->index = tree_indexcount++;
    tree_index[ (*(*x)->next)[i]->index ] = (*(*x)->next)[i];
    return (*(*x)->next)[i];
  };

void tree_delnode(node** x)            // �????�� �??? ? ?�?� ??�?????�??
  {
    assert(*x!=NULL);
    if ((*x)->next!=NULL)
    for (int i=0; i<256; i++)
      if ((*(*x)->next)[i]!=NULL)
        tree_delnode(&(*(*x)->next)[i]);
    assert((*x)->nextcount==0);
    if ((*x)->prev!=NULL)
      (*x)->prev->nextcount--;
    tree_deallocnode(x);
  };

void tree_init()                       // ???�??????�???�� root
  {
    nodecount=0;
    root=tree_allocnode();
    tree_indexcount=0;
    memset(tree_index, 0, sizeof(tree_index));
  };

void tree_init1()                      // ???�??????�???�� ??�?�? ��????�
  {
    for (int i=0; i<256; i++)
      tree_addnode(&root,i);
  };

void tree_done()                       // ?�� ???��� ?�??????�� � �?�??
  {
    tree_delnode(&root);
    assert(nodecount==0);
  };

byte* tree_node2string(node* x)        // �??? --> ���?�?
  {
    assert(x!=NULL);
    static byte s[TREE_DEPTH+1];
    memset(s,0,sizeof(s));
    while (x->prev!=NULL)
    {
      s[x->level-1]=x->sym;
      x=x->prev;
    };
    return s;
  };

node* tree_string2node(byte* s)        // ���?�? --> �???
  {
    assert(s!=NULL);
    node* x = root;
    node* p;
    while (x!=NULL)
    {
      p=x;
      if (*x->next==NULL) x=NULL; else x=(*x->next)[*s++];
    };
    return p;
  };

dword tree_thisoffs;                   // �?���?? ?��?� ? �????

void tree_update(byte* s)              // ??????�� ���?�� ? ??�???
  {
    assert(s!=NULL);
    node** x = &root;
    for (int i=0; i<TREE_DEPTH; i++)
    {
      if (((*x)->next==NULL)||
          ((*(*x)->next)[*(s+i)]==NULL))
        tree_addnode(x,*(s+i));
      x=&(*(*x)->next)[*(s+i)];
      (*x)->count++;
      (*x)->lastoffs = tree_thisoffs;
    };
  };

void tree_filter_node(node** x)        // �??���???�� �??� ??�???� � ???????
  {
    assert(*x!=NULL);

    if (*x!=NULL)
    if ((*x)->next!=NULL)
    for (int i=0; i<256; i++)
      if ((*(*x)->next)[i]!=NULL)
        tree_filter_node(&(*(*x)->next)[i]);

    assert(*x!=NULL);

    if (((*x)->hint+1)*TREE_HINT_RANGE<
        (tree_thisoffs-(*x)->lastoffs)*TREE_HINT_COUNT)
    {
//    printf("kill: hint=%i range=%i [%s]\n",(*x)->hint, tree_thisoffs-(*x)->lastoffs,tree_node2string(*x));
      tree_delnode(x);
    };
  };

void tree_filter()                     // �??���???�� ??�???
  {
    if (root->next!=NULL)
    for (int i=0; i<256; i++)
      if ((*root->next)[i]!=NULL)
      if ((*root->next)[i]->next!=NULL)
      for (int j=0; j<256; j++)
        if ((*(*root->next)[i]->next)[j]!=NULL)
          tree_filter_node(&(*(*root->next)[i]->next)[j]);
  };

void tree_enum_node(node* x)    // ??�??�??�???�� ????���(????�?) �????
  {
    assert(x!=NULL);
    x->index=tree_indexcount++;
    tree_index[x->index]=x;
    if (x->next!=NULL)
      for (int i=0;i<256;i++)
        if ((*x->next)[i]!=NULL)
          tree_enum_node((*x->next)[i]);
  };

void tree_enum()                // ??�??�??�???�� ?�? ????���
  {
    tree_indexcount=0;
    memset(tree_index, 0, sizeof(tree_index));
    tree_enum_node(root);
    assert(tree_indexcount==nodecount);
  };

void tree_dump_node(node* x)    // ???? �???
  {
    assert(x!=NULL);
    if (x->prev==NULL)
      printf("[ROOT]\n");
    else
      printf("[%s] hint=%i lastoffs=%i level=%i index=%i nextcount=%i\n",
        tree_node2string(x),(int)x->hint,(int)x->lastoffs,(int)x->level,(int)x->index,(int)x->nextcount);
    if (x->next!=NULL)
    for (int i=0; i<256; i++)
      if ((*x->next)[i]!=NULL)
        tree_dump_node((*x->next)[i]);
  };

void tree_dump()                // ???? ??�???
  {
    tree_dump_node(root);
  };

#ifdef USE_HUFF

/* ?�? ??� �?��?????�??? ??�??? */
/* ????��?�?� ?�??????�?�� �???? �???????� � ?�????�? ??�???? */

dword huff_max0;                // ??�??�??? �?�?? �????
dword huff_count[TREE_MAX*2];   // �?�?? ???�?�???? �???
dword huff_next [TREE_MAX*2];   // �???���?? �?-�
dword huff_prev0[TREE_MAX*2];   // ?�??�?��??-???�? �?-�
dword huff_prev1[TREE_MAX*2];   // ?�??�?��??-?�??�? �?-�
dword huff_bit  [TREE_MAX*2];   // ??� (0=???�?, 1=?�??�?)
dword huff_max;                 // �?���?? �?�?? �?-�? ??�???
dword huff_min;                 // �?���?? ???�???? ???????�???? huff_count[i]
dword huff_left;                // ????� ��?????? ?????? ??�??????????? �?-�?

void huff_init()
  {
  };

dword huff_findmin()            // ???� �?-�? � ???????�?�? ???�?????
  {
    dword mi=0xFFFFFFFF;
    dword mv=0xFFFFFFFF;
    while (huff_next[huff_left]!=0) huff_left++;
    for (dword i=huff_left; i<huff_max; i++)    // 0=root -- not used
      if (huff_next[i]==0)
      if (huff_count[i]<mv)
      {
        mi=i;
        mv=huff_count[i];
        if (mv<=huff_min) break;
      };
    huff_min=mv;
    return mi;
  };

void huff_build()               // ����ந�� ��䬠���᪮� ��ॢ�
  {
    huff_max0 = tree_indexcount;
    huff_max = tree_indexcount;
    for (dword i=0; i<tree_indexcount; i++)
      huff_count[i] = tree_index[i]->count * tree_index[i]->level;

    memset(huff_next,0,sizeof(huff_next));
    huff_min = 0;
    huff_left = 1;

    for (;;)
    {
      dword a = huff_findmin();
      huff_next[a] = huff_max;
      assert(a!=0xFFFFFFFF);
      dword b = huff_findmin();
      if (b==0xFFFFFFFF)
      {
        huff_next[a]=0;
        assert(a==huff_max-1);
        break;
      };
      huff_next[b] = huff_max;
      huff_prev0[huff_max] = a;
      huff_prev1[huff_max] = b;
      huff_count[huff_max] = huff_count[a]+huff_count[b];
      huff_bit[a] = 0;
      huff_bit[b] = 1;
      huff_max++;
    };
  };

#endif

void showstat(dword max, dword i, dword o)
  {
    byte s[41];
    s[40]=0;
    dword ti=i*40/max;
    dword to=o*40/max;
    if (ti<to) { ti^=to;to^=ti;ti^=to; };
    for (int j=0; j<40; j++)
    {
      s[j]=0xB0;
      if (j<=ti) s[j]=0xB1;
      if (j<=to) s[j]=0xB2;
    };
    printf("[%s] %5.1f%% %6.2fx %5i-->%5i %6i#\x0D",
      s,(float)max(i,o)*100/max,(float)o/(i+1),  (int)i,(int)o,(int)nodecount);
  };

inline dword ln2(dword x)
  {
    dword y=8;
    if (x>=  256) y++;
    if (x>=  512) y++;
    if (x>= 1024) y++;
    if (x>= 2048) y++;
    if (x>= 4096) y++;
    if (x>= 8192) y++;
    if (x>=16384) y++;
    if (x>=32768) y++;
    return y;
  };

/* ���������� ���� */

void encode_buf(byte ibuf[], dword isize, byte obuf[], dword *osize)
  {
    assert(ibuf!=NULL);
    assert(obuf!=NULL);

    tree_init();
    tree_init1();
    tree_enum();

#ifdef USE_HUFF
    huff_init();
    huff_build();
#endif

    *osize=0;
    dword ocode=0, olen=0;
    dword i,c;
    for (i=0; i<isize; )
    {
      if (clock()-c>CLK_TCK/4)
      {
        c=clock();
        showstat(isize,i,*osize);
      };

      tree_thisoffs = i;                // ⥪�騩 ����
      node *x;
      if (i<isize-TREE_DEPTH+1)
        x = tree_string2node(&ibuf[i]); // ᤥ���� �� ��ப� ���
      else
        x = (*root->next)[ibuf[i]];
      assert(x!=NULL);

#ifdef USE_HUFF
      while (x->index >= huff_max0) x=x->prev;
      assert(x!=NULL);
#endif

      dword code,len;

#ifdef USE_HUFF
      dword index=x->index;             // ᦠ�� ��� ��䬠���
      code=len=0;
      while (huff_next[index]!=0)
      {
        code<<=1;
        code|=huff_bit[index];
        len++;
        index=huff_next[index];
      };
      
#else
      code=x->index;
      len=ln2(tree_indexcount);
#endif

      ocode|=code<<olen;                // �������� � ��室��� ��⮪
      olen+=len;
      while (olen>=8)
      {
        obuf[(*osize)++]=ocode&255;
        ocode>>=8;
        olen-=8;
      };

      x->hint++;

      for (int j=0; j<x->level; j++)    // �����⨬ ᫮����
      {
        if (i>=TREE_DEPTH-1)
          if (nodecount<TREE_MAX)
            tree_update(&ibuf[i-TREE_DEPTH+1]);
        i++;

        if ((i%TREE_FILT_RANGE)==0)
        {
          tree_filter();                // ��⨬ ᫮����
          tree_enum();
#ifdef USE_HUFF
          huff_build();
#endif
        };

      };

    };
    if (olen!=0) obuf[(*osize)++]=ocode;

    showstat(isize,i,*osize);

    tree_done();
  };

/* �ᯠ������ ���� */

void decode_buf(byte ibuf[], dword isize, byte obuf[], dword osize)
  {
    assert(ibuf!=NULL);
    assert(obuf!=NULL);

    tree_init();
    tree_init1();
    tree_enum();

#ifdef USE_HUFF
    huff_init();
    huff_build();
#endif

    dword i=0, icode=0, ilen=0;
    dword o,c;

    for (o=0; (o<osize)&&(i<isize); )
    {
      if (clock()-c>CLK_TCK/4)
      {
        c=clock();
        showstat(osize,i,o);
      };

      tree_thisoffs = o;

      dword index;

#ifdef USE_HUFF
      index=huff_max-1;                 // �⠥� �� ��⠬,
      do                                // ���� �� ��䬠���᪮�� ��ॢ�,
      {                       // ����砥� ������ ��ப� � �᭮���� ��ॢ�
        if (ilen==0)
        {
          icode|=ibuf[i++]<<ilen;
          ilen+=8;
        };
        if ((icode&1)==0)
          index=huff_prev0[index];
        else
          index=huff_prev1[index];
        icode>>=1;
        ilen--;
      } while (index >= huff_max0);
#else
      dword len=ln2(tree_indexcount);   // ���� ���� �⠥� ������
      while (ilen<len)
      {
        icode|=ibuf[i++]<<ilen;
        ilen+=8;
      };
      index=icode&((1<<len)-1);
      icode>>=len;
      ilen-=len;
#endif

      node* x = tree_index[index];      // ����砥� 㧥�
      assert(x!=NULL);

      memcpy(&obuf[o],tree_node2string(x),x->level); // ������塞 ��ப�

      x->hint++;

      for (int j=0; j<x->level; j++)    // �����⨬ ᫮����
      {
        if (o>=TREE_DEPTH-1)
          if (nodecount<TREE_MAX)
            tree_update(&obuf[o-TREE_DEPTH+1]);
        o++;

        if ((o%TREE_FILT_RANGE)==0)
        {
          tree_filter();                // ��⨬ ᫮����
          tree_enum();
#ifdef USE_HUFF
          huff_build();
#endif
        };

      };

    };

    showstat(osize,i,o);

    tree_done();
  };


