#include "all.h"

static void setPaddings(uint8_t *payload, size_t dataLen, char *padPattern, size_t padPatternLen)
{

    if (padPattern)
    {
        size_t indexOnAByteInPadPattern = 0;
        for (uint8_t *pointerToAByteInPayload = payload; pointerToAByteInPayload < payload + dataLen;
             pointerToAByteInPayload++)
        {
            *pointerToAByteInPayload = padPattern[indexOnAByteInPadPattern];
            if (++indexOnAByteInPadPattern >= padPatternLen)
                indexOnAByteInPadPattern = 0;
        }
    }
    else
    {
        size_t indexOnAByteInPayload = 0;
        for (uint8_t *pointerToAByteInPayload = payload; pointerToAByteInPayload < payload + dataLen;
             pointerToAByteInPayload++)
            *pointerToAByteInPayload = indexOnAByteInPayload++;
    }
}

IcmpEchoRequest constructIcmpEchoRequest(uint16_t id, uint16_t sequenceNumber, char *padPattern, size_t padPatternLen,
                                         size_t dataLen)
{
    IcmpEchoRequest icmpEchoRequest;
    icmpEchoRequest.icmpHeader.type = 8;
    icmpEchoRequest.icmpHeader.code = 0;
    icmpEchoRequest.icmpHeader.checksum = 0;
    icmpEchoRequest.icmpHeader.un.echo.id = id;
    icmpEchoRequest.icmpHeader.un.echo.sequence = sequenceNumber;
    icmpEchoRequest.dataLen = dataLen;
    if (icmpEchoRequest.dataLen >= sizeof(struct timeval))
    {
        const struct timeval timeSent = timeOfDay();
        memcpy(icmpEchoRequest.data, &timeSent, sizeof(struct timeval));
        setPaddings(icmpEchoRequest.data + sizeof(struct timeval), icmpEchoRequest.dataLen - sizeof(struct timeval),
                    padPattern, padPatternLen);
    }
    else
        setPaddings(icmpEchoRequest.data, icmpEchoRequest.dataLen, padPattern, padPatternLen);
    return icmpEchoRequest;
}

static uint16_t calculateChecksum(void *data, size_t length)
{
    uint16_t *buf = data;
    uint32_t sum = 0;
    uint16_t result;

    for (sum = 0; length > 1; length -= 2)
        sum += *buf++;
    if (length == 1)
        sum += *(uint8_t *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}


void encodeIcmpEchoRequest(IcmpEchoRequest icmpEchoRequest, char *buffer)
{
    const uint16_t temporaryChecksum = 0;
    buffer[0] = icmpEchoRequest.icmpHeader.type;
    buffer[1] = icmpEchoRequest.icmpHeader.code;

    buffer[2] = (temporaryChecksum >> 8) & 0xFF;
    buffer[3] = temporaryChecksum & 0xFF;

    buffer[4] = (icmpEchoRequest.icmpHeader.un.echo.id >> 8) & 0xFF;
    buffer[5] = icmpEchoRequest.icmpHeader.un.echo.id & 0xFF;

    buffer[6] = (icmpEchoRequest.icmpHeader.un.echo.sequence >> 8) & 0xFF;
    buffer[7] = icmpEchoRequest.icmpHeader.un.echo.sequence & 0xFF;

    /* Copy data */
    memcpy(buffer + 8, icmpEchoRequest.data, icmpEchoRequest.dataLen);
    const uint16_t checksum = calculateChecksum(buffer, icmpEchoRequest.dataLen + 8);
    buffer[3] = (checksum >> 8) & 0xFF;
    buffer[2] = checksum & 0xFF;
}