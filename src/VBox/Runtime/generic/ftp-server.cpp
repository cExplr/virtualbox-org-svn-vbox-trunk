/* $Id$ */
/** @file
 * Generic FTP server (RFC 959) implementation.
 */

/*
 * Copyright (C) 2020 Oracle Corporation
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

/**
 * Known limitations so far:
 * - UTF-8 support only.
 * - No support for writing / modifying ("DELE", "MKD", "RMD", "STOR", ++).
 * - No FTPS / SFTP support.
 * - No passive mode ("PASV") support.
 * - No proxy support.
 * - No FXP support.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP RTLOGGROUP_FTP
#include <iprt/ftp.h>
#include "internal/iprt.h"
#include "internal/magics.h"

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/errcore.h>
#include <iprt/getopt.h>
#include <iprt/mem.h>
#include <iprt/log.h>
#include <iprt/path.h>
#include <iprt/poll.h>
#include <iprt/socket.h>
#include <iprt/string.h>
#include <iprt/system.h>
#include <iprt/tcp.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Internal FTP server instance.
 */
typedef struct RTFTPSERVERINTERNAL
{
    /** Magic value. */
    uint32_t                u32Magic;
    /** Callback table. */
    RTFTPSERVERCALLBACKS    Callbacks;
    /** Pointer to TCP server instance. */
    PRTTCPSERVER            pTCPServer;
    /** Number of currently connected clients. */
    uint32_t                cClients;
} RTFTPSERVERINTERNAL;
/** Pointer to an internal FTP server instance. */
typedef RTFTPSERVERINTERNAL *PRTFTPSERVERINTERNAL;


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Validates a handle and returns VERR_INVALID_HANDLE if not valid. */
#define RTFTPSERVER_VALID_RETURN_RC(hFTPServer, a_rc) \
    do { \
        AssertPtrReturn((hFTPServer), (a_rc)); \
        AssertReturn((hFTPServer)->u32Magic == RTFTPSERVER_MAGIC, (a_rc)); \
    } while (0)

/** Validates a handle and returns VERR_INVALID_HANDLE if not valid. */
#define RTFTPSERVER_VALID_RETURN(hFTPServer) RTFTPSERVER_VALID_RETURN_RC((hFTPServer), VERR_INVALID_HANDLE)

/** Validates a handle and returns (void) if not valid. */
#define RTFTPSERVER_VALID_RETURN_VOID(hFTPServer) \
    do { \
        AssertPtrReturnVoid(hFTPServer); \
        AssertReturnVoid((hFTPServer)->u32Magic == RTFTPSERVER_MAGIC); \
    } while (0)

/** Supported FTP server command IDs.
 *  Alphabetically, named after their official command names. */
typedef enum RTFTPSERVER_CMD
{
    /** Invalid command, do not use. Always must come first. */
    RTFTPSERVER_CMD_INVALID = 0,
    /** Aborts the current command on the server. */
    RTFTPSERVER_CMD_ABOR,
    /** Changes the current working directory. */
    RTFTPSERVER_CMD_CDUP,
    /** Changes the current working directory. */
    RTFTPSERVER_CMD_CWD,
    /** Lists a directory. */
    RTFTPSERVER_CMD_LIST,
    /** Sets the transfer mode. */
    RTFTPSERVER_CMD_MODE,
    /** Sends a nop ("no operation") to the server. */
    RTFTPSERVER_CMD_NOOP,
    /** Sets the password for authentication. */
    RTFTPSERVER_CMD_PASS,
    /** Sets the port to use for the data connection. */
    RTFTPSERVER_CMD_PORT,
    /** Gets the current working directory. */
    RTFTPSERVER_CMD_PWD,
    /** Terminates the session (connection). */
    RTFTPSERVER_CMD_QUIT,
    /** Retrieves a specific file. */
    RTFTPSERVER_CMD_RETR,
    /** Recursively gets a directory (and its contents). */
    RTFTPSERVER_CMD_RGET,
    /** Retrieves the current status of a transfer. */
    RTFTPSERVER_CMD_STAT,
    /** Gets the server's OS info. */
    RTFTPSERVER_CMD_SYST,
    /** Sets the (data) representation type. */
    RTFTPSERVER_CMD_TYPE,
    /** Sets the user name for authentication. */
    RTFTPSERVER_CMD_USER,
    /** End marker. */
    RTFTPSERVER_CMD_LAST,
    /** The usual 32-bit hack. */
    RTFTPSERVER_CMD_32BIT_HACK = 0x7fffffff
} RTFTPSERVER_CMD;

/**
 * Structure for maintaining an internal FTP server client.
 */
typedef struct RTFTPSERVERCLIENT
{
    /** Pointer to internal server state. */
    PRTFTPSERVERINTERNAL        pServer;
    /** Socket handle the client is bound to. */
    RTSOCKET                    hSocket;
    /** Actual client state. */
    RTFTPSERVERCLIENTSTATE      State;
} RTFTPSERVERCLIENT;
/** Pointer to an internal FTP server client state. */
typedef RTFTPSERVERCLIENT *PRTFTPSERVERCLIENT;

/** Function pointer declaration for a specific FTP server command handler. */
typedef DECLCALLBACK(int) FNRTFTPSERVERCMD(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs);
/** Pointer to a FNRTFTPSERVERCMD(). */
typedef FNRTFTPSERVERCMD *PFNRTFTPSERVERCMD;

/** Handles a FTP server callback with no arguments and returns. */
#define RTFTPSERVER_HANDLE_CALLBACK_RET(a_Name) \
    do \
    { \
        PRTFTPSERVERCALLBACKS pCallbacks = &pClient->pServer->Callbacks; \
        if (pCallbacks->a_Name) \
        { \
            RTFTPCALLBACKDATA Data = { &pClient->State, pCallbacks->pvUser, pCallbacks->cbUser }; \
            return pCallbacks->a_Name(&Data); \
        } \
        else \
            return VERR_NOT_IMPLEMENTED; \
    } while (0)

/** Handles a FTP server callback with no arguments and sets rc accordingly. */
#define RTFTPSERVER_HANDLE_CALLBACK(a_Name) \
    do \
    { \
        PRTFTPSERVERCALLBACKS pCallbacks = &pClient->pServer->Callbacks; \
        if (pCallbacks->a_Name) \
        { \
            RTFTPCALLBACKDATA Data = { &pClient->State, pCallbacks->pvUser, pCallbacks->cbUser }; \
            rc = pCallbacks->a_Name(&Data); \
        } \
        else \
            rc = VERR_NOT_IMPLEMENTED; \
    } while (0)

/** Handles a FTP server callback with arguments and sets rc accordingly. */
#define RTFTPSERVER_HANDLE_CALLBACK_VA(a_Name, ...) \
    do \
    { \
        PRTFTPSERVERCALLBACKS pCallbacks = &pClient->pServer->Callbacks; \
        if (pCallbacks->a_Name) \
        { \
            RTFTPCALLBACKDATA Data = { &pClient->State, pCallbacks->pvUser, pCallbacks->cbUser }; \
            rc = pCallbacks->a_Name(&Data, __VA_ARGS__); \
        } \
        else \
            rc = VERR_NOT_IMPLEMENTED; \
    } while (0)

/** Handles a FTP server callback with arguments and returns. */
#define RTFTPSERVER_HANDLE_CALLBACK_VA_RET(a_Name, ...) \
    do \
    { \
        PRTFTPSERVERCALLBACKS pCallbacks = &pClient->pServer->Callbacks; \
        if (pCallbacks->a_Name) \
        { \
            RTFTPCALLBACKDATA Data = { &pClient->State, pCallbacks->pvUser, pCallbacks->cbUser }; \
            return pCallbacks->a_Name(&Data, __VA_ARGS__); \
        } \
        else \
            return VERR_NOT_IMPLEMENTED; \
    } while (0)

/**
 * Function prototypes for command handlers.
 */
static FNRTFTPSERVERCMD rtFtpServerHandleABOR;
static FNRTFTPSERVERCMD rtFtpServerHandleCDUP;
static FNRTFTPSERVERCMD rtFtpServerHandleCWD;
static FNRTFTPSERVERCMD rtFtpServerHandleLIST;
static FNRTFTPSERVERCMD rtFtpServerHandleMODE;
static FNRTFTPSERVERCMD rtFtpServerHandleNOOP;
static FNRTFTPSERVERCMD rtFtpServerHandlePASS;
static FNRTFTPSERVERCMD rtFtpServerHandlePORT;
static FNRTFTPSERVERCMD rtFtpServerHandlePWD;
static FNRTFTPSERVERCMD rtFtpServerHandleQUIT;
static FNRTFTPSERVERCMD rtFtpServerHandleRETR;
static FNRTFTPSERVERCMD rtFtpServerHandleRGET;
static FNRTFTPSERVERCMD rtFtpServerHandleSTAT;
static FNRTFTPSERVERCMD rtFtpServerHandleSYST;
static FNRTFTPSERVERCMD rtFtpServerHandleTYPE;
static FNRTFTPSERVERCMD rtFtpServerHandleUSER;

/**
 * Structure for maintaining a single command entry for the command table.
 */
typedef struct RTFTPSERVER_CMD_ENTRY
{
    /** Command ID. */
    RTFTPSERVER_CMD    enmCmd;
    /** Command represented as ASCII string. */
    char               szCmd[RTFTPSERVER_MAX_CMD_LEN];
    /** Function pointer invoked to handle the command. */
    PFNRTFTPSERVERCMD  pfnCmd;
} RTFTPSERVER_CMD_ENTRY;

/**
 * Table of handled commands.
 */
const RTFTPSERVER_CMD_ENTRY g_aCmdMap[] =
{
    { RTFTPSERVER_CMD_ABOR,     "ABOR",         rtFtpServerHandleABOR },
    { RTFTPSERVER_CMD_CDUP,     "CDUP",         rtFtpServerHandleCDUP },
    { RTFTPSERVER_CMD_CWD,      "CWD",          rtFtpServerHandleCWD  },
    { RTFTPSERVER_CMD_LIST,     "LIST",         rtFtpServerHandleLIST },
    { RTFTPSERVER_CMD_MODE,     "MODE",         rtFtpServerHandleMODE },
    { RTFTPSERVER_CMD_NOOP,     "NOOP",         rtFtpServerHandleNOOP },
    { RTFTPSERVER_CMD_PASS,     "PASS",         rtFtpServerHandlePASS },
    { RTFTPSERVER_CMD_PORT,     "PORT",         rtFtpServerHandlePORT },
    { RTFTPSERVER_CMD_PWD,      "PWD",          rtFtpServerHandlePWD  },
    { RTFTPSERVER_CMD_QUIT,     "QUIT",         rtFtpServerHandleQUIT },
    { RTFTPSERVER_CMD_RETR,     "RETR",         rtFtpServerHandleRETR },
    { RTFTPSERVER_CMD_RGET,     "RGET",         rtFtpServerHandleRGET },
    { RTFTPSERVER_CMD_STAT,     "STAT",         rtFtpServerHandleSTAT },
    { RTFTPSERVER_CMD_SYST,     "SYST",         rtFtpServerHandleSYST },
    { RTFTPSERVER_CMD_TYPE,     "TYPE",         rtFtpServerHandleTYPE },
    { RTFTPSERVER_CMD_USER,     "USER",         rtFtpServerHandleUSER },
    { RTFTPSERVER_CMD_LAST,     "",             NULL }
};


/*********************************************************************************************************************************
*   Protocol Functions                                                                                                           *
*********************************************************************************************************************************/

/**
 * Replies a (three digit) reply code back to the client.
 *
 * @returns VBox status code.
 * @param   pClient             Client to reply to.
 * @param   enmReply            Reply code to send.
 */
static int rtFtpServerSendReplyRc(PRTFTPSERVERCLIENT pClient, RTFTPSERVER_REPLY enmReply)
{
    char szReply[32];
    RTStrPrintf2(szReply, sizeof(szReply), "%RU32\r\n", enmReply);

    return RTTcpWrite(pClient->hSocket, szReply, strlen(szReply) + 1);
}

/**
 * Replies a string back to the client.
 *
 * @returns VBox status code.
 * @param   pClient             Client to reply to.
 * @param   pcszStr             String to reply.
 */
static int rtFtpServerSendReplyStr(PRTFTPSERVERCLIENT pClient, const char *pcszStr)
{
    char *pszReply;
    int rc = RTStrAPrintf(&pszReply, "%s\r\n", pcszStr);
    if (RT_SUCCESS(rc))
    {
        rc = RTTcpWrite(pClient->hSocket, pszReply, strlen(pszReply) + 1);
        RTStrFree(pszReply);
        return rc;
    }

    return VERR_NO_MEMORY;
}

/**
 * Looks up an user account.
 *
 * @returns VBox status code, or VERR_NOT_FOUND if user has not been found.
 * @param   pClient             Client to look up user for.
 * @param   pcszUser            User name to look up.
 */
static int rtFtpServerLookupUser(PRTFTPSERVERCLIENT pClient, const char *pcszUser)
{
    RTFTPSERVER_HANDLE_CALLBACK_VA_RET(pfnOnUserConnect, pcszUser);
}

/**
 * Handles the actual client authentication.
 *
 * @returns VBox status code, or VERR_ACCESS_DENIED if authentication failed.
 * @param   pClient             Client to authenticate.
 * @param   pcszUser            User name to authenticate with.
 * @param   pcszPassword        Password to authenticate with.
 */
static int rtFtpServerAuthenticate(PRTFTPSERVERCLIENT pClient, const char *pcszUser, const char *pcszPassword)
{
    RTFTPSERVER_HANDLE_CALLBACK_VA_RET(pfnOnUserAuthenticate, pcszUser, pcszPassword);
}


/*********************************************************************************************************************************
*   Command Protocol Handlers                                                                                                    *
*********************************************************************************************************************************/

static int rtFtpServerHandleABOR(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    RT_NOREF(pClient, cArgs, apcszArgs);

    /** @todo Anything to do here? */
    return VERR_NOT_IMPLEMENTED;
}

static int rtFtpServerHandleCDUP(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    RT_NOREF(cArgs, apcszArgs);

    int rc;

    RTFTPSERVER_HANDLE_CALLBACK(pfnOnPathUp);

    if (RT_SUCCESS(rc))
        return rtFtpServerSendReplyRc(pClient, RTFTPSERVER_REPLY_OKAY);

    return rc;
}

static int rtFtpServerHandleCWD(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    if (cArgs != 1)
        return VERR_INVALID_PARAMETER;

    int rc;

    RTFTPSERVER_HANDLE_CALLBACK_VA(pfnOnPathSetCurrent, apcszArgs[0]);

    if (RT_SUCCESS(rc))
        return rtFtpServerSendReplyRc(pClient, RTFTPSERVER_REPLY_OKAY);

    return rc;
}

static int rtFtpServerHandleLIST(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    RT_NOREF(cArgs, apcszArgs);

    int rc;

    void   *pvData = NULL;
    size_t  cbData = 0;

    /* The first argument might indicate a directory to list. */
    RTFTPSERVER_HANDLE_CALLBACK_VA(pfnOnList,
                                     cArgs == 1
                                   ? apcszArgs[0] : NULL, &pvData, &cbData);

    if (RT_SUCCESS(rc))
    {
        RTMemFree(pvData);
    }

    return rc;
}

static int rtFtpServerHandleMODE(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    RT_NOREF(pClient, cArgs, apcszArgs);

    /** @todo Anything to do here? */
    return VINF_SUCCESS;
}

static int rtFtpServerHandleNOOP(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    RT_NOREF(cArgs, apcszArgs);

    /* Save timestamp of last command sent. */
    pClient->State.tsLastCmdMs = RTTimeMilliTS();

    return rtFtpServerSendReplyRc(pClient, RTFTPSERVER_REPLY_OKAY);
}

static int rtFtpServerHandlePASS(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    if (cArgs != 1)
        return rtFtpServerSendReplyRc(pClient, RTFTPSERVER_REPLY_ERROR_INVALID_PARAMETERS);

    const char *pcszPassword = apcszArgs[0];
    AssertPtrReturn(pcszPassword, VERR_INVALID_PARAMETER);

    int rc = rtFtpServerAuthenticate(pClient, pClient->State.pszUser, pcszPassword);
    if (RT_SUCCESS(rc))
    {
        rc = rtFtpServerSendReplyRc(pClient, RTFTPSERVER_REPLY_LOGGED_IN_PROCEED);
    }
    else
    {
        pClient->State.cFailedLoginAttempts++;

        int rc2 = rtFtpServerSendReplyRc(pClient, RTFTPSERVER_REPLY_NOT_LOGGED_IN);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    return rc;
}

static int rtFtpServerHandlePORT(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    RT_NOREF(pClient, cArgs, apcszArgs);

    /** @todo Anything to do here? */
    return VERR_NOT_IMPLEMENTED;
}

static int rtFtpServerHandlePWD(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    RT_NOREF(cArgs, apcszArgs);

    int rc;

    char szPWD[RTPATH_MAX];

    RTFTPSERVER_HANDLE_CALLBACK_VA(pfnOnPathGetCurrent, szPWD, sizeof(szPWD));

    if (RT_SUCCESS(rc))
       rc = rtFtpServerSendReplyStr(pClient, szPWD);

    return rc;
}

static int rtFtpServerHandleQUIT(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    RT_NOREF(pClient, cArgs, apcszArgs);

    /** @todo Anything to do here? */
    return VERR_NOT_IMPLEMENTED;
}

static int rtFtpServerHandleRETR(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    RT_NOREF(pClient, cArgs, apcszArgs);

    /** @todo Anything to do here? */
    return VERR_NOT_IMPLEMENTED;
}

static int rtFtpServerHandleRGET(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    RT_NOREF(pClient, cArgs, apcszArgs);

    /** @todo Anything to do here? */
    return VERR_NOT_IMPLEMENTED;
}

static int rtFtpServerHandleSTAT(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    RT_NOREF(pClient, cArgs, apcszArgs);

    /** @todo Anything to do here? */
    return VERR_NOT_IMPLEMENTED;
}

static int rtFtpServerHandleSYST(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    RT_NOREF(cArgs, apcszArgs);

    char szOSInfo[64];
    int rc = RTSystemQueryOSInfo(RTSYSOSINFO_PRODUCT, szOSInfo, sizeof(szOSInfo));
    if (RT_SUCCESS(rc))
        rc = rtFtpServerSendReplyStr(pClient, szOSInfo);

    return rc;
}

static int rtFtpServerHandleTYPE(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    RT_NOREF(pClient, cArgs, apcszArgs);

    /** @todo Anything to do here? */
    return VERR_NOT_IMPLEMENTED;
}

static int rtFtpServerHandleUSER(PRTFTPSERVERCLIENT pClient, uint8_t cArgs, const char * const *apcszArgs)
{
    if (cArgs != 1)
        return VERR_INVALID_PARAMETER;

    const char *pcszUser = apcszArgs[0];
    AssertPtrReturn(pcszUser, VERR_INVALID_PARAMETER);

    if (pClient->State.pszUser)
    {
        RTStrFree(pClient->State.pszUser);
        pClient->State.pszUser = NULL;
    }

    int rc = rtFtpServerLookupUser(pClient, pcszUser);
    if (RT_SUCCESS(rc))
    {
        pClient->State.pszUser = RTStrDup(pcszUser);
        AssertPtrReturn(pClient->State.pszUser, VERR_NO_MEMORY);

        rc = rtFtpServerSendReplyRc(pClient, RTFTPSERVER_REPLY_USERNAME_OKAY_NEED_PASSWORD);
    }
    else
    {
        pClient->State.cFailedLoginAttempts++;

        int rc2 = rtFtpServerSendReplyRc(pClient, RTFTPSERVER_REPLY_NOT_LOGGED_IN);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    return rc;
}


/*********************************************************************************************************************************
*   Internal server functions                                                                                                    *
*********************************************************************************************************************************/

/**
 * Parses FTP command arguments handed in by the client.
 *
 * @returns VBox status code.
 * @param   pcszCmdParms        Pointer to command arguments, if any. Can be NULL if no arguments are given.
 * @param   pcArgs              Returns the number of parsed arguments, separated by a space (hex 0x20).
 * @param   ppapcszArgs         Returns the string array of parsed arguments. Needs to be free'd with rtFtpServerCmdArgsFree().
 */
static int rtFtpServerCmdArgsParse(const char *pcszCmdParms, uint8_t *pcArgs, char ***ppapcszArgs)
{
    *pcArgs      = 0;
    *ppapcszArgs = NULL;

    if (!pcszCmdParms) /* No parms given? Bail out early. */
        return VINF_SUCCESS;

    /** @todo Anything else to do here? */
    /** @todo Check if quoting is correct. */

    int cArgs = 0;
    int rc = RTGetOptArgvFromString(ppapcszArgs, &cArgs, pcszCmdParms, RTGETOPTARGV_CNV_QUOTE_MS_CRT, " " /* Separators */);
    if (RT_SUCCESS(rc))
    {
        if (cArgs <= UINT8_MAX)
        {
            *pcArgs = (uint8_t)cArgs;
        }
        else
            rc = VERR_INVALID_PARAMETER;
    }

    return rc;
}

/**
 * Frees a formerly argument string array parsed by rtFtpServerCmdArgsParse().
 *
 * @param   ppapcszArgs         Argument string array to free.
 */
static void rtFtpServerCmdArgsFree(char **ppapcszArgs)
{
    RTGetOptArgvFree(ppapcszArgs);
}

/**
 * Main loop for processing client commands.
 *
 * @returns VBox status code.
 * @param   pClient             Client to process commands for.
 */
static int rtFtpServerProcessCommands(PRTFTPSERVERCLIENT pClient)
{
    int rc;

    for (;;)
    {
        size_t cbRead;
        char   szCmd[RTFTPSERVER_MAX_CMD_LEN];
        rc = RTTcpRead(pClient->hSocket, szCmd, sizeof(szCmd), &cbRead);
        if (RT_SUCCESS(rc))
        {
            /* Make sure to terminate the string in any case. */
            szCmd[RTFTPSERVER_MAX_CMD_LEN - 1] = '\0';

            /* A tiny bit of sanitation. */
            RTStrStripL(szCmd);

            /* First, terminate string by finding the command end marker (telnet style). */
            /** @todo Not sure if this is entirely correct and/or needs tweaking; good enough for now as it seems. */
            char *pszCmdEnd = RTStrIStr(szCmd, "\r\n");
            if (pszCmdEnd)
                *pszCmdEnd = '\0';

            int rcCmd = VINF_SUCCESS;

            uint8_t cArgs     = 0;
            char  **papszArgs = NULL;
            rc = rtFtpServerCmdArgsParse(szCmd, &cArgs, &papszArgs);
            if (   RT_SUCCESS(rc)
                && cArgs) /* At least the actual command (without args) must be present. */
            {
                unsigned i = 0;
                for (; i < RT_ELEMENTS(g_aCmdMap); i++)
                {
                    if (!RTStrICmp(papszArgs[0], g_aCmdMap[i].szCmd))
                    {
                        /* Save timestamp of last command sent. */
                        pClient->State.tsLastCmdMs = RTTimeMilliTS();

                        rcCmd = g_aCmdMap[i].pfnCmd(pClient, cArgs - 1, cArgs > 1 ? &papszArgs[1] : NULL);
                        break;
                    }
                }

                rtFtpServerCmdArgsFree(papszArgs);

                if (i == RT_ELEMENTS(g_aCmdMap))
                {
                    int rc2 = rtFtpServerSendReplyRc(pClient, RTFTPSERVER_REPLY_ERROR_CMD_NOT_IMPL);
                    if (RT_SUCCESS(rc))
                        rc = rc2;

                    continue;
                }

                const bool fDisconnect =    g_aCmdMap[i].enmCmd == RTFTPSERVER_CMD_QUIT
                                         || pClient->State.cFailedLoginAttempts >= 3; /** @todo Make this dynamic. */
                if (fDisconnect)
                {
                    int rc2 = rtFtpServerSendReplyRc(pClient, RTFTPSERVER_REPLY_CLOSING_CTRL_CONN);
                    if (RT_SUCCESS(rc))
                        rc = rc2;

                    RTFTPSERVER_HANDLE_CALLBACK(pfnOnUserDisconnect);
                    break;
                }

                switch (rcCmd)
                {
                    case VERR_INVALID_PARAMETER:
                        RT_FALL_THROUGH();
                    case VERR_INVALID_POINTER:
                        rc = rtFtpServerSendReplyRc(pClient, RTFTPSERVER_REPLY_ERROR_INVALID_PARAMETERS);
                        break;

                    case VERR_NOT_IMPLEMENTED:
                        rc = rtFtpServerSendReplyRc(pClient, RTFTPSERVER_REPLY_ERROR_CMD_NOT_IMPL);
                        break;

                    default:
                        break;
                }
            }
            else
            {
                int rc2 = rtFtpServerSendReplyRc(pClient, RTFTPSERVER_REPLY_ERROR_INVALID_PARAMETERS);
                if (RT_SUCCESS(rc))
                    rc = rc2;
            }
        }
        else
        {
            int rc2 = rtFtpServerSendReplyRc(pClient, RTFTPSERVER_REPLY_ERROR_CMD_NOT_RECOGNIZED);
            if (RT_SUCCESS(rc))
                rc = rc2;
        }
    }

    return rc;
}

/**
 * Resets the client's state.
 *
 * @param   pState              Client state to reset.
 */
static void rtFtpServerClientStateReset(PRTFTPSERVERCLIENTSTATE pState)
{
    RTStrFree(pState->pszUser);
    pState->pszUser = NULL;

    pState->tsLastCmdMs = RTTimeMilliTS();
}

/**
 * Per-client thread for serving the server's control connection.
 *
 * @returns VBox status code.
 * @param   hSocket             Socket handle to use for the control connection.
 * @param   pvUser              User-provided arguments. Of type PRTFTPSERVERINTERNAL.
 */
static DECLCALLBACK(int) rtFtpServerClientThread(RTSOCKET hSocket, void *pvUser)
{
    PRTFTPSERVERINTERNAL pThis = (PRTFTPSERVERINTERNAL)pvUser;
    RTFTPSERVER_VALID_RETURN(pThis);

    RTFTPSERVERCLIENT Client;
    RT_ZERO(Client);

    Client.pServer     = pThis;
    Client.hSocket     = hSocket;

    rtFtpServerClientStateReset(&Client.State);

    /* Send welcome message. */
    int rc = rtFtpServerSendReplyRc(&Client, RTFTPSERVER_REPLY_READY_FOR_NEW_USER);
    if (RT_SUCCESS(rc))
    {
        ASMAtomicIncU32(&pThis->cClients);

        rc = rtFtpServerProcessCommands(&Client);

        ASMAtomicDecU32(&pThis->cClients);
    }

    rtFtpServerClientStateReset(&Client.State);

    return rc;
}

RTR3DECL(int) RTFtpServerCreate(PRTFTPSERVER phFTPServer, const char *pcszAddress, uint16_t uPort,
                                PRTFTPSERVERCALLBACKS pCallbacks)
{
    AssertPtrReturn(phFTPServer,  VERR_INVALID_POINTER);
    AssertPtrReturn(pcszAddress,  VERR_INVALID_POINTER);
    AssertReturn   (uPort,        VERR_INVALID_PARAMETER);
    AssertPtrReturn(pCallbacks,   VERR_INVALID_POINTER);

    int rc;

    PRTFTPSERVERINTERNAL pThis = (PRTFTPSERVERINTERNAL)RTMemAllocZ(sizeof(RTFTPSERVERINTERNAL));
    if (pThis)
    {
        pThis->u32Magic  = RTFTPSERVER_MAGIC;
        pThis->Callbacks = *pCallbacks;

        rc = RTTcpServerCreate(pcszAddress, uPort, RTTHREADTYPE_DEFAULT, "ftpsrv",
                               rtFtpServerClientThread, pThis /* pvUser */, &pThis->pTCPServer);
        if (RT_SUCCESS(rc))
        {
            *phFTPServer = (RTFTPSERVER)pThis;
        }
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

RTR3DECL(int) RTFtpServerDestroy(RTFTPSERVER hFTPServer)
{
    if (hFTPServer == NIL_RTFTPSERVER)
        return VINF_SUCCESS;

    PRTFTPSERVERINTERNAL pThis = hFTPServer;
    RTFTPSERVER_VALID_RETURN(pThis);

    AssertPtr(pThis->pTCPServer);

    int rc = RTTcpServerDestroy(pThis->pTCPServer);
    if (RT_SUCCESS(rc))
    {
        pThis->u32Magic = RTFTPSERVER_MAGIC_DEAD;

        RTMemFree(pThis);
    }

    return rc;
}

