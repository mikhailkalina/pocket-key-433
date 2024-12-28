#include "signal.h"

// Invalid signal structure
static const Signal signalInvalid = {
    .protocol = SIGNAL_PROTOCOL_INV,
    .value = 0,
    .bitLength = 0,
};

/**
 * @brief Return invalid signal
 *
 * @return Reference invalid signal
 */
const Signal &SGNL_getInvalid()
{
    return signalInvalid;
}
