# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================

# Platform type define.
set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -DWS_LINUX)

set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -Werror)

set(COMPILE_OPTIONS ${COMPILE_OPTIONS} $<$<COMPILE_LANGUAGE:CXX>:-Wno-invalid-offsetof>)

