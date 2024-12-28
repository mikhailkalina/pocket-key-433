#define SIGNAL_PROTOCOL_INV (0)
#define SIGNAL_PROTOCOL_MIN (1)
#define SIGNAL_PROTOCOL_MAX (12)

/**
 * @brief Radio signal structure
 */
struct Signal
{
    unsigned int protocol;
    unsigned long value;
    unsigned int bitLength;
};

/**
 * @brief Return invalid signal
 * 
 * @return Reference invalid signal
 */
const Signal &SGNL_getInvalid();
