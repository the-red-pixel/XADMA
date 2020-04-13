#pragma once

#include "pch.h"

#define DLLEXPORT __declspec(dllexport)

// XADMA constants ====================================================


#define XADMA_REGION_MAGIC_VALUE            0x0AD0ACFE
#define XADMA_REGION_HEADER_VERSION         0x0000000C

#define XADMA_REGION_HEADER_RESERVATION     512

#define XADMA_REGION_PAGE_PROTECTION        PAGE_READWRITE | SEC_COMMIT
#define XADMA_REGION_ACCESS                 FILE_MAP_READ | FILE_MAP_WRITE

#define XADMA_REGION_BIG_ENDIAN             0
#define XADMA_REGION_LITTLE_ENDIAN          1

#ifdef _WIN64
#define LPVOID_SHIFT(base, shift) (LPVOID)((INT64)base + shift)
#else
#define LPVOID_SHIFT(base, shift) (LPVOID)((INT32)base + shift)
#endif

// ====================================================================


// XADMA structures ===================================================


typedef struct __XADMA_REGION_HEADER {
    UINT magicValue;

    UINT version;

    UINT referenceCount;

} XADMA_REGION_HEADER, * PXADMA_REGION_HEADER;


typedef struct __XADMA_REGION_CONTEXT {
    HANDLE MappingHandle;

    LPVOID BaseAddress;

    PXADMA_REGION_HEADER RegionHeader;

    LPVOID RegionBaseAddress;

    LPCWSTR Name;

} XADMA_REGION_CONTEXT, * PXADMA_REGION_CONTEXT;

// ====================================================================


// XADMA basic constants ==============================================


DLLEXPORT UINT XADMAGetRegionHeaderReservation();


DLLEXPORT UINT XADMAGetRegionHeaderVersion();

// ====================================================================


// XADMA region header operations =====================================


DLLEXPORT BOOL XADMACheckRegionHeader(
    _In_        PXADMA_REGION_HEADER pHeader
    );


DLLEXPORT VOID XADMAInitializeRegionHeader(
    _Out_       PXADMA_REGION_HEADER pHeader,
    _In_        BOOL reference
    );

// ====================================================================


// XADMA region operations ============================================


_Ret_maybenull_
DLLEXPORT LPVOID XADMAQueryRegion(
    _In_        LPCWSTR name,
    _In_        BOOL reference,
    _Out_opt_   PXADMA_REGION_CONTEXT pRegionContext
    );


_Ret_maybenull_
DLLEXPORT LPVOID XADMAAllocateRegion(
    _In_        LPCWSTR name,
    _In_        DWORD size,
    _Out_opt_   PXADMA_REGION_CONTEXT pRegionContext
    );


_Ret_maybenull_
DLLEXPORT LPVOID XADMAQueryOrAllocateRegion(
    _In_        LPCWSTR name,
    _In_        DWORD expectedSize,
    _Out_opt_   PXADMA_REGION_CONTEXT pRegionContext
    );


#define XADMA_REGION_NOT_VALID                      0

#define XADMA_REGION_DESTROYED                      1

#define XADMA_REGION_REFERENCE_COUNT_REDUCED        2

DLLEXPORT UINT XADMAReleaseRegion(
    _In_        PXADMA_REGION_CONTEXT pRegionContext
    );

// ====================================================================