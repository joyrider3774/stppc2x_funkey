#define scalloc(A, B)    smalloc((A)*(B))


void * UpperMalloc(size_t size);
void UpperFree(void* mem);
int GetUpperSize(void* mem);
void InitMemPool();
void DestroyMemPool();
