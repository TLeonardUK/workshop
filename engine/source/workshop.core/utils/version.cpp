// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/utils/version.h"
#include "workshop.core/containers/string.h"

// These values should be defined when building a production build, if they aren't defined
// we use some debugging defaults.

#ifndef WORKSHOP_VERSION_MAJOR 
#define WORKSHOP_VERSION_MAJOR 0
#endif
#ifndef WORKSHOP_VERSION_MINOR
#define WORKSHOP_VERSION_MINOR 1
#endif
#ifndef WORKSHOP_VERSION_REVISION
#define WORKSHOP_VERSION_REVISION 0
#endif
#ifndef WORKSHOP_VERSION_BUILD
#define WORKSHOP_VERSION_BUILD 0
#endif
#ifndef WORKSHOP_VERSION_CHANGESET
#define WORKSHOP_VERSION_CHANGESET "local"
#endif

namespace ws {

version_info get_version()
{
    version_info result;
    result.major = WORKSHOP_VERSION_MAJOR;
    result.minor = WORKSHOP_VERSION_MINOR;
    result.revision = WORKSHOP_VERSION_REVISION;
    result.build = WORKSHOP_VERSION_BUILD;
    result.changeset = WORKSHOP_VERSION_CHANGESET;
    result.string = string_format("%i.%i.%i.%i-%s", result.major, result.minor, result.revision, result.build, result.changeset.c_str());
    return result;
}

}; // namespace workshop
