# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================

set(DEBUG_COMPILE_OPTIONS   ${COMPILE_OPTIONS} ${DEBUG_COMPILE_OPTIONS})
set(PROFILE_COMPILE_OPTIONS ${COMPILE_OPTIONS} ${PROFILE_COMPILE_OPTIONS})
set(RELEASE_COMPILE_OPTIONS ${COMPILE_OPTIONS} ${RELEASE_COMPILE_OPTIONS})

set(DEBUG_LINK_OPTIONS   ${LINK_OPTIONS} ${DEBUG_LINK_OPTIONS})
set(PROFILE_LINK_OPTIONS ${LINK_OPTIONS} ${PROFILE_LINK_OPTIONS})
set(RELEASE_LINK_OPTIONS ${LINK_OPTIONS} ${RELEASE_LINK_OPTIONS})

add_compile_options(
    "$<$<CONFIG:Debug>:${DEBUG_COMPILE_OPTIONS}>"
    "$<$<CONFIG:Profile>:${PROFILE_COMPILE_OPTIONS}>"
    "$<$<CONFIG:Release>:${RELEASE_COMPILE_OPTIONS}>"
)

add_link_options(
    "$<$<CONFIG:Debug>:${DEBUG_LINK_OPTIONS}>"
    "$<$<CONFIG:Profile>:${PROFILE_LINK_OPTIONS}>"
    "$<$<CONFIG:Release>:${RELEASE_LINK_OPTIONS}>"
)