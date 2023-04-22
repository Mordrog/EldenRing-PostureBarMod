#include "Common.hpp"
/// <summary>
/// Clear Bytes
/// </summary>
/// <param name="dst">Address</param>
/// <param name="size">Size</param>
void Nop(BYTE* dst, unsigned int size)
{
	DWORD oldprotect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
	memset(dst, 0x90, size);
	VirtualProtect(dst, size, oldprotect, &oldprotect);
}

/// <summary>
/// Patch Bytes
/// </summary>
/// <param name="dst">Address</param>
/// <param name="src">Value</param>
/// <param name="size">Size</param>
void Patch(BYTE* dst, BYTE* src, unsigned int size)
{
	DWORD oldprotect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
	memcpy(dst, src, size);
	VirtualProtect(dst, size, oldprotect, &oldprotect);
}