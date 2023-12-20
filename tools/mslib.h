#ifndef __mslib_h__
#define __mslib_h__

/*
 * Communicate with a moonshot device over the extended cyberstick protocol.
 * As host to device communication is performed over 1-bit channel though the device to host
 * is done over 4-bits, communication bandwidth is asymmetric. So, the behind the original
 * cyberstick protocol's 12 low and high cycles, the host send 4-bits marker and 8-bits
 * command. If the device recognizes the marker as a start signal for the extended protocol,
 * the device continues to send 4 bytes in 8 extended cycles. The host can send the next
 * command behind this extended cycles. Thus, we can expect 4 bytes response per each command.
 * The sequence will be terminated by the terminal command 0xff.
 *
 * arguments:
 *   len command length (0 can be set for the original cyberstick mode)
 *   cmd pointer for command buffer (length: `len`)
 *   res pointer for response buffer (length: 6 + `len` * 4)
 */
int ms_comm(int len, unsigned char* cmd, unsigned char* res);

/*
 * Set timeout setting for ms_comm. Default value is 800, and maximum value 0xfffe would be
 * suitable for U-kun.
 */
void ms_set_timeout(unsigned short timeout);

#endif  /* __mslib_h__ */
