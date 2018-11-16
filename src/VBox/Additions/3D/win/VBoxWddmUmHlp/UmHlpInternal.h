/* $Id$ */
/** @file
 * VBox WDDM User Mode Driver Helpers
 */

/*
 * Copyright (C) 2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UmHlpInternal_h__
#define ___UmHlpInternal_h__

#include "VBoxWddmUmHlp.h"

NTSTATUS vboxDispKmtOpenAdapter2(D3DKMT_HANDLE *phAdapter, LUID *pLuid);
NTSTATUS vboxDispKmtOpenAdapter(D3DKMT_HANDLE *phAdapter);
NTSTATUS vboxDispKmtCloseAdapter(D3DKMT_HANDLE hAdapter);

#endif