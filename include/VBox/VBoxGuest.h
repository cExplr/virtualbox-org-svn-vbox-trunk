/** @file
 * VBoxGuest - VirtualBox Guest Additions Driver Interface. (ADD,DEV)
 *
 * @note    This file is used by 16-bit compilers too (OpenWatcom).
 *
 * @remarks This is in the process of being split up and usage cleaned up.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef ___VBox_VBoxGuest_h
#define ___VBox_VBoxGuest_h

#include <VBox/types.h>
#include <iprt/assert.h>
#include <VBox/VMMDev2.h>



/** @defgroup grp_vboxguest  VirtualBox Guest Additions Device Driver
 *
 * Also know as VBoxGuest.
 *
 * @{
 */

/** @defgroup grp_vboxguest_ioc  VirtualBox Guest Additions Driver Interface
 *
 * @note This is considered internal in ring-3, please use the VbglR3 functions.
 *
 * - The 7th bit (128) is reserved for distinguishing between 32-bit and 64-bit
 *   processes in future 64-bit guest additions, where it matters.
 * - The 6th bit is reserved for future hacks.
 * - IDC specific requests descends from 63.
 *
 * @remarks When creating new IOCtl interfaces keep in mind that not all OSes supports
 *          reporting back the output size. (This got messed up a little bit in VBoxDrv.)
 *
 *          The request size is also a little bit tricky as it's passed as part of the
 *          request code on unix. The size field is 14 bits on Linux, 12 bits on *BSD,
 *          13 bits Darwin, and 8-bits on Solaris. All the BSDs and Darwin kernels
 *          will make use of the size field, while Linux and Solaris will not. We're of
 *          course using the size to validate and/or map/lock the request, so it has
 *          to be valid.
 *
 *          For Solaris we will have to do something special though, 255 isn't
 *          sufficient for all we need. A 4KB restriction (BSD) is probably not
 *          too problematic (yet) as a general one.
 *
 *          More info can be found in SUPDRVIOC.h and related sources.
 *
 * @remarks If adding interfaces that only has input or only has output, some new macros
 *          needs to be created so the most efficient IOCtl data buffering method can be
 *          used.
 *
 * @{
 */
#if !defined(IN_RC) && !defined(IN_RING0_AGNOSTIC) && !defined(IPRT_NO_CRT)

/** Fictive start address of the hypervisor physical memory for MmMapIoSpace. */
#define VBOXGUEST_HYPERVISOR_PHYSICAL_START         UINT32_C(0xf8000000)

#ifdef RT_OS_DARWIN
/** Cookie used to fend off some unwanted clients to the IOService. */
# define VBOXGUEST_DARWIN_IOSERVICE_COOKIE          UINT32_C(0x56426f78) /* 'VBox' */
#endif

/** The bit-count indicator mask. */
#define VBGL_IOCTL_FLAG_BIT_MASK                    UINT32_C(128)
/** 64-bit specific I/O control flag. */
#define VBGL_IOCTL_FLAG_64BIT                       UINT32_C(128)
/** 32-bit specific I/O control flag.
 * @note Just for complementing VBGL_IOCTL_FLAG_64BIT.  It is also the same
 *       value we use for bit-count agnostic requests, so don't ever use for
 *       testing!
 * @internal */
#define VBGL_IOCTL_FLAG_32BIT                       UINT32_C(0)
/** 16-bit specific I/O control flag. */
#define VBGL_IOCTL_FLAG_16BIT                       UINT32_C(64)
/** Check if the I/O control is a 64-bit one.   */
#define VBGL_IOCTL_IS_64BIT(a_uIOCtl)               RT_BOOL((a_uIOCtl) & VBGL_IOCTL_FLAG_64BIT)

/** @def VBGL_IOCTL_FLAG_CC
 * The context specific bit-count flag to include in the bit-count sensitive
 * I/O controls. */
#if ARCH_BITS == 64
# define VBGL_IOCTL_FLAG_CC                         VBGL_IOCTL_FLAG_64BIT
#elif ARCH_BITS == 32
# define VBGL_IOCTL_FLAG_CC                         VBGL_IOCTL_FLAG_32BIT
#elif ARCH_BITS == 16
# define VBGL_IOCTL_FLAG_CC                         VBGL_IOCTL_FLAG_16BIT
#else
# error "dunno which arch this is!"
#endif


#if defined(RT_OS_WINDOWS)
# ifndef CTL_CODE
#  include <iprt/win/windows.h>
# endif
  /* Automatic buffering, size not encoded. */
# define VBGL_IOCTL_CODE_SIZE(Function, Size)       CTL_CODE(FILE_DEVICE_UNKNOWN, 2048 + (Function), METHOD_BUFFERED, FILE_WRITE_ACCESS)
# define VBGL_IOCTL_CODE_BIG(Function)              CTL_CODE(FILE_DEVICE_UNKNOWN, 2048 + (Function), METHOD_BUFFERED, FILE_WRITE_ACCESS)
# define VBGL_IOCTL_CODE_FAST(Function)             CTL_CODE(FILE_DEVICE_UNKNOWN, 2048 + (Function), METHOD_NEITHER,  FILE_WRITE_ACCESS)
# define VBGL_IOCTL_CODE_STRIPPED(a_uIOCtl)         ((a_uIOCtl) & ~VBGL_IOCTL_FLAG_BIT_MASK)
# define VBOXGUEST_DEVICE_NAME                      "\\\\.\\VBoxGuest"
/** The support service name. */
# define VBOXGUEST_SERVICE_NAME                     "VBoxGuest"
/** Global name for Win2k+ */
# define VBOXGUEST_DEVICE_NAME_GLOBAL               "\\\\.\\Global\\VBoxGuest"
/** Win32 driver name */
# define VBOXGUEST_DEVICE_NAME_NT                   L"\\Device\\VBoxGuest"
/** Device name. */
# define VBOXGUEST_DEVICE_NAME_DOS                  L"\\DosDevices\\VBoxGuest"

#elif defined(RT_OS_OS2)
  /* No automatic buffering, size not encoded. */
# define VBGL_IOCTL_CATEGORY                        0xc2
# define VBGL_IOCTL_CODE_SIZE(Function, Size)       ((unsigned char)(Function))
# define VBGL_IOCTL_CODE_BIG(Function)              ((unsigned char)(Function))
# define VBGL_IOCTL_CATEGORY_FAST                   0xc3 /**< Also defined in VBoxGuestA-os2.asm. */
# define VBGL_IOCTL_CODE_FAST(Function)             ((unsigned char)(Function))
# define VBGL_IOCTL_CODE_STRIPPED(a_uIOCtl)         ((a_uIOCtl) & ~VBGL_IOCTL_FLAG_BIT_MASK)
# define VBOXGUEST_DEVICE_NAME                      "\\Dev\\VBoxGst$"
/** Short device name for AttachDD. */
# define VBOXGUEST_DEVICE_NAME_SHORT                "VBoxGst$"

#elif defined(RT_OS_SOLARIS)
  /* No automatic buffering, size limited to 255 bytes => use VBGLBIGREQ for everything. */
# include <sys/ioccom.h>
# define VBGL_IOCTL_CODE_SIZE(Function, Size)       _IOWRN('V', (Function), sizeof(VBGLREQHDR))
# define VBGL_IOCTL_CODE_BIG(Function)              _IOWRN('V', (Function), sizeof(VBGLREQHDR))
# define VBGL_IOCTL_CODE_FAST(Function)             _IO(   'F', (Function))
# define VBGL_IOCTL_CODE_STRIPPED(a_uIOCtl)         ((a_uIOCtl) & ~VBGL_IOCTL_FLAG_BIT_MASK)
# define VBGL_IOCTL_IS_FAST(a_uIOCtl)               ( ((a_uIOCtl) & 0x0000ff00) == ('F' << 8) )

#elif defined(RT_OS_LINUX)
  /* No automatic buffering, size limited to 16KB. */
# include <linux/ioctl.h>
# define VBGL_IOCTL_CODE_SIZE(Function, Size)       _IOC(_IOC_READ | _IOC_WRITE, 'V', (Function), (Size))
# define VBGL_IOCTL_CODE_BIG(Function)              _IO('V', (Function))
# define VBGL_IOCTL_CODE_FAST(Function)             _IO('F', (Function))
# define VBGL_IOCTL_CODE_STRIPPED(a_uIOCtl)         (_IOC_NR((a_uIOCtl)) & ~VBGL_IOCTL_FLAG_BIT_MASK)
# define VBOXGUEST_USER_DEVICE_NAME                 "/dev/vboxuser"

#elif defined(RT_OS_HAIKU)
  /* No automatic buffering, size not encoded. */
  /** @todo do something better */
# define VBGL_IOCTL_CODE_SIZE(Function, Size)       (0x56420000 | (Function))
# define VBGL_IOCTL_CODE_BIG(Function)              (0x56420000 | (Function))
# define VBGL_IOCTL_CODE_FAST(Function)             (0x56420000 | (Function))
# define VBGL_IOCTL_CODE_STRIPPED(a_uIOCtl)         ((a_uIOCtl) & ~VBGL_IOCTL_FLAG_BIT_MASK)
# define VBOXGUEST_DEVICE_NAME                      "/dev/misc/vboxguest"

#else /* BSD Like */
  /* Automatic buffering, size limited to 4KB on *BSD and 8KB on Darwin - commands the limit, 4KB. */
# include <sys/ioccom.h>
# define VBGL_IOCTL_CODE_SIZE(Function, Size)       _IOC(IOC_INOUT, 'V', (Function), (Size))
# define VBGL_IOCTL_CODE_BIG(Function)              _IO('V', (Function))
# define VBGL_IOCTL_CODE_FAST(Function)             _IO('F', (Function))
# define VBGL_IOCTL_CODE_STRIPPED(a_uIOCtl)         ((a_uIOCtl) & ~(_IOC(0,0,0,IOCPARM_MASK) | VBGL_IOCTL_FLAG_BIT_MASK))
#endif


/** @todo It would be nice if we could have two defines without paths. */

/** @def VBOXGUEST_DEVICE_NAME
 * The support device name. */
#ifndef VBOXGUEST_DEVICE_NAME /* PORTME */
# define VBOXGUEST_DEVICE_NAME          "/dev/vboxguest"
#endif

/** @def VBOXGUEST_USER_DEVICE_NAME
 * The support device name of the user accessible device node. */
#ifndef VBOXGUEST_USER_DEVICE_NAME
# define VBOXGUEST_USER_DEVICE_NAME     VBOXGUEST_DEVICE_NAME
#endif


/**
 * Common In/Out header.
 *
 * This is a copy/mirror of VMMDevRequestHeader to prevent duplicating data and
 * needing to verify things multiple times.  For that reason this differs a bit
 * from SUPREQHDR.
 *
 * @sa VMMDevRequestHeader
 */
typedef struct VBGLREQHDR
{
    /** IN: The request input size, and output size if cbOut is zero.
     * @sa VMMDevRequestHeader::size  */
    uint32_t        cbIn;
    /** IN: Structure version (VBGLREQHDR_VERSION)
     * @sa VMMDevRequestHeader::version */
    uint32_t        uVersion;
    /** IN: The VMMDev request type, set to VBGLREQHDR_TYPE_DEFAULT unless this is a
     * kind of VMMDev request.
     * @sa VMMDevRequestType, VMMDevRequestHeader::requestType */
    uint32_t        uType;
    /** OUT: The VBox status code of the operation, out direction only. */
    int32_t         rc;
    /** IN: The output size.  This is optional - set to zero to use cbIn as the
     * output size. */
    uint32_t        cbOut;
    /** Reserved, MBZ. */
    uint32_t        uReserved;
} VBGLREQHDR;
AssertCompileSize(VBGLREQHDR, 24);
/** Pointer to a IOC header. */
typedef VBGLREQHDR RT_FAR *PVBGLREQHDR;

/** Version of VMMDevRequestHeader structure. */
#define VBGLREQHDR_VERSION          UINT32_C(0x10001)
/** Default request type.  Use this for non-VMMDev requests. */
#define VBGLREQHDR_TYPE_DEFAULT     UINT32_C(0)

/** Initialize a VBGLREQHDR structure for a fixed size I/O control call.
 * @param   a_pHdr      Pointer to the header to initialize.
 * @param   a_IOCtl     The base I/O control name, no VBGL_IOCTL_ prefix.  We
 *                      have to skip the prefix to avoid it getting expanded
 *                      before we append _SIZE_IN and _SIZE_OUT to it.
 */
#define VBGLREQHDR_INIT(a_pHdr, a_IOCtl) \
    VBGLREQHDR_INIT_EX(a_pHdr, RT_CONCAT3(VBGL_IOCTL_,a_IOCtl,_SIZE_IN), RT_CONCAT3(VBGL_IOCTL_,a_IOCtl,_SIZE_OUT))
/** Initialize a VBGLREQHDR structure, extended version. */
#define VBGLREQHDR_INIT_EX(a_pHdr, a_cbIn, a_cbOut) \
    do { \
        (a_pHdr)->cbIn      = (uint32_t)(a_cbIn); \
        (a_pHdr)->uVersion  = VBGLREQHDR_VERSION; \
        (a_pHdr)->uType     = VBGLREQHDR_TYPE_DEFAULT; \
        (a_pHdr)->rc        = VERR_INTERNAL_ERROR; \
        (a_pHdr)->cbOut     = (uint32_t)(a_cbOut); \
        (a_pHdr)->uReserved = 0; \
    } while (0)
/** Initialize a VBGLREQHDR structure for a VMMDev request.
 * Same as VMMDEV_REQ_HDR_INIT().  */
#define VBGLREQHDR_INIT_VMMDEV(a_pHdr, a_cb, a_enmType) \
    do { \
        (a_pHdr)->cbIn      = (a_cb); \
        (a_pHdr)->uVersion  = VBGLREQHDR_VERSION; \
        (a_pHdr)->uType     = (a_enmType); \
        (a_pHdr)->rc        = VERR_INTERNAL_ERROR; \
        (a_pHdr)->cbOut     = 0; \
        (a_pHdr)->uReserved = 0; \
    } while (0)



/**
 * The VBoxGuest I/O control version.
 *
 * As usual, the high word contains the major version and changes to it
 * signifies incompatible changes.
 *
 * The lower word is the minor version number, it is increased when new
 * functions are added or existing changed in a backwards compatible manner.
 */
#define VBGL_IOC_VERSION                          UINT32_C(0x00010000)



/** @name VBGL_IOCTL_DRIVER_INFO
 * Adjust and get driver information.
 *
 * @note May switch the session to a backwards compatible interface version if
 *       uClientVersion indicates older client code.
 *
 * @{
 */
#define VBGL_IOCTL_DRIVER_VERSION_INFO              VBGL_IOCTL_CODE_SIZE(0, VBGL_IOCTL_DRIVER_VERSION_INFO_SIZE)
#define VBGL_IOCTL_DRIVER_VERSION_INFO_SIZE         sizeof(VBGLIOCDRIVERVERSIONINFO)
#define VBGL_IOCTL_DRIVER_VERSION_INFO_SIZE_IN      RT_UOFFSET_AFTER(VBGLIOCDRIVERVERSIONINFO, u.In)
#define VBGL_IOCTL_DRIVER_VERSION_INFO_SIZE_OUT     sizeof(VBGLIOCDRIVERVERSIONINFO)
typedef struct VBGLIOCDRIVERVERSIONINFO
{
    /** The header. */
    VBGLREQHDR          Hdr;
    union
    {
        struct
        {
            /** The requested interface version number (VBGL_IOC_VERSION). */
            uint32_t        uReqVersion;
            /** The minimum interface version number
             * (typically the major version part of VBGL_IOC_VERSION). */
            uint32_t        uMinVersion;
            /** Reserved, MBZ. */
            uint32_t        uReserved1;
            /** Reserved, MBZ. */
            uint32_t        uReserved2;
        } In;
        struct
        {
            /** Interface version for this session (typically VBGL_IOC_VERSION). */
            uint32_t        uSessionVersion;
            /** The version of the IDC interface (VBGL_IOC_VERSION). */
            uint32_t        uDriverVersion;
            /** The SVN revision of the driver.
             * This will be set to 0 if not compiled into the driver. */
            uint32_t        uDriverRevision;
            /** Reserved \#1 (will be returned as zero until defined). */
            uint32_t        uReserved1;
            /** Reserved \#2 (will be returned as zero until defined). */
            uint32_t        uReserved2;
        } Out;
    } u;
} VBGLIOCDRIVERVERSIONINFO, RT_FAR *PVBGLIOCDRIVERVERSIONINFO;
AssertCompileSize(VBGLIOCDRIVERVERSIONINFO, 24 + 20);
#ifndef __GNUC__ /* Some GCC versions can't handle the complicated RT_UOFFSET_AFTER macro, it seems. */
AssertCompile(VBGL_IOCTL_DRIVER_VERSION_INFO_SIZE_IN == 24 + 16);
#endif
/** @} */


/** @name VBGL_IOCTL_GET_PORT_INFO
 * Query VMMDev I/O port region and MMIO mapping address.
 * @remarks Ring-0 only.
 * @{
 */
#define VBGL_IOCTL_GET_VMMDEV_IO_INFO               VBGL_IOCTL_CODE_SIZE(1 | VBGL_IOCTL_FLAG_CC, VBGL_IOCTL_GET_VMMDEV_IO_INFO_SIZE)
#define VBGL_IOCTL_GET_VMMDEV_IO_INFO_SIZE          sizeof(VBGLIOCGETVMMDEVIOINFO)
#define VBGL_IOCTL_GET_VMMDEV_IO_INFO_SIZE_IN       sizeof(VBGLREQHDR)
#define VBGL_IOCTL_GET_VMMDEV_IO_INFO_SIZE_OUT      sizeof(VBGLIOCGETVMMDEVIOINFO)
typedef struct VBGLIOCGETVMMDEVIOINFO
{
    /** The header. */
    VBGLREQHDR  Hdr;
    union
    {
        struct
        {
            /** The MMIO mapping.  NULL if no MMIO region. */
            struct VMMDevMemory volatile RT_FAR *pvVmmDevMapping;
            /** The I/O port address. */
            RTIOPORT                        IoPort;
            /** Padding, ignore. */
            RTIOPORT                        auPadding[HC_ARCH_BITS == 64 ? 3 : 1];
        } Out;
    } u;
} VBGLIOCGETVMMDEVIOINFO, RT_FAR *PVBGLIOCGETVMMDEVIOINFO;
AssertCompileSize(VBGLIOCGETVMMDEVIOINFO, 24 + (HC_ARCH_BITS == 64 ? 16 : 8));
/** @} */


/** @name VBGL_IOCTL_VMMDEV_REQUEST
 * IOCTL to VBoxGuest to perform a VMM Device request less than 1KB in size.
 * @{
 */
#define VBGL_IOCTL_VMMDEV_REQUEST(a_cb)             VBGL_IOCTL_CODE_SIZE(2, (a_cb))
/** @} */


/** @name VBGL_IOCTL_VMMDEV_REQUEST_BIG
 * IOCTL to VBoxGuest to perform a VMM Device request that can 1KB or larger.
 * @{
 */
#define VBGL_IOCTL_VMMDEV_REQUEST_BIG               VBGL_IOCTL_CODE_BIG(3)
/** @} */


#ifdef VBOX_WITH_HGCM
/** @name VBGL_IOCTL_HGCM_CONNECT
 * Connect to a HGCM service.
 * @{ */
# define VBGL_IOCTL_HGCM_CONNECT                    VBGL_IOCTL_CODE_SIZE(4, VBGL_IOCTL_HGCM_CONNECT_SIZE)
# define VBGL_IOCTL_HGCM_CONNECT_SIZE               sizeof(VBGLIOCHGCMCONNECT)
# define VBGL_IOCTL_HGCM_CONNECT_SIZE_IN            sizeof(VBGLIOCHGCMCONNECT)
# define VBGL_IOCTL_HGCM_CONNECT_SIZE_OUT           RT_UOFFSET_AFTER(VBGLIOCHGCMCONNECT, u.Out)
typedef struct VBGLIOCHGCMCONNECT
{
    /** The header. */
    VBGLREQHDR                  Hdr;
    union
    {
        struct
        {
            HGCMServiceLocation Loc;
        } In;
        struct
        {
            uint32_t            idClient;
        } Out;
    } u;
} VBGLIOCHGCMCONNECT, RT_FAR *PVBGLIOCHGCMCONNECT;
AssertCompileSize(VBGLIOCHGCMCONNECT, 24 + 132);
#ifndef __GNUC__ /* Some GCC versions can't handle the complicated RT_UOFFSET_AFTER macro, it seems. */
AssertCompile(VBGL_IOCTL_HGCM_CONNECT_SIZE_OUT == 24 + 4);
#endif
/** @} */


/** @name VBGL_IOCTL_HGCM_DISCONNECT
 * Disconnect from a HGCM service.
 * @{ */
# define VBGL_IOCTL_HGCM_DISCONNECT                 VBGL_IOCTL_CODE_SIZE(5, VBGL_IOCTL_HGCM_DISCONNECT_SIZE)
# define VBGL_IOCTL_HGCM_DISCONNECT_SIZE            sizeof(VBGLIOCHGCMDISCONNECT)
# define VBGL_IOCTL_HGCM_DISCONNECT_SIZE_IN         sizeof(VBGLIOCHGCMDISCONNECT)
# define VBGL_IOCTL_HGCM_DISCONNECT_SIZE_OUT        sizeof(VBGLREQHDR)
/** @note This is also used by a VbglR0 API.  */
typedef struct VBGLIOCHGCMDISCONNECT
{
    /** The header. */
    VBGLREQHDR          Hdr;
    union
    {
        struct
        {
            uint32_t    idClient;
        } In;
    } u;
} VBGLIOCHGCMDISCONNECT, RT_FAR *PVBGLIOCHGCMDISCONNECT;
AssertCompileSize(VBGLIOCHGCMDISCONNECT, 24 + 4);
/** @} */


/** @name VBGL_IOCTL_HGCM_CALL, VBGL_IOCTL_HGCM_CALL_WITH_USER_DATA
 *
 * Make a call to a HGCM servicesure.  There are several variations here.
 *
 * The VBGL_IOCTL_HGCM_CALL_WITH_USER_DATA variation is for other drivers (like
 * the graphics ones) passing on requests from user land that contains user
 * data.  These calls are always interruptible.
 *
 * @{ */
# define VBGL_IOCTL_HGCM_CALL(a_cb)                 VBGL_IOCTL_CODE_SIZE(6 | VBGL_IOCTL_FLAG_CC,    (a_cb))
# define VBGL_IOCTL_HGCM_CALL_32(a_cb)              VBGL_IOCTL_CODE_SIZE(6 | VBGL_IOCTL_FLAG_32BIT, (a_cb))
# define VBGL_IOCTL_HGCM_CALL_64(a_cb)              VBGL_IOCTL_CODE_SIZE(6 | VBGL_IOCTL_FLAG_64BIT, (a_cb))
# define VBGL_IOCTL_HGCM_CALL_WITH_USER_DATA(a_cb)  VBGL_IOCTL_CODE_SIZE(7 | VBGL_IOCTL_FLAG_CC,    (a_cb))
/** @note This is used by alot of HGCM call structures. */
typedef struct VBGLIOCHGCMCALL
{
    /** Common header. */
    VBGLREQHDR  Hdr;
    /** Input: The id of the caller. */
    uint32_t    u32ClientID;
    /** Input: Function number. */
    uint32_t    u32Function;
    /** Input: How long to wait (milliseconds) for completion before cancelling the
     * call.  This is ignored if not a VBGL_IOCTL_HGCM_CALL_TIMED or
     * VBGL_IOCTL_HGCM_CALL_TIMED_32 request. */
    uint32_t    cMsTimeout;
    /** Input: Whether a timed call is interruptible (ring-0 only).  This is ignored
     * if not a VBGL_IOCTL_HGCM_CALL_TIMED or VBGL_IOCTL_HGCM_CALL_TIMED_32
     * request, or if made from user land. */
    bool        fInterruptible;
    /** Explicit padding, MBZ. */
    uint8_t     bReserved;
    /** Input: How many parameters following this structure.
     *
     * The parameters are either HGCMFunctionParameter64 or HGCMFunctionParameter32,
     * depending on whether we're receiving a 64-bit or 32-bit request.
     *
     * The current maximum is 61 parameters (given a 1KB max request size,
     * and a 64-bit parameter size of 16 bytes).
     *
     * @note This information is duplicated by Hdr.cbIn, but it's currently too much
     *       work to eliminate this. */
    uint16_t    cParms;
    /* Parameters follow in form HGCMFunctionParameter aParms[cParms] */
} VBGLIOCHGCMCALL, RT_FAR *PVBGLIOCHGCMCALL;
AssertCompileSize(VBGLIOCHGCMCALL, 24 + 16);
typedef VBGLIOCHGCMCALL const RT_FAR *PCVBGLIOCHGCMCALL;

/**
 * Initialize a HGCM header (VBGLIOCHGCMCALL) for a non-timed call.
 *
 * @param   a_pHdr          The header to initalize.
 * @param   a_idClient      The client connection ID to call thru.
 * @param   a_idFunction    The function we're calling
 * @param   a_cParameters   Number of parameters.
 */
# define VBGL_HGCM_HDR_INIT(a_pHdr, a_idClient, a_idFunction, a_cParameters) \
    do { \
        VBGLREQHDR_INIT_EX(&(a_pHdr)->Hdr, \
                           sizeof(VBGLIOCHGCMCALL) + (a_cParameters) * sizeof(HGCMFunctionParameter), \
                           sizeof(VBGLIOCHGCMCALL) + (a_cParameters) * sizeof(HGCMFunctionParameter)); \
        (a_pHdr)->u32ClientID    = (a_idClient); \
        (a_pHdr)->u32Function    = (a_idFunction); \
        (a_pHdr)->cMsTimeout     = RT_INDEFINITE_WAIT; \
        (a_pHdr)->fInterruptible = true; \
        (a_pHdr)->bReserved      = 0; \
        (a_pHdr)->cParms         = (a_cParameters); \
    } while (0)

/**
 * Initialize a HGCM header (VBGLIOCHGCMCALL) for a non-timed call, custom size.
 *
 * This is usually only needed when appending page lists to the call.
 *
 * @param   a_pHdr          The header to initalize.
 * @param   a_idClient      The client connection ID to call thru.
 * @param   a_idFunction    The function we're calling
 * @param   a_cParameters   Number of parameters.
 * @param   a_cbReq         The request size.
 */
# define VBGL_HGCM_HDR_INIT_EX(a_pHdr, a_idClient, a_idFunction, a_cParameters, a_cbReq) \
    do { \
        Assert((a_cbReq) >= sizeof(VBGLIOCHGCMCALL) + (a_cParameters) * sizeof(HGCMFunctionParameter)); \
        VBGLREQHDR_INIT_EX(&(a_pHdr)->Hdr, (a_cbReq), (a_cbReq)); \
        (a_pHdr)->u32ClientID    = (a_idClient); \
        (a_pHdr)->u32Function    = (a_idFunction); \
        (a_pHdr)->cMsTimeout     = RT_INDEFINITE_WAIT; \
        (a_pHdr)->fInterruptible = true; \
        (a_pHdr)->bReserved      = 0; \
        (a_pHdr)->cParms         = (a_cParameters); \
    } while (0)

/**
 * Initialize a HGCM header (VBGLIOCHGCMCALL), with timeout (interruptible).
 *
 * @param   a_pHdr          The header to initalize.
 * @param   a_idClient      The client connection ID to call thru.
 * @param   a_idFunction    The function we're calling
 * @param   a_cParameters   Number of parameters.
 * @param   a_cMsTimeout    The timeout in milliseconds.
 */
# define VBGL_HGCM_HDR_INIT_TIMED(a_pHdr, a_idClient, a_idFunction, a_cParameters, a_cMsTimeout) \
    do { \
        (a_pHdr)->u32ClientID    = (a_idClient); \
        (a_pHdr)->u32Function    = (a_idFunction); \
        (a_pHdr)->cMsTimeout     = (a_cMsTimeout); \
        (a_pHdr)->fInterruptible = true; \
        (a_pHdr)->bReserved      = 0; \
        (a_pHdr)->cParms         = (a_cParameters); \
    } while (0)

/** Get the pointer to the first HGCM parameter.  */
# define VBGL_HGCM_GET_CALL_PARMS(a_pInfo)   ( (HGCMFunctionParameter   *)((uint8_t *)(a_pInfo) + sizeof(VBGLIOCHGCMCALL)) )
/** Get the pointer to the first HGCM parameter in a 32-bit request.  */
# define VBGL_HGCM_GET_CALL_PARMS32(a_pInfo) ( (HGCMFunctionParameter32 *)((uint8_t *)(a_pInfo) + sizeof(VBGLIOCHGCMCALL)) )

/** @} */
#endif /* VBOX_WITH_HGCM */


/** @name VBGL_IOCTL_LOG
 * IOCTL to VBoxGuest to perform backdoor logging.
 * @{ */
#define VBOXGUEST_IOCTL_LOG(Size)
#define VBGL_IOCTL_LOG(a_cchMsg)                    VBGL_IOCTL_CODE_BIG(8)
#define VBGL_IOCTL_LOG_SIZE(a_cchMsg)               (sizeof(VBGLREQHDR) + (a_cchMsg) + 1)
#define VBGL_IOCTL_LOG_SIZE_IN(a_cchMsg)            (sizeof(VBGLREQHDR) + (a_cchMsg) + 1)
#define VBGL_IOCTL_LOG_SIZE_OUT                     sizeof(VBGLREQHDR)
typedef struct VBGLIOCLOG
{
    /** The header. */
    VBGLREQHDR                      Hdr;
    union
    {
        struct
        {
            /** The log message.
             * The length is determined from the input size and zero termination. */
            char                    szMsg[RT_FLEXIBLE_ARRAY_IN_NESTED_UNION];
        } In;
    } u;
} VBGLIOCLOG, RT_FAR *PVBGLIOCLOG;
/** @} */


/** @name VBGL_IOCTL_WAIT_FOR_EVENTS
 * Wait for a VMMDev host event notification.
 * @{
 */
#define VBGL_IOCTL_WAIT_FOR_EVENTS                  VBGL_IOCTL_CODE_SIZE(9, VBGL_IOCTL_GET_VMMDEV_IO_INFO_SIZE)
#define VBGL_IOCTL_WAIT_FOR_EVENTS_SIZE             sizeof(VBGLIOCWAITFOREVENTS)
#define VBGL_IOCTL_WAIT_FOR_EVENTS_SIZE_IN          sizeof(VBGLIOCWAITFOREVENTS)
#define VBGL_IOCTL_WAIT_FOR_EVENTS_SIZE_OUT         RT_UOFFSET_AFTER(VBGLIOCWAITFOREVENTS, u.Out)
typedef struct VBGLIOCWAITFOREVENTS
{
    /** The header. */
    VBGLREQHDR                      Hdr;
    union
    {
        struct
        {
            /** Timeout in milliseconds. */
            uint32_t                cMsTimeOut;
            /** Events to wait for. */
            uint32_t                fEvents;
        } In;
        struct
        {
            /** Events that occurred. */
            uint32_t                fEvents;
        } Out;
    } u;
} VBGLIOCWAITFOREVENTS, RT_FAR *PVBGLIOCWAITFOREVENTS;
AssertCompileSize(VBGLIOCWAITFOREVENTS, 24 + 8);
/** @} */


/** @name VBGL_IOCTL_INTERRUPT_ALL_WAIT_FOR_EVENTS
 * IOCTL to VBoxGuest to interrupt (cancel) any pending
 * VBGL_IOCTL_WAIT_FOR_EVENTS and return.
 *
 * Handled inside the guest additions and not seen by the host at all.
 * After calling this, VBGL_IOCTL_WAIT_FOR_EVENTS should no longer be called in
 * the same session.  At the time of writing this is not enforced; at the time
 * of reading it may be.
 * @see VBGL_IOCTL_WAIT_FOR_EVENTS
 *
 * @{
 */
#define VBGL_IOCTL_INTERRUPT_ALL_WAIT_FOR_EVENTS    VBGL_IOCTL_CODE_SIZE(10, VBGL_IOCTL_INTERRUPT_ALL_WAIT_FOR_EVENTS_SIZE)
#define VBGL_IOCTL_INTERRUPT_ALL_WAIT_FOR_EVENTS_SIZE       sizeof(VBGLREQHDR)
#define VBGL_IOCTL_INTERRUPT_ALL_WAIT_FOR_EVENTS_SIZE_IN    sizeof(VBGLREQHDR)
#define VBGL_IOCTL_INTERRUPT_ALL_WAIT_FOR_EVENTS_SIZE_OUT   sizeof(VBGLREQHDR)
/** @} */


/** @name VBGL_IOCTL_CHANGE_FILTER_MASK
 * IOCTL to VBoxGuest to control the event filter mask.
 * @{ */
#define VBGL_IOCTL_CHANGE_FILTER_MASK               VBGL_IOCTL_CODE_SIZE(11, VBGL_IOCTL_CHANGE_FILTER_MASK_SIZE)
#define VBGL_IOCTL_CHANGE_FILTER_MASK_SIZE          sizeof(VBGLIOCCHANGEFILTERMASK)
#define VBGL_IOCTL_CHANGE_FILTER_MASK_SIZE_IN       sizeof(VBGLIOCCHANGEFILTERMASK)
#define VBGL_IOCTL_CHANGE_FILTER_MASK_SIZE_OUT      sizeof(VBGLREQHDR)
typedef struct VBGLIOCCHANGEFILTERMASK
{
    /** The header. */
    VBGLREQHDR                      Hdr;
    union
    {
        struct
        {
            /** Flags to set. */
            uint32_t fOrMask;
            /** Flags to remove. */
            uint32_t fNotMask;
        } In;
    } u;
} VBGLIOCCHANGEFILTERMASK, RT_FAR *PVBGLIOCCHANGEFILTERMASK;
AssertCompileSize(VBGLIOCCHANGEFILTERMASK, 24 + 8);
/** @} */


/** @name VBGL_IOCTL_GUEST_CAPS_ACQUIRE
 * IOCTL to for acquiring and releasing guest capabilities.
 *
 * This is used for multiple purposes:
 * 1. By doing @a acquire r3 client application (e.g. VBoxTray) claims it will
 *    use the given session for performing operations like @a seamless or
 *    @a auto-resize, thus, if the application terminates, the driver will
 *    automatically cleanup the caps reported to host, so that host knows guest
 *    does not support them anymore
 * 2. In a multy-user environment this will not allow r3 applications (like
 *    VBoxTray) running in different user sessions simultaneously to interfere
 *    with each other.  An r3 client application (like VBoxTray) is responsible
 *    for Acquiring/Releasing caps properly as needed.
 *
 *
 * VERR_RESOURCE_BUSY is returned if any capabilities in the fOrMask are
 * currently acquired by some other VBoxGuest session.
 *
 * @{
 */
#define VBGL_IOCTL_ACQUIRE_GUEST_CAPABILITIES           VBGL_IOCTL_CODE_SIZE(12, VBGL_IOCTL_ACQUIRE_GUEST_CAPABILITIES_SIZE)
#define VBGL_IOCTL_ACQUIRE_GUEST_CAPABILITIES_SIZE      sizeof(VBGLIOCACQUIREGUESTCAPS)
#define VBGL_IOCTL_ACQUIRE_GUEST_CAPABILITIES_SIZE_IN   sizeof(VBGLIOCACQUIREGUESTCAPS)
#define VBGL_IOCTL_ACQUIRE_GUEST_CAPABILITIES_SIZE_OUT  sizeof(VBGLREQHDR)

/** Default operation (full acquire/release). */
#define VBGL_IOC_AGC_FLAGS_DEFAULT                      UINT32_C(0x00000000)
/** Configures VBoxGuest to use the specified caps in Acquire mode, w/o making
 * any caps acquisition/release.  This is only possible to set acquire mode for
 * caps, but not clear it, so fNotMask is ignored when this flag is set. */
#define VBGL_IOC_AGC_FLAGS_CONFIG_ACQUIRE_MODE          UINT32_C(0x00000001)
/** Valid flag mask. */
#define VBGL_IOC_AGC_FLAGS_VALID_MASK                   UINT32_C(0x00000001)

typedef struct VBGLIOCACQUIREGUESTCAPS
{
    /** The header. */
    VBGLREQHDR              Hdr;
    union
    {
        struct
        {
            /** Acquire flags (VBGL_IOC_AGC_FLAGS_XXX). */
            uint32_t        fFlags;
            /** Guest capabilities to acquire (VMMDEV_GUEST_SUPPORTS_XXX). */
            uint32_t        fOrMask;
            /** Guest capabilities to release (VMMDEV_GUEST_SUPPORTS_XXX). */
            uint32_t        fNotMask;
        } In;
    } u;
} VBGLIOCACQUIREGUESTCAPS, RT_FAR *PVBGLIOCACQUIREGUESTCAPS;
AssertCompileSize(VBGLIOCACQUIREGUESTCAPS, 24 + 12);
/** @} */


/** @name VBGL_IOCTL_CHANGE_GUEST_CAPABILITIES
 * IOCTL to VBoxGuest to set guest capabilities.
 * @{ */
#define VBGL_IOCTL_CHANGE_GUEST_CAPABILITIES            VBGL_IOCTL_CODE_SIZE(13, VBGL_IOCTL_CHANGE_GUEST_CAPABILITIES_SIZE)
#define VBGL_IOCTL_CHANGE_GUEST_CAPABILITIES_SIZE       sizeof(VBGLIOCSETGUESTCAPS)
#define VBGL_IOCTL_CHANGE_GUEST_CAPABILITIES_SIZE_IN    sizeof(VBGLIOCSETGUESTCAPS)
#define VBGL_IOCTL_CHANGE_GUEST_CAPABILITIES_SIZE_OUT   sizeof(VBGLIOCSETGUESTCAPS)
typedef struct VBGLIOCSETGUESTCAPS
{
    /** The header. */
    VBGLREQHDR              Hdr;
    union
    {
        struct
        {
            /** The capabilities to set (VMMDEV_GUEST_SUPPORTS_XXX). */
            uint32_t        fOrMask;
            /** The capabilities to drop (VMMDEV_GUEST_SUPPORTS_XXX). */
            uint32_t        fNotMask;
        } In;
        struct
        {
            /** The capabilities held by the session after the call (VMMDEV_GUEST_SUPPORTS_XXX). */
            uint32_t        fSessionCaps;
            /** The capabilities for all the sessions after the call (VMMDEV_GUEST_SUPPORTS_XXX). */
            uint32_t        fGlobalCaps;
        } Out;
    } u;
} VBGLIOCSETGUESTCAPS, RT_FAR *PVBGLIOCSETGUESTCAPS;
AssertCompileSize(VBGLIOCSETGUESTCAPS, 24 + 8);
typedef VBGLIOCSETGUESTCAPS VBoxGuestSetCapabilitiesInfo;
/** @} */


/** @name VBGL_IOCTL_SET_MOUSE_STATUS
 * IOCTL to VBoxGuest to update the mouse status features.
 * @{ */
#define VBGL_IOCTL_SET_MOUSE_STATUS                 VBGL_IOCTL_CODE_SIZE(14, VBGL_IOCTL_SET_MOUSE_STATUS_SIZE)
#define VBGL_IOCTL_SET_MOUSE_STATUS_SIZE            sizeof(VBGLIOCSETMOUSESTATUS)
#define VBGL_IOCTL_SET_MOUSE_STATUS_SIZE_IN         sizeof(VBGLIOCSETMOUSESTATUS)
#define VBGL_IOCTL_SET_MOUSE_STATUS_SIZE_OUT        sizeof(VBGLREQHDR)
typedef struct VBGLIOCSETMOUSESTATUS
{
    /** The header. */
    VBGLREQHDR          Hdr;
    union
    {
        struct
        {
            /** Mouse status flags (VMMDEV_MOUSE_XXX). */
            uint32_t    fStatus;
        } In;
    } u;
} VBGLIOCSETMOUSESTATUS, RT_FAR *PVBGLIOCSETMOUSESTATUS;
/** @} */


/** @name VBGL_IOCTL_SET_MOUSE_NOTIFY_CALLBACK
 *
 * IOCTL to for setting the mouse driver callback.
 * @note The callback will be called in interrupt context with the VBoxGuest
 *       device event spinlock held.
 * @note ring-0 only.
 *
 * @{ */
#define VBGL_IOCTL_SET_MOUSE_NOTIFY_CALLBACK            VBGL_IOCTL_CODE_SIZE(15, VBGL_IOCTL_SET_MOUSE_NOTIFY_CALLBACK_SIZE)
#define VBGL_IOCTL_SET_MOUSE_NOTIFY_CALLBACK_SIZE       sizeof(VBGLIOCSETMOUSENOTIFYCALLBACK)
#define VBGL_IOCTL_SET_MOUSE_NOTIFY_CALLBACK_SIZE_IN    sizeof(VBGLIOCSETMOUSENOTIFYCALLBACK)
#define VBGL_IOCTL_SET_MOUSE_NOTIFY_CALLBACK_SIZE_OUT   sizeof(VBGLREQHDR)
/**
 * Mouse event noticification callback function.
 * @param   pvUser      Argument given when setting the callback.
 */
typedef DECLCALLBACK(void) FNVBOXGUESTMOUSENOTIFY(void *pvUser);
/** Pointer to a mouse event notification callback function. */
typedef FNVBOXGUESTMOUSENOTIFY *PFNVBOXGUESTMOUSENOTIFY; /**< @todo fix type prefix */
typedef struct VBGLIOCSETMOUSENOTIFYCALLBACK
{
    /** The header. */
    VBGLREQHDR              Hdr;
    union
    {
        struct
        {
            /** Mouse notification callback function. */
            PFNVBOXGUESTMOUSENOTIFY     pfnNotify;
            /** The callback argument. */
            void                RT_FAR *pvUser;
        } In;
    } u;
} VBGLIOCSETMOUSENOTIFYCALLBACK, RT_FAR *PVBGLIOCSETMOUSENOTIFYCALLBACK;
/** @} */


/** @name VBGL_IOCTL_CHECK_BALLOON
 * IOCTL to VBoxGuest to check memory ballooning.
 *
 * The guest kernel module / device driver will ask the host for the current size of
 * the balloon and adjust the size. Or it will set fHandledInR0 = false and R3 is
 * responsible for allocating memory and calling R0 (VBGL_IOCTL_CHANGE_BALLOON).
 * @{ */
#define VBGL_IOCTL_CHECK_BALLOON                    VBGL_IOCTL_CODE_SIZE(16, VBGL_IOCTL_CHECK_BALLOON_SIZE)
#define VBGL_IOCTL_CHECK_BALLOON_SIZE               sizeof(VBGLIOCCHECKBALLOON)
#define VBGL_IOCTL_CHECK_BALLOON_SIZE_IN            sizeof(VBGLREQHDR)
#define VBGL_IOCTL_CHECK_BALLOON_SIZE_OUT           sizeof(VBGLIOCCHECKBALLOON)
typedef struct VBGLIOCCHECKBALLOON
{
    /** The header. */
    VBGLREQHDR                      Hdr;
    union
    {
        struct
        {
            /** The size of the balloon in chunks of 1MB. */
            uint32_t                cBalloonChunks;
            /** false = handled in R0, no further action required.
             *   true = allocate balloon memory in R3. */
            bool                    fHandleInR3;
            /** Explicit padding, please ignore. */
            bool                    afPadding[3];
        } Out;
    } u;
} VBGLIOCCHECKBALLOON, RT_FAR *PVBGLIOCCHECKBALLOON;
AssertCompileSize(VBGLIOCCHECKBALLOON, 24 + 8);
typedef VBGLIOCCHECKBALLOON VBoxGuestCheckBalloonInfo;
/** @} */


/** @name VBGL_IOCTL_CHANGE_BALLOON
 * IOCTL to VBoxGuest to supply or revoke one chunk for ballooning.
 *
 * The guest kernel module / device driver will lock down supplied memory or
 * unlock reclaimed memory and then forward the physical addresses of the
 * changed balloon chunk to the host.
 *
 * @{ */
#define VBGL_IOCTL_CHANGE_BALLOON                   VBGL_IOCTL_CODE_SIZE(17, VBGL_IOCTL_CHANGE_BALLOON_SIZE)
#define VBGL_IOCTL_CHANGE_BALLOON_SIZE              sizeof(VBGLIOCCHANGEBALLOON)
#define VBGL_IOCTL_CHANGE_BALLOON_SIZE_IN           sizeof(VBGLIOCCHANGEBALLOON)
#define VBGL_IOCTL_CHANGE_BALLOON_SIZE_OUT          sizeof(VBGLREQHDR)
typedef struct VBGLIOCCHANGEBALLOON
{
    /** The header. */
    VBGLREQHDR          Hdr;
    union
    {
        struct
        {
            /** Address of the chunk (user space address). */
            RTR3PTR     pvChunk;
            /** Explicit alignment padding, MBZ. */
            uint8_t     abPadding[ARCH_BITS == 64 ? 0 + 7 : 4 + 7];
            /** true = inflate, false = deflate. */
            bool        fInflate;
        } In;
    } u;
} VBGLIOCCHANGEBALLOON, RT_FAR *PVBGLIOCCHANGEBALLOON;
AssertCompileSize(VBGLIOCCHANGEBALLOON, 24+16);
/** @} */


/** @name VBGL_IOCTL_WRITE_CORE_DUMP
 * IOCTL to VBoxGuest to write guest core.
 * @{ */
#define VBGL_IOCTL_WRITE_CORE_DUMP                  VBGL_IOCTL_CODE_SIZE(18, VBGL_IOCTL_WRITE_CORE_DUMP_SIZE)
#define VBGL_IOCTL_WRITE_CORE_DUMP_SIZE             sizeof(VBGLIOCWRITECOREDUMP)
#define VBGL_IOCTL_WRITE_CORE_DUMP_SIZE_IN          sizeof(VBGLIOCWRITECOREDUMP)
#define VBGL_IOCTL_WRITE_CORE_DUMP_SIZE_OUT         sizeof(VBGLREQHDR)
typedef struct VBGLIOCWRITECOREDUMP
{
    /** The header. */
    VBGLREQHDR          Hdr;
    union
    {
        struct
        {
            /** Flags (reserved, MBZ). */
            uint32_t    fFlags;
        } In;
    } u;
} VBGLIOCWRITECOREDUMP, RT_FAR *PVBGLIOCWRITECOREDUMP;
AssertCompileSize(VBGLIOCWRITECOREDUMP, 24 + 4);
typedef VBGLIOCWRITECOREDUMP VBoxGuestWriteCoreDump;
/** @} */


#ifdef VBOX_WITH_DPC_LATENCY_CHECKER
/** @name VBGL_IOCTL_DPC_LATENCY_CHECKER
 * IOCTL to VBoxGuest to perform DPC latency tests, printing the result in
 * the release log on the host.  Takes no data, returns no data.
 * @{ */
# define VBGL_IOCTL_DPC_LATENCY_CHECKER             VBGL_IOCTL_CODE_SIZE(19, VBGL_IOCTL_DPC_LATENCY_CHECKER_SIZE)
# define VBGL_IOCTL_DPC_LATENCY_CHECKER_SIZE        sizeof(VBGLREQHDR)
# define VBGL_IOCTL_DPC_LATENCY_CHECKER_SIZE_IN     sizeof(VBGLREQHDR)
# define VBGL_IOCTL_DPC_LATENCY_CHECKER_SIZE_OUT    sizeof(VBGLREQHDR)
/** @} */
#endif


#ifdef RT_OS_OS2
/**
 * The data buffer layout for the IDC entry point (AttachDD).
 *
 * @remark  This is defined in multiple 16-bit headers / sources.
 *          Some places it's called VBGOS2IDC to short things a bit.
 */
typedef struct VBGLOS2ATTACHDD
{
    /** VBGL_IOC_VERSION. */
    uint32_t u32Version;
    /** Opaque session handle. */
    uint32_t u32Session;

    /**
     * The 32-bit service entry point.
     *
     * @returns VBox status code.
     * @param   u32Session          The session handle (PVBOXGUESTSESSION).
     * @param   iFunction           The requested function.
     * @param   pReqHdr             The input/output data buffer.  The caller
     *                              ensures that this cannot be swapped out, or that
     *                              it's acceptable to take a page in fault in the
     *                              current context.  If the request doesn't take
     *                              input or produces output, apssing NULL is okay.
     * @param   cbReq               The size of the data buffer.
     */
# if ARCH_BITS == 32 || defined(DOXYGEN_RUNNING)
    DECLCALLBACKMEMBER(int, pfnServiceEP)(uint32_t u32Session, unsigned iFunction, PVBGLREQHDR pReqHdr, size_t cbReq);
# else
    uint32_t pfnServiceEP;
#endif

    /** The 16-bit service entry point for C code (cdecl).
     *
     * It's the same as the 32-bit entry point, but the types has
     * changed to 16-bit equivalents.
     *
     * @code
     * int far cdecl
     * VBoxGuestOs2IDCService16(uint32_t u32Session, uint16_t iFunction,
     *                          PVBGLREQHDR fpvData, uint16_t cbData);
     * @endcode
     */
# if ARCH_BITS == 16 || defined(DOXYGEN_RUNNING)
    DECLCALLBACKMEMBER(int, fpfnServiceEP)(uint32_t u32Session, uint16_t iFunction, PVBGLREQHDR fpvData, uint16_t cbData);
# else
    RTFAR16 fpfnServiceEP;
# endif

    /** The 16-bit service entry point for Assembly code (register).
     *
     * This is just a wrapper around fpfnServiceEP to simplify calls
     * from 16-bit assembly code.
     *
     * @returns (e)ax: VBox status code; cx: The amount of data returned.
     *
     * @param   u32Session          eax   - The above session handle.
     * @param   iFunction           dl    - The requested function.
     * @param   pvData              es:bx - The input/output data buffer.
     * @param   cbData              cx    - The size of the data buffer.
     */
    RTFAR16 fpfnServiceAsmEP;
} VBGLOS2ATTACHDD;
/** Pointer to VBOXGUESTOS2IDCCONNECT buffer. */
typedef VBGLOS2ATTACHDD RT_FAR *PVBGLOS2ATTACHDD;

/**
 * Prototype for the 16-bit callback returned by AttachDD on OS/2.
 * @param   pAttachInfo     Pointer to structure to fill in.
 */
# if defined(__IBMC__) || defined(__IBMCPP__)
typedef void (* __cdecl RT_FAR_CODE PFNVBGLOS2ATTACHDD)(PVBGLOS2ATTACHDD pAttachInfo);
# else
typedef void (__cdecl RT_FAR_CODE *PFNVBGLOS2ATTACHDD)(PVBGLOS2ATTACHDD pAttachInfo);
# endif
#endif /* RT_OS_OS2 */


/** @name VBGL_IOCL_IDC_CONNECT
 * IDC client connect request.
 *
 * On platforms other than Windows and OS/2, this will also create a kernel
 * session for the caller.
 *
 * @note ring-0 only.
 */
#define VBGL_IOCTL_IDC_CONNECT                      VBGL_IOCTL_CODE_SIZE(63 | VBGL_IOCTL_FLAG_CC, VBGL_IOCTL_IDC_CONNECT_SIZE)
#define VBGL_IOCTL_IDC_CONNECT_SIZE                 sizeof(VBGLIOCIDCCONNECT)
#define VBGL_IOCTL_IDC_CONNECT_SIZE_IN              RT_UOFFSET_AFTER(VBGLIOCIDCCONNECT, u.In)
#define VBGL_IOCTL_IDC_CONNECT_SIZE_OUT             sizeof(VBGLIOCIDCCONNECT)
typedef struct VBGLIOCIDCCONNECT
{
    /** The header. */
    VBGLREQHDR          Hdr;
    /** The payload union. */
    union
    {
        struct
        {
            /** VBGL_IOCTL_IDC_CONNECT_MAGIC_COOKIE. */
            uint32_t        u32MagicCookie;
            /** The desired version of the I/O control interface (VBGL_IOC_VERSION). */
            uint32_t        uReqVersion;
            /** The minimum version of the I/O control interface (VBGL_IOC_VERSION). */
            uint32_t        uMinVersion;
            /** Reserved, MBZ. */
            uint32_t        uReserved;
        } In;
        struct
        {
            /** The session handle (opaque). */
#if ARCH_BITS >= 32
            void    RT_FAR *pvSession;
#else
            uint32_t        pvSession;
#endif
            /** The version of the I/O control interface for this session
             * (typically VBGL_IOC_VERSION). */
            uint32_t        uSessionVersion;
            /** The I/O control interface version for of the driver (VBGL_IOC_VERSION). */
            uint32_t        uDriverVersion;
            /** The SVN revision of the driver.
             * This will be set to 0 if not compiled into the driver. */
            uint32_t        uDriverRevision;
            /** Reserved \#1 (will be returned as zero until defined). */
            uint32_t        uReserved1;
            /** Reserved \#2 (will be returned as NULL until defined). */
            void    RT_FAR *pvReserved2;
        } Out;
    } u;
} VBGLIOCIDCCONNECT, RT_FAR *PVBGLIOCIDCCONNECT;
AssertCompileSize(VBGLIOCIDCCONNECT, 24 + 16 + (ARCH_BITS == 64 ? 8 : 4) * 2);
#ifndef __GNUC__ /* Some GCC versions can't handle the complicated RT_UOFFSET_AFTER macro, it seems. */
AssertCompile(VBGL_IOCTL_IDC_CONNECT_SIZE_IN == 24 + 16);
#endif
#define VBGL_IOCTL_IDC_CONNECT_MAGIC_COOKIE         UINT32_C(0x55aa4d5a) /**< Magic value for doing an IDC connect. */
/** @} */


/** @name VBGL_IOCL_IDC_DISCONNECT
 * IDC client disconnect request.
 *
 * This will destroy the kernel session associated with the IDC connection.
 *
 * @note ring-0 only.
 */
#define VBGL_IOCTL_IDC_DISCONNECT                   VBGL_IOCTL_CODE_SIZE(62 | VBGL_IOCTL_FLAG_CC, VBGL_IOCTL_IDC_DISCONNECT_SIZE)
#define VBGL_IOCTL_IDC_DISCONNECT_SIZE              sizeof(VBGLIOCIDCDISCONNECT)
#define VBGL_IOCTL_IDC_DISCONNECT_SIZE_IN           sizeof(VBGLIOCIDCDISCONNECT)
#define VBGL_IOCTL_IDC_DISCONNECT_SIZE_OUT          sizeof(VBGLREQHDR)
typedef struct VBGLIOCIDCDISCONNECT
{
    /** The header. */
    VBGLREQHDR          Hdr;
    union
    {
        struct
        {
            /** The session handle for platforms where this is needed. */
#if ARCH_BITS >= 32
            void RT_FAR *pvSession;
#else
            uint32_t     pvSession;
#endif
        } In;
    } u;
} VBGLIOCIDCDISCONNECT, RT_FAR *PVBGLIOCIDCDISCONNECT;
AssertCompileSize(VBGLIOCIDCDISCONNECT, 24 + (ARCH_BITS == 64 ? 8 : 4));
/** @} */


#if !defined(RT_OS_WINDOWS) && !defined(RT_OS_OS2)
RT_C_DECLS_BEGIN
/**
 * The VBoxGuest IDC entry point.
 *
 * @returns VBox status code.
 * @param   pvSession   The session.
 * @param   uReq        The request code.
 * @param   pReqHdr     The request.
 * @param   cbReq       The request size.
 */
int VBOXCALL VBoxGuestIDC(void RT_FAR *pvSession, uintptr_t uReq, PVBGLREQHDR pReqHdr, size_t cbReq);
RT_C_DECLS_END
#endif


#if defined(RT_OS_LINUX) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)

/* Private IOCtls between user space and the kernel video driver.  DRM private
 * IOCtls always have the type 'd' and a number between 0x40 and 0x99 (0x9F?) */

# define VBOX_DRM_IOCTL(a) (0x40 + DRM_VBOX_ ## a)

/** Stop using HGSMI in the kernel driver until it is re-enabled, so that a
 *  user-space driver can use it.  It must be re-enabled before the kernel
 *  driver can be used again in a sensible way. */
/** @note These IOCtls was removed from the code, but are left here as
 * templates as we may need similar ones in future. */
# define DRM_VBOX_DISABLE_HGSMI    0
# define DRM_IOCTL_VBOX_DISABLE_HGSMI    VBOX_DRM_IOCTL(DISABLE_HGSMI)
# define VBOXVIDEO_IOCTL_DISABLE_HGSMI   _IO('d', DRM_IOCTL_VBOX_DISABLE_HGSMI)
/** Enable HGSMI in the kernel driver after it was previously disabled. */
# define DRM_VBOX_ENABLE_HGSMI     1
# define DRM_IOCTL_VBOX_ENABLE_HGSMI     VBOX_DRM_IOCTL(ENABLE_HGSMI)
# define VBOXVIDEO_IOCTL_ENABLE_HGSMI    _IO('d', DRM_IOCTL_VBOX_ENABLE_HGSMI)

#endif /* RT_OS_LINUX || RT_OS_SOLARIS || RT_OS_FREEBSD */

#endif /* !defined(IN_RC) && !defined(IN_RING0_AGNOSTIC) && !defined(IPRT_NO_CRT) */

/** @} */

/** @} */
#endif

