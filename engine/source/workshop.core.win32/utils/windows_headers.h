// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

// This is a stupid file that shouldn't exist. 

// Some of the windows header files need to be included in a very specific
// order due to poor include guarding resulting in redefition errors (specifically winsock2).
// So I'm keeping all windows incldues in here for now. This should eventually go into
// a precompiled header.

#define NOMINMAX

#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <ShlObj.h>
#include <shlwapi.h>
#include <winternl.h>
#include <mswsock.h>
#include <processthreadsapi.h>
#include <DbgEng.h>
#include <DbgHelp.h>