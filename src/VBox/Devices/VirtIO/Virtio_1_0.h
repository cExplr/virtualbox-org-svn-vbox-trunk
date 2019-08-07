/* $Id$ */
/** @file
 * Virtio_1_0p .h - Virtio Declarations
 */

/*
 * Copyright (C) 2009-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_SRC_VirtIO_Virtio_1_0_h
#define VBOX_INCLUDED_SRC_VirtIO_Virtio_1_0_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/ctype.h>

/** @name Saved state versions.
 * The saved state version is changed if either common or any of specific
 * parts are changed. That is, it is perfectly possible that the version
 * of saved vnet state will increase as a result of change in vblk structure
 * for example.
 */
#define VIRTIO_SAVEDSTATE_VERSION_3_1_BETA1 1
#define VIRTIO_SAVEDSTATE_VERSION           2
/** @} */

/** Reserved Feature Bits */
#define VIRTIO_F_RING_INDIRECT_DESC         RT_BIT_64(28)
#define VIRTIO_F_RING_IDX                   RT_BIT_64(29)
#define VIRTIO_F_VERSION_1                  RT_BIT_64(32)

#define MAX_QUEUES                          5       /** Maximum number of queues we support     */

/** Mandatory for Virtio PCI device identification, per spec */
#define DEVICE_PCI_VENDOR_ID_VIRTIO         0x1AF4

/** Virtio spec suggests >= 1 for non-transitional drivers */
#define DEVICE_PCI_REVISION_ID_VIRTIO       1

/* Virtio Device PCI Capabilities type codes */
#define VIRTIO_PCI_CAP_COMMON_CFG           1
#define VIRTIO_PCI_CAP_NOTIFY_CFG           2
#define VIRTIO_PCI_CAP_ISR_CFG              3
#define VIRTIO_PCI_CAP_DEVICE_CFG           4
#define VIRTIO_PCI_CAP_PCI_CFG              5

/** Device Status field constants (from Virtio 1.0 spec) */
#define VIRTIO_STATUS_ACKNOWLEDGE           0x01    /** Guest found this emulated PCI device     */
#define VIRTIO_STATUS_DRIVER                0x02    /** Guest can drive this PCI device          */
#define VIRTIO_STATUS_DRIVER_OK             0x04    /** Guest VirtIO driver setup and ready      */
#define VIRTIO_STATUS_FEATURES_OK           0x08    /** Guest says feature negotiation complete  */
#define VIRTIO_STATUS_FAILED                0x80    /** Fatal: Something wrong, guest gave up    */
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET    0x40    /** Device experienced unrecoverable error   */

#define VIRTIO_MAX_NQUEUES                  256     /** Max queues we allow guest to create      */
#define VIRTQUEUEDESC_MAX_SIZE              (2 * 1024 * 1024)   /** Legacy only? */
#define VIRTQUEUE_MAX_SIZE                  1024    /** According to? Legacy only? */
#define VIRTIO_ISR_QUEUE                    0x1     /* Legacy only? Now derived from feature negotiation? */
#define VIRTIO_ISR_CONFIG                   0x3     /* Legacy only? Now derived from feature negotiation? */

#define VIRTIO_PCI_CAP_ID_VENDOR            0x09

/* Vector value used to disable MSI for queue */
#define VIRTIO_MSI_NO_VECTOR                0xffff

/** Most of the following definitions attempt to adhere to the names and
 *  and forms used by the VirtIO 1.0 specification to increase the maintainability
 *  and speed of ramp-up, in other words, to as unambiguously as possible
 *  map the spec to the VirtualBox implementation of it */

/** Indicates that this descriptor chains to another */
#define VIRTQ_DESC_F_NEXT                   1

/** Marks buffer as write-only (default ro) */
#define VIRTQ_DESC_F_WRITE                  2

/** This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT               4

/** Optimization: Device advises driver (unreliably):  Don't kick me when adding buffer */
#define VIRTQ_USED_F_NO_NOTIFY              1

/** Optimization: Driver advises device (unreliably): Don't interrupt when buffer consumed */
#define VIRTQ_AVAIL_F_NO_INTERRUPT          1

/** Using indirect descriptors */
#define VIRTIO_F_INDIRECT_DESC              28

/** Indicates either avail_event, or used_event fields are in play */
#define VIRTIO_F_EVENT_IDX                  29

/** Allow aribtary descriptor layouts */
#define VIRTIO_F_ANY_LAYOUT                 27

typedef struct VIRTIOPCIPARAMS
{
    uint16_t  uDeviceId;
    uint16_t  uClassBase;
    uint16_t  uClassSub;
    uint16_t  uClassProg;
    uint16_t  uSubsystemId;
    uint16_t  uSubsystemVendorId;
    uint16_t  uRevisionId;
    uint16_t  uInterruptLine;
    uint16_t  uInterruptPin;
} VIRTIOPCIPARAMS, *PVIRTIOPCIPARAMS;

typedef struct virtio_pci_cap
{
    /* All little-endian */
    uint8_t   uCapVndr;               /** Generic PCI field: PCI_CAP_ID_VNDR          */
    uint8_t   uCapNext;               /** Generic PCI field: next ptr.                */
    uint8_t   uCapLen;                /** Generic PCI field: capability length        */
    uint8_t   uCfgType;               /** Identifies the structure.                   */
    uint8_t   uBar;                   /** Where to find it.                           */
    uint8_t   uPadding[3];            /** Pad to full dword.                          */
    uint32_t  uOffset;                /** Offset within bar.  (L.E.)                  */
    uint32_t  uLength;                /** Length of struct, in bytes. (L.E.)          */
}  VIRTIO_PCI_CAP_T, *PVIRTIO_PCI_CAP_T;

typedef struct virtio_pci_common_cfg /* VirtIO 1.0 specification name of this struct  */
{
    /**
     *
     * RW = driver (guest) writes value into this field.
     * RO = device (host)  writes value into this field.
     */

    /* Per device fields */
    uint32_t  uDeviceFeaturesSelect;   /** RW (driver selects device features)         */
    uint32_t  uDeviceFeatures;         /** RO (device reports features to driver)      */
    uint32_t  uDriverFeaturesSelect;   /** RW (driver selects driver features)         */
    uint32_t  uDriverFeatures;         /** RW (driver accepts device features)         */
    uint16_t  uMsixConfig;            /** RW (driver sets MSI-X config vector)        */
    uint16_t  uNumQueues;             /** RO (device specifies max queues)            */
    uint8_t   uDeviceStatus;          /** RW (driver writes device status, 0 resets)  */
    uint8_t   uConfigGeneration;      /** RO (device changes when changing configs)   */

    /* Per virtqueue fields (as determined by uQueueSelect) */
    uint16_t  uQueueSelect;           /** RW (selects queue focus for these fields)   */
    uint16_t  uQueueSize;             /** RW (queue size, 0 - 2^n)                    */
    uint16_t  uQueueMsixVector;       /** RW (driver selects MSI-X queue vector)      */
    uint16_t  uQueueEnable;           /** RW (driver controls usability of queue)     */
    uint16_t  uQueueNotifyOff;        /** RO (offset uto virtqueue; see spec)         */
    uint64_t  uQueueDesc;             /** RW (driver writes desc table phys addr)     */
    uint64_t  uQueueAvail;            /** RW (driver writes avail ring phys addr)     */
    uint64_t  uQueueUsed;             /** RW (driver writes used ring  phys addr)     */
} VIRTIO_PCI_COMMON_CFG_T, *PVIRTIO_PCI_COMMON_CFG_T;

/* The following two types are opaque here. The device-specific capabilities are handled
 * through a callback to the consumer host device driver, where the corresponding
 * struct is declared, since it cannot be defined in the host-generic section of
 * the VirtIO specification. */

typedef void * PVIRTIO_PCI_DEVICE_CAP_T;
typedef struct virtio_pci_device_cap
{
} VIRTIO_PCI_DEVICE_CAP_T;

typedef struct virtio_pci_notify_cap
{
        struct virtio_pci_cap pciCap;
        uint32_t uNotifyOffMultiplier; /* notify_off_multiplier                        */
} VIRTIO_PCI_NOTIFY_CAP_T, *PVIRTIO_PCI_NOTIFY_CAP_T;

/*
 * If the client of this library has any device-specific capabilities, it must define
 * and implement this struct and the macro
 */
typedef struct virtio_pci_cfg_cap {
        struct virtio_pci_cap pciCap;
        uint8_t uPciCfgData[4]; /* Data for BAR access. */
} VIRTIO_PCI_CFG_CAP_T, *PVIRTIO_PCI_CFG_CAP_T;

typedef struct virtq_desc  /* VirtIO 1.0 specification formal name of this struct     */
{
    uint64_t  pGcPhysBuf;             /** addr        GC physical address of buffer   */
    uint32_t  cbBuf;                  /** len         Buffer length                   */
    uint16_t  fFlags;                 /** flags       Buffer specific flags           */
    uint16_t  uNext;                  /** next        Chain idx if VIRTIO_DESC_F_NEXT */
} VIRTQUEUEDESC, *PVIRTQUEUEDESC;

typedef struct virtq_avail  /* VirtIO 1.0 specification formal name of this struct    */
{
    uint16_t  fFlags;                 /** flags       avail ring guest-to-host flags */
    uint16_t  uIdx;                   /** idx         Index of next free ring slot    */
    uint16_t  auRing[1];              /** ring        Ring: avail guest-to-host bufs  */
    uint16_t  uUsedEventIdx;          /** used_event  (if VIRTQ_USED_F_NO_NOTIFY)     */
} VIRTQUEUEAVAIL, *PVIRTQUEUEAVAIL;

typedef struct virtq_used_elem /* VirtIO 1.0 specification formal name of this struct */
{
    uint32_t  uIdx;                   /** idx         Start of used descriptor chain  */
    uint32_t  cbElem;                 /** len         Total len of used desc chain    */
} VIRTQUEUEUSEDELEM;

typedef struct virt_used  /* VirtIO 1.0 specification formal name of this struct */
{
    uint16_t  fFlags;                 /** flags       used ring host-to-guest flags  */
    uint16_t  uIdx;                   /** idx         Index of next ring slot        */
    VIRTQUEUEUSEDELEM aRing[1];       /** ring        Ring: used host-to-guest bufs  */
    uint16_t  uAvailEventIdx;         /** avail_event if (VIRTQ_USED_F_NO_NOTIFY)    */
} VIRTQUEUEUSED, *PVIRTQUEUEUSED;


typedef struct virtq
{
    uint16_t cbQueue;                 /** virtq size                                 */
    uint16_t padding[3];              /** 64-bit-align phys ptrs to start of struct  */
    RTGCPHYS pGcPhysVirtqDescriptors; /** (struct virtq_desc  *)                     */
    RTGCPHYS pGcPhysVirtqAvail;       /** (struct virtq_avail *)                     */
    RTGCPHYS pGcPhysVirtqUsed;        /** (struct virtq_used  *)                     */
} VIRTQUEUE, *PVIRTQUEUE;

/**
 * Queue callback (consumer?).
 *
 * @param   pvState         Pointer to the VirtIO PCI core state, VIRTIOSTATE.
 * @param   pQueue          Pointer to the queue structure.
 */
typedef DECLCALLBACK(void) FNVIRTIOQUEUECALLBACK(void *pvState, struct VQueue *pQueue);
/** Pointer to a VQUEUE callback function. */
typedef FNVIRTIOQUEUECALLBACK *PFNVIRTIOQUEUECALLBACK;

//PCIDevGetCapabilityList
//PCIDevSetCapabilityList
//DECLINLINE(void) PDMPciDevSetCapabilityList(PPDMPCIDEV pPciDev, uint8_t u8Offset)
//    PDMPciDevSetByte(pPciDev, VBOX_PCI_CAPABILITY_LIST, u8Offset);
//DECLINLINE(uint8_t) PDMPciDevGetCapabilityList(PPDMPCIDEV pPciDev)
//    return PDMPciDevGetByte(pPciDev, VBOX_PCI_CAPABILITY_LIST);

typedef struct VQueue
{
    VIRTQUEUE    VirtQueue;
    uint16_t uNextAvailIndex;
    uint16_t uNextUsedIndex;
    uint32_t uPageNumber;
    R3PTRTYPE(PFNVIRTIOQUEUECALLBACK) pfnCallback;
    R3PTRTYPE(const char *)           pcszName;
} VQUEUE;
typedef VQUEUE *PVQUEUE;

typedef struct VQueueElemSeg
{
    RTGCPHYS addr;
    void    *pv;
    uint32_t cb;
} VQUEUESEG;

typedef struct VQueueElem
{
    uint32_t  idx;
    uint32_t  nIn;
    uint32_t  nOut;
    VQUEUESEG aSegsIn[VIRTQUEUE_MAX_SIZE];
    VQUEUESEG aSegsOut[VIRTQUEUE_MAX_SIZE];
} VQUEUEELEM;
typedef VQUEUEELEM *PVQUEUEELEM;


/**
 * Interface this library uses to let the client handle VirtioIO device-specific capabilities I/O
 */

 /**
 * VirtIO Device-specific capabilities read callback
 * (other VirtIO capabilities and features are handled in VirtIO implementation)
 *
 * @param   pDevIns     The device instance.
 * @param   offset      Offset within device specific capabilities struct
 * @param   pvBuf       Buffer in which to save read data
 * @param   cbWrite     Number of bytes to write
 */
typedef DECLCALLBACK(int)   FNVIRTIODEVCAPWRITE(PPDMDEVINS pDevIns, uint32_t uOffset, const void *pvBuf, size_t cbWrite);
typedef FNVIRTIODEVCAPWRITE *PFNVIRTIODEVCAPWRITE;

/**
 * VirtIO Device-specific capabilities read callback
 * (other VirtIO capabilities and features are handled in VirtIO implementation)
 *
 * @param   pDevIns     The device instance.
 * @param   offset      Offset within device specific capabilities struct
 * @param   pvBuf       Buffer in which to save read data
 * @param   cbRead      Number of bytes to read
 */
typedef DECLCALLBACK(int)   FNVIRTIODEVCAPREAD(PPDMDEVINS pDevIns, uint32_t uOffset, const void *pvBuf, size_t cbRead);
typedef FNVIRTIODEVCAPREAD *PFNVIRTIODEVCAPREAD;

/** @name VirtIO port I/O callbacks.
 * @{ */
typedef struct VIRTIOCALLBACKS
{
     DECLCALLBACKMEMBER(int,      pfnVirtioDevCapRead)
                                      (PPDMDEVINS pDevIns, uint32_t uOffset, const void *pvBuf, size_t cbRead);
     DECLCALLBACKMEMBER(int,      pfnVirtioDevCapWrite)
                                      (PPDMDEVINS pDevIns, uint32_t uOffset, const void *pvBuf, size_t cbWrite);
} VIRTIOCALLBACKS, *PVIRTALCALLBACKS;
/** @} */

/**
 * The core (/common) state of the VirtIO PCI device
 *
 * @implements  PDMILEDPORTS
 */
typedef struct VIRTIOSTATE
{
    PDMPCIDEV                    dev;
    PDMCRITSECT                  cs;                     /**< Critical section - what is it protecting? */
    /* Read-only part, never changes after initialization. */
    char                         szInstance[8];          /**< Instance name, e.g. VNet#1. */

#if HC_ARCH_BITS != 64
    uint32_t                     padding1;
#endif

    /** Status LUN: Base interface. */
    PDMIBASE                     IBase;

    /** Status LUN: LED port interface. */
    PDMILEDPORTS                 ILed;

    /* Read/write part, protected with critical section. */
    /** Status LED. */
    PDMLED                       led;

    VIRTIOCALLBACKS              virtioCallbacks;

    /** Status LUN: LED connector (peer). */
    R3PTRTYPE(PPDMILEDCONNECTORS) pLedsConnector;

    PPDMDEVINSR3                 pDevInsR3;              /**< Device instance - R3. */
    PPDMDEVINSR0                 pDevInsR0;              /**< Device instance - R0. */
    PPDMDEVINSRC                 pDevInsRC;              /**< Device instance - RC. */


    uint8_t                      uPciCfgDataOff;
#if HC_ARCH_BITS == 64
    uint32_t                     padding2;
#endif

    /** Base port of I/O space region. */
    RTIOPORT                     IOPortBase;

    uint32_t                     uGuestFeatures;
    uint16_t                     uQueueSelector;             /** An index in aQueues array. */
    uint8_t                      uStatus;                    /** Device Status (bits are device-specific). */
    uint8_t                      uISR;                       /** Interrupt Status Register. */

#if HC_ARCH_BITS != 64
    uint32_t                     padding3;
#endif

    uint32_t                     nQueues;                    /** Actual number of queues used. */
    VQUEUE                       Queues[VIRTIO_MAX_NQUEUES];

    uint32_t                     uDeviceFeaturesSelect;     /** Select hi/lo 32-bit uDeviceFeatures to r/w */
    uint64_t                     uDeviceFeatures;           /** Host features offered */
    uint32_t                     uDriverFeaturesSelect;     /** Selects hi/lo 32-bit uDriverFeatures to r/w*/
    uint64_t                     uDriverFeatures;           /** Host features accepted by guest */

    uint32_t                     uMsixConfig;
    uint8_t                      uDeviceStatus;
    uint8_t                      uConfigGeneration;
    uint32_t                     uNotifyOffMultiplier;       /* Multiplier for uQueueNotifyOff[idx] */

    uint16_t                     uQueueSelect;
    uint16_t                     uQueueSize[MAX_QUEUES];
    uint16_t                     uQueueMsixVector[MAX_QUEUES];
    uint16_t                     uQueueEnable[MAX_QUEUES];
    uint16_t                     uQueueNotifyOff[MAX_QUEUES];
    uint64_t                     uQueueDesc[MAX_QUEUES];
    uint64_t                     uQueueAvail[MAX_QUEUES];
    uint64_t                     uQueueUsed[MAX_QUEUES];


    uint8_t                      uVirtioCapRegion;      /* Client assigned  Virtio dedicated capabilities region*/

    /** Callbacks when guest driver reads or writes VirtIO device-specific capabilities(s) */
    PFNPCICONFIGREAD             pfnPciConfigReadOld;
    PFNPCICONFIGWRITE            pfnPciConfigWriteOld;

    bool                         fHaveDevSpecificCap;
    uint32_t                     cbDevSpecificCap;

    PVIRTIO_PCI_CFG_CAP_T        pPciCfgCap;            /** Pointer to struct in configuration area */
    PVIRTIO_PCI_NOTIFY_CAP_T     pNotifyCap;            /** Pointer to struct in configuration area */
    PVIRTIO_PCI_CAP_T            pCommonCfgCap;         /** Pointer to struct in configuration area */
    PVIRTIO_PCI_CAP_T            pIsrCap;               /** Pointer to struct in configuration area */
    PVIRTIO_PCI_CAP_T            pDeviceCap;            /** Pointer to struct in configuration area */

    /** Base address of PCI capabilities */
    RTGCPHYS                     GCPhysPciCapBase;
    RTGCPHYS                     pGcPhysCommonCfg;      /** Pointer to MMIO mapped capability data */
    RTGCPHYS                     pGcPhysNotifyCap;      /** Pointer to MMIO mapped capability data */
    RTGCPHYS                     pGcPhysIsrCap;         /** Pointer to MMIO mapped capability data */
    RTGCPHYS                     pGcPhysDeviceCap;      /** Pointer to MMIO mapped capability data */

    bool fDeviceConfigInterrupt;
    bool fQueueInterrupt;

} VIRTIOSTATE, *PVIRTIOSTATE;

void    virtioRelocate(PPDMDEVINS pDevIns, RTGCINTPTR offDelta);
void   *virtioQueryInterface(struct PDMIBASE *pInterface, const char *pszIID);

int     virtioConstruct(PPDMDEVINS pDevIns, PVIRTIOSTATE pVirtio, int iInstance, PVIRTIOPCIPARAMS pPciParams,
                    const char *pcszNameFmt, uint32_t nQueues, uint32_t uVirtioCapRegion,
                    PFNVIRTIODEVCAPREAD devCapReadCallback, PFNVIRTIODEVCAPWRITE devCapWriteCallback,
                    uint16_t cbDevSpecificCap, bool fHaveDevSpecificCap, uint32_t uNotifyOffMultiplier);

int     virtioDestruct(PVIRTIOSTATE pVirtio);
int     virtioReset(PVIRTIOSTATE pVirtio);
int     virtioSaveExec(PVIRTIOSTATE pVirtio, PSSMHANDLE pSSM);
int     virtioLoadExec(PVIRTIOSTATE pVirtio, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass, uint32_t nQueues);
void    virtioSetReadLed(PVIRTIOSTATE pVirtio, bool fOn);
void    virtioSetWriteLed(PVIRTIOSTATE pVirtio, bool fOn);
int     virtioRaiseInterrupt(PVIRTIOSTATE pVirtio, int rcBusy, uint8_t uint8_tIntCause);
void    virtioNotify(PVIRTIOSTATE pVirtio);
void    virtioSetHostFeatures(PVIRTIOSTATE pVirtio, uint64_t uDeviceFeatures);

PVQUEUE virtioAddQueue(PVIRTIOSTATE pVirtio, unsigned uSize, PFNVIRTIOQUEUECALLBACK pfnCallback, const char *pcszName);
bool    virtQueueSkip(PVIRTIOSTATE pVirtio, PVQUEUE pQueue);
bool    virtQueueGet(PVIRTIOSTATE pVirtio, PVQUEUE pQueue, PVQUEUEELEM pElem, bool fRemove = true);
void    virtQueuePut(PVIRTIOSTATE pVirtio, PVQUEUE pQueue, PVQUEUEELEM pElem, uint32_t len, uint32_t uReserved = 0);
void    virtQueueNotify(PVIRTIOSTATE pVirtio, PVQUEUE pQueue);
void    virtQueueSync(PVIRTIOSTATE pVirtio, PVQUEUE pQueue);
void    vringSetNotification( PVIRTIOSTATE pVirtio, PVIRTQUEUE pVirtQueue, bool fEnabled);


/*  FROM Virtio 1.0 SPEC, NYI
      static inline int virtq_need_event(uint16_t event_idx, uint16_t new_idx, uint16_t old_idx)
            return (uint16_t)(new_idx - event_idx - 1) < (uint16_t)(new_idx - old_idx);
      }
      // Get location of event indices (only with VIRTIO_F_EVENT_IDX)
      static inline le16 *virtq_used_event(struct virtq *vq)
      {
              // For backwards compat, used event index is at *end* of avail ring.
              return &vq->avail->ring[vq->num];
}
      static inline le16 *virtq_avail_event(struct virtq *vq)
      {
              // For backwards compat, avail event index is at *end* of used ring.
              return (le16 *)&vq->used->ring[vq->num];
      }
}
*/



/**
 * Formats the logging of a memory-mapped I/O input or output value
 *
 * @param   pszFunc     - To avoid displaying this function's name via __FUNCTION__ or LogFunc()
 * @param   pszMember   - Name of struct member
 * @param   pv          - pointer to value
 * @param   cb          - size of value
 * @param   uOffset     - offset into member where value starts
 * @param   fWrite      - True if write I/O
 * @param   fHasIndex   - True if the member is indexed
 * @param   idx         - The index if fHasIndex
 */
DECLINLINE(void) virtioLogMappedIoValue(const char *pszFunc, const char *pszMember, const void *pv, uint32_t cb,
                        uint32_t uOffset, bool fWrite, bool fHasIndex, uint32_t idx)
{

#define FMTHEX(fmtout, v, cNybs) \
    fmtout[cNybs] = '\0'; \
    for (uint8_t i = 0; i < cNybs; i++) \
        fmtout[(cNybs - i) -1] = "0123456789abcdef"[(val >> (i * 4)) & 0xf];

#define MAX_STRING   64
    char pszIdx[MAX_STRING] = { 0 };
    char pszDepiction[MAX_STRING] = { 0 };
    char pszFormattedVal[MAX_STRING] = { 0 };
    if (fHasIndex)
        RTStrPrintf(pszIdx, sizeof(pszIdx), "[%d]", idx);
    if (cb == 1 || cb == 2 || cb == 4 || cb == 8)
    {
        /* manually padding with 0's instead of \b due to different impl of %x precision than printf() */
        uint64_t val = 0;
        memcpy((char *)&val, pv, cb);
        FMTHEX(pszFormattedVal, val, cb * 2);
        if (uOffset != 0) /* display bounds if partial member access */
            RTStrPrintf(pszDepiction, sizeof(pszDepiction), "%s%s[%d:%d]", pszMember, pszIdx, uOffset, uOffset + cb - 1);
        else
            RTStrPrintf(pszDepiction, sizeof(pszDepiction), "%s%s", pszMember, pszIdx);
        RTStrPrintf(pszDepiction, sizeof(pszDepiction), "%-30s", pszDepiction);
        int first = 0;
        for (uint8_t i = 0; i < sizeof(pszDepiction); i++)
            if (pszDepiction[i] == ' ' && first++ != 0)
                pszDepiction[i] = '.';
        Log(("%s: Guest %s %s 0x%s\n", \
                  pszFunc, fWrite ? "wrote" : "read ", pszDepiction, pszFormattedVal));
    }
    else /* odd number or oversized access, ... log inline hex-dump style */
    {
        Log(("%s: Guest %s %s%s[%d:%d]: %.*Rhxs\n", \
              pszFunc, fWrite ? "wrote" : "read ", pszMember,
              pszIdx, uOffset, uOffset + cb, cb, pv));
    }
}

typedef DECLCALLBACK(uint32_t) FNGETHOSTFEATURES(void *pvState);
typedef FNGETHOSTFEATURES *PFNGETHOSTFEATURES;

DECLINLINE(uint16_t) vringReadAvail(PVIRTIOSTATE pState, PVIRTQUEUE pVirtQueue)
{
    uint16_t dataWord;
    PDMDevHlpPhysRead(pState->CTX_SUFF(pDevIns),
                      pVirtQueue->pGcPhysVirtqAvail + RT_UOFFSETOF(VIRTQUEUEAVAIL, uIdx),
                      &dataWord, sizeof(dataWord));
    return dataWord;
}

DECLINLINE(bool) virtQueuePeek(PVIRTIOSTATE pState, PVQUEUE pQueue, PVQUEUEELEM pElem)
{
    return virtQueueGet(pState, pQueue, pElem, /* fRemove */ false);
}

DECLINLINE(bool) virtQueueIsReady(PVIRTIOSTATE pState, PVQUEUE pQueue)
{
    NOREF(pState);
    return !!pQueue->VirtQueue.pGcPhysVirtqAvail;
}

DECLINLINE(bool) virtQueueIsEmpty(PVIRTIOSTATE pState, PVQUEUE pQueue)
{
    return (vringReadAvail(pState, &pQueue->VirtQueue) == pQueue->uNextAvailIndex);
}

#endif /* !VBOX_INCLUDED_SRC_VirtIO_Virtio_1_0_h */
