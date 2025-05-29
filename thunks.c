
#include <windows.h>

enum {
	TOT_THUNK_SIZE=4096,
#if defined(_M_AMD64) || defined(WIN64)
	THUNK_SIZE=22,
#elif defined(_M_IX86)
	THUNK_SIZE=14, // 13 aligned to USHORT
#else
	#error Unsupported processor type, determine THUNK_SIZE here...
#endif
	MAX_THUNKS=TOT_THUNK_SIZE/THUNK_SIZE,
};

static BYTE *thunks_array = NULL;
static WORD numthunks = 0;
static WORD free_idx = 0; // Free index mem;

static void *AllocThunk(LONG_PTR tProc, void *vParam)
{
	if (!thunks_array) {
		thunks_array = (BYTE*)VirtualAlloc( NULL, TOT_THUNK_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
		if (!thunks_array)
			return NULL;

		free_idx = 0;
		for (WORD i=0; i < MAX_THUNKS; i++)
			*(WORD*)&thunks_array[i*THUNK_SIZE] = i+1;
	}

	if (numthunks >= MAX_THUNKS) {
		return NULL;
	}
	++numthunks;

	// get the free thunk
	BYTE *thunk = &thunks_array[free_idx * THUNK_SIZE];
	free_idx = *(WORD*)thunk; // Pick up the free index

	DWORD oldprotect;
	VirtualProtect(thunks_array, TOT_THUNK_SIZE, PAGE_EXECUTE_READWRITE, &oldprotect);
	// x86
	//   | mov dword ptr [esp+4] this
	//   | jmp MainProc
	// AMD64
	//   | mov rcx this
	//   | mov rax MainProc
	//   | jmp rax
	//
	#if defined(_M_AMD64) || defined(WIN64)
		*(WORD*)   (thunk + 0) = 0xb948;
		*(void**)  (thunk + 2) = vParam;
		*(WORD*)   (thunk +10) = 0xb848;
		*(void**)  (thunk +12) = tProc;
		*(WORD*)   (thunk +20) = 0xe0ff;
	#elif defined(_M_IX86)
		*(DWORD*)  (thunk +0) = 0x042444C7;
		*(void**)  (thunk +4) = vParam;
		*(BYTE*)   (thunk +8) = 0xE9;
		*(DWORD *) (thunk+9)  = (BYTE*)((BYTE*)tProc)-(thunk+13);
	#else
		#error Unsupported processor type, please implement assembly code...
	#endif

	// Make thuk execute only for safety.
	VirtualProtect(thunks_array, TOT_THUNK_SIZE, PAGE_EXECUTE, &oldprotect);
	FlushInstructionCache( GetCurrentProcess(), thunks_array, TOT_THUNK_SIZE );

	return thunk;
}
static void ReleaseThunk(BYTE *thunk)
{
	if (!thunk || thunks_array + TOT_THUNK_SIZE < thunk  || thunk < thunks_array )
		return;

	DWORD oldprotect;
	VirtualProtect(thunks_array, TOT_THUNK_SIZE, PAGE_EXECUTE_READWRITE, &oldprotect);

	*(WORD*)thunk = free_idx; // Set free index in the current thunk location
	free_idx = (WORD)( (thunk - thunks_array) / THUNK_SIZE );
	--numthunks;

	VirtualProtect(thunks_array, TOT_THUNK_SIZE, PAGE_EXECUTE_READ, &oldprotect);

	// Everything is free!
	if (numthunks == 0) {
		VirtualFree( thunks_array, 0, MEM_RELEASE );
		thunks_array = NULL;
	}
}
#ifdef TEST_MAIN
#include <stdio.h>
int main(void)
{
	enum { LEN = 1024 };
	BYTE *t[LEN];
	for (int i =0; i < LEN; i++)
	{
		t[i] = (BYTE*)AllocThunk((LONG_PTR)333, NULL);
		if (t[i])
			printf("t[%d] = %lx\n", i, (DWORD)t[i]);
	}

	for (int i = LEN-1; i >=0; i--)
	{
		size_t prevfreeidx = free_idx;
		ReleaseThunk(t[i]);
		if (t[i])
			printf("released t[%d] = %lx  free_idx=%d / prev=%d\n", i, (DWORD)t[i], (int)free_idx, prevfreeidx);
		if(i%3==0)
			t[i] = (BYTE*)AllocThunk((LONG_PTR)444, NULL);
	}

	return 0;
}
#endif
