static UINT (WINAPI *mySendInput)(UINT cInputs, LPINPUT pInputs, int cbSize);
static BOOL  (WINAPI *myEmptyWorkingSet)(HANDLE hProcess);

UINT SendInputL(UINT cInputs, LPINPUT pInputs, int cbSize)
{
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1: /* First time */
        mySendInput=(void *)GetProcAddress(GetModuleHandleA("USER32.DLL"), "SendInput");
        if(!mySendInput) {
            have_func=0;
            break;
        } else {
            have_func=1;
        }
    case 1: /* We know we have the function */
        return mySendInput(cInputs, pInputs, cbSize);
    case 0:
    default: break;
    }
    UINT i;
    for(i=0; i< cInputs; i++){
         if (pInputs[i].type == INPUT_KEYBOARD){
             keybd_event(pInputs[i].ki.wVk, 0, 0, 0);
             keybd_event(pInputs[i].ki.wVk, 0, KEYEVENTF_KEYUP, 0);
         }else if(pInputs[i].type == INPUT_MOUSE){
             /* mouse_event(...); */
         }else if(pInputs[i].type == INPUT_HARDWARE){
             /* TODO */
         }
    }
    return i;
}
#define SendInput SendInputL

/* KERNEL32.DLL */
static BOOL (WINAPI *myIsWow64Process) (HANDLE hProcess, PBOOL Wow64Process);
BOOL IsWow64ProcessL(HANDLE hProcess, PBOOL Wow64Process)
{
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1: /* First time */
        myIsWow64Process = (void *)GetProcAddress(GetModuleHandleA("KERNEL32.DLL"), "IsWow64Process");
        if(!myIsWow64Process) {
            have_func=0;
            break;
        } else {
            have_func=1;
        }
    case 1: /* We know we have the function */
        return myIsWow64Process(hProcess, Wow64Process);
    case 0:
    default: break;
    }
    return FALSE;
}
#define IsWow64Process IsWow64ProcessL


BOOL EmptyWorkingSetL(HANDLE hProcess)
{
    static HINSTANCE hPSAPIdll=NULL;
    static char have_func=HAVE_FUNC;

    switch(have_func){
    case -1:
        hPSAPIdll = LoadLibraryA("PSAPI.DLL");
        if(!hPSAPIdll) {
            have_func = 0;
            break;
        } else {
            myEmptyWorkingSet=(void *)GetProcAddress(hPSAPIdll, "EmptyWorkingSet");
            if(myEmptyWorkingSet){
                have_func = 1;
            } else {
                FreeLibrary(hPSAPIdll);
                hPSAPIdll = NULL;
                have_func = 0;
                break;
            }
        }
    case 1:
        return myEmptyWorkingSet(hProcess);
    case 0:
    default: break;
    }
    return FALSE;
}
#define EmptyWorkingSet EmptyWorkingSetL
