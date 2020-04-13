// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "org_kucro3_XADMA_XADMA.h"

#include "dllmain.h"

#include <map>
#include <mutex>
#include <string>

//
std::map<std::wstring, XADMA_REGION_CONTEXT> region_contexts;

std::mutex region_contexts_lock;

int system_endian;

//

VOID __InitializeEndian()
{
    INT16 tR = 0x0001;

    if (*((PINT8)&tR + 1)) // big-endian
        system_endian = XADMA_REGION_BIG_ENDIAN;
    else
        system_endian = XADMA_REGION_LITTLE_ENDIAN;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        __InitializeEndian();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// String util
FORCEINLINE LPWSTR __CopyOfWideString(LPCWSTR string)
{
    LPWSTR copy = (LPWSTR)malloc(sizeof(WCHAR) * (wcslen(string) + 1));

    lstrcpyW(copy, string);

    return copy;
}

FORCEINLINE VOID __FreeCopyOfWideString(LPWSTR string)
{
    free(string);
}

class jni_string_guard {
public:
    jni_string_guard(JNIEnv* env, jstring string) : env(env), string(string)
    {
        wstr = env->GetStringChars(string, NULL);
    }

    ~jni_string_guard()
    {
        env->ReleaseStringChars(string, wstr);
    }

    operator LPCWSTR ()
    {
        return (LPCWSTR) wstr;
    }

    operator const jchar* ()
    {
        return wstr;
    }

private:
    JNIEnv* env;

    jstring string;

    const jchar* wstr;
};

//

DLLEXPORT BOOL XADMACheckRegionHeader(
    PXADMA_REGION_HEADER pHeader
    )
{
    if (pHeader->magicValue != XADMA_REGION_MAGIC_VALUE)
        return FALSE;

    if (pHeader->version != XADMA_REGION_HEADER_VERSION)
        return FALSE;

    if (pHeader->referenceCount <= 0) // already released
        return FALSE;

    return TRUE;
}

DLLEXPORT VOID XADMAInitializeRegionHeader(
    PXADMA_REGION_HEADER pHeader,
    BOOL reference = 0
    )
{
    pHeader->magicValue = XADMA_REGION_MAGIC_VALUE;
    pHeader->version = XADMA_REGION_HEADER_VERSION;
    pHeader->referenceCount = reference ? 1 : 0;
}

DLLEXPORT UINT XADMAGetRegionHeaderReservation()
{
    return XADMA_REGION_HEADER_RESERVATION;
}

DLLEXPORT UINT XADMAGetRegionHeaderVersion()
{
    return XADMA_REGION_HEADER_VERSION;
}

DLLEXPORT LPVOID XADMAQueryRegion(
    LPCWSTR name, 
    BOOL reference,
    PXADMA_REGION_CONTEXT pRegionContext = NULL)
{
    std::lock_guard<std::mutex> _lock(region_contexts_lock);

    // query from local

    std::map<std::wstring, XADMA_REGION_CONTEXT>::iterator iter
        = region_contexts.find(std::wstring(name));

    if (iter != region_contexts.end())
    {
        if (pRegionContext)
            *pRegionContext = iter->second;

        if (reference)
            (iter->second.RegionHeader->referenceCount)++;

        return iter->second.BaseAddress;
    }

    // query from global

    HANDLE handle = OpenFileMappingW(XADMA_REGION_PAGE_PROTECTION, FALSE, name);

    if (handle)
    {
        LPVOID region = MapViewOfFile(handle, XADMA_REGION_ACCESS, 0, 0, 0);

        if (region)
        {
            // check header
            PXADMA_REGION_HEADER pHeader = (PXADMA_REGION_HEADER)region;

            if (!XADMACheckRegionHeader(pHeader))
            {
                CloseHandle(handle);

                return NULL;
            }

            if (reference)
                (pHeader->referenceCount)++;

            XADMA_REGION_CONTEXT context;

            // register
            region_contexts[std::wstring(name)] = context = XADMA_REGION_CONTEXT{
                handle,
                region,
                pHeader,
                LPVOID_SHIFT(region, XADMA_REGION_HEADER_RESERVATION),
                __CopyOfWideString(name)
            };

            if (pRegionContext)
                *pRegionContext = context;

            return region;
        }
        else
            CloseHandle(handle);
    }

    return NULL;
}

FORCEINLINE LPVOID __XADMAAcquireRegion(LPCWSTR name, DWORD size, PXADMA_REGION_CONTEXT pRegionContext)
{
    HANDLE handle = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, XADMA_REGION_PAGE_PROTECTION, 0, size, name);

    if (handle)
    {
        LPVOID region = MapViewOfFile(handle, XADMA_REGION_ACCESS, 0, 0, 0);

        if (region)
        {
            XADMAInitializeRegionHeader((PXADMA_REGION_HEADER)region, TRUE);

            XADMA_REGION_CONTEXT context;

            // register
            region_contexts[std::wstring(name)] = context = XADMA_REGION_CONTEXT {
                handle,
                region,
                (PXADMA_REGION_HEADER)region,
                LPVOID_SHIFT(region, XADMA_REGION_HEADER_RESERVATION),
                __CopyOfWideString(name)
            };

            if (pRegionContext)
                *pRegionContext = context;

            return region;
        }
        else
            CloseHandle(handle);
    }

    return NULL;
}

DLLEXPORT LPVOID XADMAAllocateRegion(
    LPCWSTR name,
    DWORD size, 
    PXADMA_REGION_CONTEXT pRegionContext = NULL)
{
    std::lock_guard<std::mutex> _lock(region_contexts_lock);

    // query from local

    if (region_contexts.find(std::wstring(name)) != region_contexts.end())
        return NULL;

    // query from global

    HANDLE handle = OpenFileMappingW(XADMA_REGION_ACCESS, FALSE, name);

    if (handle)
    {
        CloseHandle(handle);

        return NULL;
    }

    // allocate

    return __XADMAAcquireRegion(name, size, pRegionContext);
}

DLLEXPORT LPVOID XADMAQueryOrAllocateRegion(
    LPCWSTR name, 
    DWORD expectedSize, 
    PXADMA_REGION_CONTEXT pRegionContext = NULL)
{
    LPVOID region = XADMAQueryRegion(name, TRUE, pRegionContext);

    if (region)
        return region;

    // allocate

    std::lock_guard<std::mutex> _lock(region_contexts_lock);

    return __XADMAAcquireRegion(name, expectedSize, pRegionContext);
}

DLLEXPORT UINT XADMAReleaseRegion(
    PXADMA_REGION_CONTEXT pRegionContext
    )
{
    std::lock_guard<std::mutex> _lock(region_contexts_lock);

    PXADMA_REGION_HEADER pHeader = pRegionContext->RegionHeader;

    if (pHeader->referenceCount <= 0)
        return XADMA_REGION_NOT_VALID;

    (pHeader->referenceCount)--;

    if (pHeader->referenceCount == 0)
    {
        UnmapViewOfFile(pRegionContext->BaseAddress);
        CloseHandle(pRegionContext->MappingHandle);

        region_contexts.erase(std::wstring(pRegionContext->Name));

        __FreeCopyOfWideString((LPWSTR)pRegionContext->Name);

        return XADMA_REGION_DESTROYED;
    }

    return XADMA_REGION_REFERENCE_COUNT_REDUCED;
}


// JNI support
extern "C"
{
    // long org.kucro3.XADMA.XADMA.querySize0(long)
    JNIEXPORT jlong JNICALL Java_org_kucro3_XADMA_XADMA_querySize0
    (   
        JNIEnv* env,
        jclass clazz,
        jlong handle
        )
    {
        MEMORY_BASIC_INFORMATION memInfo = { 0 };

        if (!VirtualQuery(((PXADMA_REGION_CONTEXT) handle)->BaseAddress, &memInfo, sizeof(MEMORY_BASIC_INFORMATION)))
            return (jlong) 0;

        return (jlong)(memInfo.RegionSize - XADMA_REGION_HEADER_RESERVATION);
    }

    // long org.kucro3.XADMA.XADMA.require0(String)
    JNIEXPORT jlong JNICALL Java_org_kucro3_XADMA_XADMA_require0
    (
        JNIEnv* env,
        jclass clazz,
        jstring name
        )
    {
        jni_string_guard jName(env, name);

        PXADMA_REGION_CONTEXT pRegionContext
            = (PXADMA_REGION_CONTEXT)malloc(sizeof(XADMA_REGION_CONTEXT));

        if (!XADMAQueryRegion(jName, TRUE, pRegionContext))
            return NULL;

        return (jlong)pRegionContext;
    }

    // long org.kucro3.XADMA.XADMA.acquire0(String, int)
    JNIEXPORT jlong JNICALL Java_org_kucro3_XADMA_XADMA_acquire0
    (
        JNIEnv* env,
        jclass clazz,
        jstring name,
        jint expectedSize
        )
    {
        jni_string_guard jName(env, name);

        PXADMA_REGION_CONTEXT pRegionContext
            = (PXADMA_REGION_CONTEXT)malloc(sizeof(XADMA_REGION_CONTEXT));

        if (!XADMAQueryOrAllocateRegion(jName, expectedSize, pRegionContext))
            return NULL;

        return (jlong)pRegionContext;
    }

    // long org.kucro3.XADMA.XADMA.allocate0(String, int)
    JNIEXPORT jlong JNICALL Java_org_kucro3_XADMA_XADMA_allocate0
    (
        JNIEnv* env,
        jclass clazz,
        jstring name,
        jint size
        )
    {
        jni_string_guard jName(env, name);

        PXADMA_REGION_CONTEXT pRegionContext
            = (PXADMA_REGION_CONTEXT)malloc(sizeof(XADMA_REGION_CONTEXT));

        if (!XADMAAllocateRegion(jName, size, pRegionContext))
            return NULL;

        return (jlong)pRegionContext;
    }

    // long org.kucro3.XADMA.XADMA.release0(long)
    JNIEXPORT jint JNICALL Java_org_kucro3_XADMA_XADMA_release0
    (
        JNIEnv* env,
        jclass clazz,
        jlong handle
        )
    {
        PXADMA_REGION_CONTEXT pRegionContext = (PXADMA_REGION_CONTEXT)handle;

        switch (XADMAReleaseRegion(pRegionContext))
        {
        case XADMA_REGION_NOT_VALID:
            return FALSE;

        case XADMA_REGION_DESTROYED:
            free(pRegionContext);

        case XADMA_REGION_REFERENCE_COUNT_REDUCED:
            return TRUE;
        }

        return FALSE;
    }


#define XADMA_REGION_ASSIGN(context, type, offset) \
    *(type)((PINT8)((PXADMA_REGION_CONTEXT)context)->RegionBaseAddress + offset)

    // void org.kucro3.XADMA.XADMA.putByte0(long, long, byte)
    JNIEXPORT VOID JNICALL Java_org_kucro3_XADMA_XADMA_putByte0
    (
        JNIEnv* env,
        jclass clazz,
        jlong handle,
        jlong offset,
        jbyte value
        )
    {
        XADMA_REGION_ASSIGN(handle, PINT8, offset) = value;
    }

    // byte org.kucro3.XADMA.XADMA.getByte0(long, long)
    JNIEXPORT jbyte JNICALL Java_org_kucro3_XADMA_XADMA_getByte0
    (
        JNIEnv* env,
        jclass clazz,
        jlong handle,
        jlong offset
        )
    {
        return XADMA_REGION_ASSIGN(handle, PINT8, offset);
    }

    // void org.kucro3.XADMA.XADMA.putShort0(long, int, long, short)
    JNIEXPORT VOID JNICALL Java_org_kucro3_XADMA_XADMA_putShort0
    (
        JNIEnv* env,
        jclass clazz,
        jlong handle,
        jint endian,
        jlong offset,
        jshort value
        )
    {
        if (endian == system_endian)
            XADMA_REGION_ASSIGN(handle, PINT16, offset) = value;
        else // reverse
            XADMA_REGION_ASSIGN(handle, PINT16, offset) = 
                ((value & 0x00FF) << 8)
                | ((value & 0xFF00) >> 8);
    }

    // short org.kucro3.XADMA.XADMA.getShort0(long, int, long)
    JNIEXPORT jshort JNICALL Java_org_kucro3_XADMA_XADMA_getShort0
    (
        JNIEnv* env,
        jclass clazz,
        jlong handle,
        jint endian,
        jlong offset
        )
    {
        if (endian == system_endian)
            return XADMA_REGION_ASSIGN(handle, PINT16, offset);
        else
        {  // reverse
            INT16 value = XADMA_REGION_ASSIGN(handle, PINT16, offset);

            return ((value & 0x00FF) << 8)
                | ((value & 0xFF00) >> 8);
        }
    }


    FORCEINLINE VOID __PutInt32
    (
        jlong handle,
        jint endian,
        jlong offset,
        INT32 value
        )
    {
        if (endian == system_endian)
            XADMA_REGION_ASSIGN(handle, PINT32, offset) = value;
        else // reverse
            XADMA_REGION_ASSIGN(handle, PINT32, offset) =
            ((value & 0x000000FF) << 24)
            | ((value & 0x0000FF00) << 8)
            | ((value & 0x00FF0000) >> 8)
            | ((value & 0xFF000000) >> 24);
    }

    FORCEINLINE INT32 __GetInt32(
        jlong handle,
        jint endian,
        jlong offset
        )
    {
        if (endian == system_endian)
            return XADMA_REGION_ASSIGN(handle, PINT32, offset);
        else
        {  // reverse
            INT32 value = XADMA_REGION_ASSIGN(handle, PINT32, offset);

            return ((value & 0x000000FF) << 24)
                | ((value & 0x0000FF00) << 8)
                | ((value & 0x00FF0000) >> 8)
                | ((value & 0xFF000000) >> 24);
        }
    }


    FORCEINLINE VOID __PutInt64(
        jlong handle,
        jint endian,
        jlong offset,
        INT64 value
        )
    {
        if (endian == system_endian)
            XADMA_REGION_ASSIGN(handle, PINT64, offset) = value;
        else // reverse
            XADMA_REGION_ASSIGN(handle, PINT64, offset) =
            ((value & 0x00000000000000FFL) << 56)
            | ((value & 0x000000000000FF00L) << 40)
            | ((value & 0x0000000000FF0000L) << 24)
            | ((value & 0x00000000FF000000L) << 8)
            | ((value & 0x000000FF00000000L) >> 8)
            | ((value & 0x0000FF0000000000L) >> 24)
            | ((value & 0x00FF000000000000L) >> 40)
            | ((value & 0xFF00000000000000L) >> 56);
    }

    FORCEINLINE INT64 __GetInt64(
        jlong handle,
        jint endian,
        jlong offset
        )
    {
        if (endian == system_endian)
            return XADMA_REGION_ASSIGN(handle, PINT64, offset);
        else
        {
            INT64 value = XADMA_REGION_ASSIGN(handle, PINT64, offset);

            return ((value & 0x00000000000000FFL) << 56)
                | ((value & 0x000000000000FF00L) << 40)
                | ((value & 0x0000000000FF0000L) << 24)
                | ((value & 0x00000000FF000000L) << 8)
                | ((value & 0x000000FF00000000L) >> 8)
                | ((value & 0x0000FF0000000000L) >> 24)
                | ((value & 0x00FF000000000000L) >> 40)
                | ((value & 0xFF00000000000000L) >> 56);
        }
    }


    JNIEXPORT VOID JNICALL Java_org_kucro3_XADMA_XADMA_putInt0
    (
        JNIEnv* env,
        jclass clazz,
        jlong handle,
        jint endian,
        jlong offset,
        jint value
        )
    {
        __PutInt32(handle, endian, offset, value);
    }

    // int org.kucro3.XADMA.XADMA.getInt0(long, int, long, int)
    JNIEXPORT jint JNICALL Java_org_kucro3_XADMA_XADMA_getInt0
    (
        JNIEnv* env,
        jclass clazz,
        jlong handle,
        jint endian,
        jlong offset
        )
    {
        return __GetInt32(handle, endian, offset);
    }

    // void org.kucro3.XADMA.XADMA.putLong0(long, int, long, long)
    JNIEXPORT VOID JNICALL Java_org_kucro3_XADMA_XADMA_putLong0
    (
        JNIEnv* env,
        jclass clazz,
        jlong handle,
        jint endian,
        jlong offset,
        jlong value
        )
    {
        __PutInt64(handle, endian, offset, value);
    }

    // long org.kucro3.XADMA.XADMA.getLong0(long, int, long, long)
    JNIEXPORT jlong JNICALL Java_org_kucro3_XADMA_XADMA_getLong0
    (
        JNIEnv* env,
        jclass clazz,
        jlong handle,
        jint endian,
        jlong offset
        )
    {
        return __GetInt64(handle, endian, offset);
    }

    // void org.kucro3.XADMA.XADMA.putFloat0(long, int, long, float)
    JNIEXPORT VOID JNICALL Java_org_kucro3_XADMA_XADMA_putFloat0
    (
        JNIEnv* env,
        jclass clazz,
        jlong handle,
        jint endian,
        jlong offset,
        jfloat value
        )
    {
        __PutInt32(handle, endian, offset, *(PINT32)&value);
    }

    // float org.kucro3.XADMA.XADMA.getFloat0(long, int, long)
    JNIEXPORT jfloat JNICALL Java_org_kucro3_XADMA_XADMA_getFloat0
    (
        JNIEnv* env,
        jclass clazz,
        jlong handle,
        jint endian,
        jlong offset
        )
    {
        INT32 value = __GetInt32(handle, endian, offset);

        return *(jfloat*)&value;
    }

    // void org.kucro3.XADMA.XADMA.putDouble0(long, int, long, double)
    JNIEXPORT VOID JNICALL Java_org_kucro3_XADMA_XADMA_putDouble0
    (
        JNIEnv* env,
        jclass clazz,
        jlong handle,
        jint endian,
        jlong offset,
        jdouble value)
    {
        __PutInt64(handle, endian, offset, *(PINT64)&value);
    }

    // double org.kucro3.XADMA.XADMA.getDouble0(long, int, long)
    JNIEXPORT jdouble JNICALL Java_org_kucro3_XADMA_XADMA_getDouble0
    (
        JNIEnv* env,
        jclass clazz,
        jlong handle,
        jint endian,
        jlong offset
        )
    {
        INT64 value = __GetInt64(handle, endian, offset);

        return *(jdouble*)&value;
    }
    
    // int org.kucro3.XADMA.XADMA.getSystemEndian0()
    JNIEXPORT jint JNICALL Java_org_kucro3_XADMA_XADMA_getSystemEndian0
    (
        JNIEnv* env,
        jclass clazz
        )
    {
        return system_endian;
    }
}