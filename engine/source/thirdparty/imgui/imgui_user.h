// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#ifdef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS

// Define our filesystem functions.
namespace ws
{
    class stream;
};

typedef ws::stream* ImFileHandle;
ImFileHandle ImFileOpen(const char* filename, const char* mode);
bool         ImFileClose(ImFileHandle file);
ImU64        ImFileGetSize(ImFileHandle file);
ImU64        ImFileRead(void* data, ImU64 size, ImU64 count, ImFileHandle file);
ImU64        ImFileWrite(const void* data, ImU64 size, ImU64 count, ImFileHandle file);

#endif
