#include "windows.h" // Includes Windows.h

EXTERN_C BOOL WINAPI IsDebuggerPresentProxy()
{
  HMODULE kernel32 = LoadLibraryA("KERNEL32.DLL");

  if (kernel32)
  {
    typedef BOOL (WINAPI *IsDebuggerPresent_t)();

    IsDebuggerPresent_t is_debugger_present = reinterpret_cast<IsDebuggerPresent_t>(
      GetProcAddress(kernel32, "IsDebuggerPresent")
    );

    BOOL debugger_present = FALSE;

    // KERNEL32.IsDebuggerPresent is missing on Windows 95
    if (is_debugger_present)
    {
      debugger_present = is_debugger_present();
    }

    FreeLibrary(kernel32);

    return debugger_present;
  }

  return FALSE;
}