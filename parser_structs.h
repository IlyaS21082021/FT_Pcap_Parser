#ifndef PARCER_STRUCTS_H
#define PARCER_STRUCTS_H

#include <netinet/in.h>

struct __attribute__((packed))
TFHeader
{
    uint32_t MNumber;
    uint16_t mjVersion;
    uint16_t mnVersion;
    uint32_t Res1;
    uint32_t Res2;
    uint32_t SnapLen;
    uint16_t linkType;
    uint16_t fcs;
};

struct TPacRec
{
    uint32_t tstamp1;
    uint32_t tstamp2;
    uint32_t capturedPacLen;
    uint32_t realPacLen;
};

struct TEthernetHeader
{
    unsigned char distMac[6];
    unsigned char srcMac[6];
    unsigned char etype[2];
};

struct TIp4Header
{
    uint8_t val1;
    uint8_t val2;
    uint16_t len;
    uint16_t val3;
    uint16_t val4;
    uint16_t val5;
    uint16_t val6;
    uint32_t srcAddr;
    uint32_t destAddr;
};

struct TUdpHeader
{
    uint16_t srcPort;
    uint16_t dstPort;
    uint16_t len;
    uint16_t cSum;
};

struct TMarketDataPacHeader
{
    uint32_t MsgSeqNum;
    uint16_t MsgSize;
    uint16_t MsgFlags;
    uint64_t SendingTime;
};

struct TSBEHeader
{
    uint16_t BlockLength;
    uint16_t TemplateID;
    uint16_t SchemaID;
    uint16_t Version;
};

struct __attribute__((packed))
TRepeatingSectionHeader
{
    uint16_t blockLength;
    uint8_t numInGroup;
};

struct TGroupSize2
{
    uint16_t blockLength;
    uint16_t numInGroup;
};

struct Decimal5NULL
{
    int64_t mantissa;
    //int8_t exponent = -5;
};

struct Decimal5
{
    int64_t mantissa;
    //int8_t exponent = -5;
};

struct __attribute__((packed))
TOBSEntry
{
    int64_t MDEntryID;
    uint64_t TransactTime;
    Decimal5NULL MDEntryPx;
    int64_t MDEntrySize;
    int64_t TradeID;
    uint64_t MDFlags;
    uint64_t MDFlags2;
    char MDEntryType;
};

struct TOrderBookSnapshot
{
    int32_t SecurityID;
    uint32_t LastMsgSeqNumProcessed;
    uint32_t RptSeq;
    uint32_t ExchangeTradingSessionID;
};

struct __attribute__((packed))
TOrderUpdate
{
    int64_t OrderUpdate;
    Decimal5 MDEntryPx;
    int64_t MDEntrySize;
    uint64_t MDFlags;
    uint64_t MDFlags2;
    int32_t SecurityID;
    uint32_t RptSeq;
    uint8_t MDUpdateAction;
    char MDEntryType;
};

struct __attribute__((packed))
TOrderExecution
{
    int64_t MDEntryID;
    Decimal5NULL MDEntryPx;
    int64_t MDEntrySize;
    Decimal5 LastPx;
    int64_t LastQty;
    int64_t TradeID;
    uint64_t MDFlags;
    uint64_t MDFlags2;
    int32_t SecurityID;
    uint32_t RptSeq;
    uint8_t MDUpdateAction;
    char MDEntryType;
};

struct __attribute__((packed))
TIncPacHeader
{
    uint64_t TransactTime;
    uint32_t ExchangeTradingSessionID;
};


#endif // PARCER_STRUCTS_H
