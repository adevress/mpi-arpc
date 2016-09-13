# Perform all basic portability check for the BBP plateforms
#
#


include(CheckPrototypeDefinition)
include(BlueGenePortability)

# zlib prefix installation helper
# if ZLIB_ROOT env var is define, use it as searching path 
if((NOT DEFINED ZLIB_ROOT) AND (DEFINED  ENV{ZLIB_ROOT}) )

set(ZLIB_ROOT $ENV{ZLIB_ROOT} )

endif()


# portability check for strerror

check_prototype_definition(strerror_r
 "char* strerror_r(int errnum, char *buf, size_t buflen)"
 "NULL"
 "string.h"
 GNU_STRERROR_R)


check_prototype_definition(strerror_r
 "int strerror_r(int errnum, char *buf, size_t buflen)"
 "0"
 "string.h"
 POSIX_STRERROR_R)


