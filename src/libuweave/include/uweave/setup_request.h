#ifndef LIBUWEAVE_INCLUDE_UWEAVE_SETUP_REQUEST_H_
#define LIBUWEAVE_INCLUDE_UWEAVE_SETUP_REQUEST_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct UwSetupRequest_ UwSetupRequest;

/**
 * Returns size of the name parameter or -1 if it is not been set.
 */
int uw_setup_request_get_name_length(UwSetupRequest* setup_request);

/**
 * Copies the name parameter to the specified character array which
 * will be zero terminated.  Returns true if the string fit into the buffer,
 * false if it was truncated.
 */
bool uw_setup_request_get_name(UwSetupRequest* setup_request,
                               char* name,
                               size_t length);

#endif  // LIBUWEAVE_INCLUDE_UWEAVE_SETUP_REQUEST_H_
