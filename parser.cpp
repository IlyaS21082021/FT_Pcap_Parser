#include <iostream>
#include <arpa/inet.h>
#include "parser.h"

constexpr int repeatHeaderSize = 3;
constexpr int incrementalPacHeaderSize = 12;

inline bool isIncrementalPacket(uint16_t msgFl)
{
    return ((msgFl & 0x8) == 0x8);
}

void TParser::ReadPacket(const std::string& inpFile, const std::string& outFile)
{
    std::ifstream parseFile(inpFile);
    if (!parseFile.is_open())
    {
        throw std::runtime_error("Cannot open file " + inpFile);
    }
    fjson.open(outFile);

    parseFile.seekg(sizeof(TFHeader), std::ios::cur);
    pacNum = 0;

    bool res = true;
    try
    {
        while (res)
        {
            pacNum++;
            res = ReadNetLevel(parseFile);
            if (pacNum%100000 == 0)
                std::cout << "Packets read: " << pacNum << "\n";
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << " -- Packet number " << pacNum << "\n";
    }

    parseFile.close();
    fjson.close();
}

bool TParser::ReadNetLevel(std::ifstream& f)
{
    TPacRec pacRec;

    f.read(reinterpret_cast<char*>(&pacRec), sizeof(TPacRec));
    if (f.gcount() == 0)
        return false;

    ReadEthLevel(f);
    return true;
}

void TParser::ReadEthLevel(std::ifstream& f)
{
    TEthernetHeader ethHeader;
    f.read(reinterpret_cast<char*>(&ethHeader), sizeof(TEthernetHeader));
    ReadIpLevel(f);
}

void TParser::ReadIpLevel(std::ifstream& f)
{
    TIp4Header ip4Header;
    f.read(reinterpret_cast<char*>(&ip4Header), sizeof(TIp4Header));
    ReadUDPLevel(f);
}

void TParser::ReadUDPLevel(std::ifstream& f)
{
    TUdpHeader udpHeader;
    f.read(reinterpret_cast<char*>(&udpHeader), sizeof(TUdpHeader));
    udpHeader.len = htons(udpHeader.len) - 8;

    std::streampos pacStartPos = f.tellg();
    if (!ReadAppLevel(f))
    {
        f.seekg(pacStartPos);
        f.seekg(udpHeader.len, std::ios::cur);
        std::cerr << "Error in format packet -- " << pacNum << ".  Packet has been skipped\n";
    }
}

bool TParser::ReadAppLevel(std::ifstream& f)
{
    TMarketDataPacHeader mdPacHeader;
    f.read(reinterpret_cast<char*>(&mdPacHeader), sizeof(TMarketDataPacHeader));

    if (isIncrementalPacket(mdPacHeader.MsgFlags))
    {
        TIncPacHeader incPacHeader;
        f.read(reinterpret_cast<char*>(&incPacHeader), incrementalPacHeaderSize);
        return ReadIncrementalPacket(f, mdPacHeader, incPacHeader, mdPacHeader.MsgSize -
                                                                sizeof(TMarketDataPacHeader) -
                                                                incrementalPacHeaderSize);
    }
    else
        return ReadSnapshotPacket(f, mdPacHeader, mdPacHeader.MsgSize - sizeof(TMarketDataPacHeader));
}

bool TParser::ReadIncrementalPacket(std::ifstream& f, const TMarketDataPacHeader& headerPac,
                                                      const TIncPacHeader& incPacHeader, int byteCount)
{
    TSBEHeader sbeHeader;
    while (byteCount > 0)
    {
        if (byteCount < sizeof(TSBEHeader))
            return false;

        f.read(reinterpret_cast<char*>(&sbeHeader), sizeof(TSBEHeader));
        if (sbeHeader.TemplateID == 15) //OrderUpdate
        {
            TOrderUpdate ordUpdate;
            f.read(reinterpret_cast<char*>(&ordUpdate), sizeof(TOrderUpdate));
            SaveOrderUpdate(headerPac, incPacHeader, sbeHeader, ordUpdate);
        }
        else if (sbeHeader.TemplateID == 16) //OrderExecution
        {
            TOrderExecution ordExec;
            f.read(reinterpret_cast<char*>(&ordExec), sizeof(TOrderExecution));
            SaveOrderExecution(headerPac, incPacHeader, sbeHeader, ordExec);
        }
        else if (sbeHeader.TemplateID == 14)
        {
            TRepeatingSectionHeader repeatHeader;
            f.read(reinterpret_cast<char*>(&repeatHeader), repeatHeaderSize);
            f.seekg(repeatHeader.blockLength * repeatHeader.numInGroup, std::ios::cur);
            byteCount = byteCount - repeatHeader.blockLength * repeatHeader.numInGroup - repeatHeaderSize;
        }
        else if (sbeHeader.TemplateID == 19)
        {
            TGroupSize2 groupSize2;
            f.read(reinterpret_cast<char*>(&groupSize2), sizeof(TGroupSize2));
            f.seekg(groupSize2.blockLength * groupSize2.numInGroup, std::ios::cur);
            byteCount = byteCount - groupSize2.blockLength * groupSize2.numInGroup - sizeof(TGroupSize2);
        }
        else
            f.seekg(sbeHeader.BlockLength, std::ios::cur);

        byteCount = byteCount - sizeof(TSBEHeader) - sbeHeader.BlockLength;
        if (byteCount < 0)
            return false;;
    }
    return true;
}


bool TParser::ReadSnapshotPacket(std::ifstream& f, const TMarketDataPacHeader& headerPac, int byteCount)
{
    TSBEHeader sbeHeader;
    f.read(reinterpret_cast<char*>(&sbeHeader), sizeof(TSBEHeader));
    if (sbeHeader.TemplateID == 17)
    {
        TOrderBookSnapshot orderBookSnapshot;
        f.read(reinterpret_cast<char*>(&orderBookSnapshot), sizeof(TOrderBookSnapshot));

        TRepeatingSectionHeader repeatHeader;
        f.read(reinterpret_cast<char*>(&repeatHeader), repeatHeaderSize);

        nlohmann::ordered_json ordersJson;
        SaveOrderBookSnapshot(headerPac, sbeHeader, orderBookSnapshot, repeatHeader, ordersJson);
        TOBSEntry obsEntry;
        for (int i = 0; i < repeatHeader.numInGroup; i++)
        {
            f.read(reinterpret_cast<char*>(&obsEntry), repeatHeader.blockLength);
            SaveOBSEntry(obsEntry, ordersJson);
        }
        fjson << ordersJson.dump() << "\n";

        byteCount = byteCount - sizeof(TSBEHeader) - sbeHeader.BlockLength;
        if (byteCount < 0)
            return false;
    }
    else
        f.seekg(byteCount - sizeof(TSBEHeader), std::ios::cur);

    return true;
}


void TParser::SaveOrderUpdate(const TMarketDataPacHeader& headerPac, const TIncPacHeader& incPacHeader,
                              const TSBEHeader& sbeHeader, const TOrderUpdate& ordUpdate)
{
    nlohmann::ordered_json ordersJson;

    ordersJson["MarketDataPacketHeader"].push_back({
        {"MsgSeqNum", headerPac.MsgSeqNum},
        {"MsgSize", headerPac.MsgSize},
        {"MsgFlags", headerPac.MsgFlags},
        {"SendingTime", headerPac.SendingTime}
    });
    ordersJson["IncrementalPacketHeader"].push_back({
        {"TransactTime", static_cast<uint64_t>(incPacHeader.TransactTime)},
        {"ExchangeTradingSessionID", static_cast<uint32_t>(incPacHeader.ExchangeTradingSessionID)}
    });
    ordersJson["SBEHeader"].push_back({
        {"BlockLength", sbeHeader.BlockLength},
        {"TemplateID", sbeHeader.TemplateID},
        {"SchemaID", sbeHeader.SchemaID},
        {"Version", sbeHeader.Version}
    });
    ordersJson["RootBlock"].push_back({
        {"OrderUpdate", static_cast<int64_t>(ordUpdate.OrderUpdate)},
        {"MDEntryPx", {{"mantissa", static_cast<int64_t>(ordUpdate.MDEntryPx.mantissa)},{"exponent", -5}}},
        {"MDEntrySize", static_cast<int64_t>(ordUpdate.MDEntrySize)},
        {"MDFlags", static_cast<uint64_t>(ordUpdate.MDFlags)},
        {"MDFlags2", static_cast<uint64_t>(ordUpdate.MDFlags2)},
        {"SecurityID", static_cast<int32_t>(ordUpdate.SecurityID)},
        {"RptSeq", static_cast<uint32_t>(ordUpdate.RptSeq)},
        {"MDUpdateAction", ordUpdate.MDUpdateAction},
        {"MDEntryType", ordUpdate.MDEntryType}
    });
    fjson << ordersJson.dump() << "\n";
}

void TParser::SaveOrderExecution(const TMarketDataPacHeader& headerPac, const TIncPacHeader& incPacHeader,
                                 const TSBEHeader& sbeHeader, const TOrderExecution& ordExec)
{
    nlohmann::ordered_json ordersJson;

    ordersJson["MarketDataPacketHeader"].push_back({
        {"MsgSeqNum", headerPac.MsgSeqNum},
        {"MsgSize", headerPac.MsgSize},
        {"MsgFlags", headerPac.MsgFlags},
        {"SendingTime", headerPac.SendingTime}
    });
    ordersJson["IncrementalPacketHeader"].push_back({
        {"TransactTime", static_cast<uint64_t>(incPacHeader.TransactTime)},
        {"ExchangeTradingSessionID", static_cast<uint32_t>(incPacHeader.ExchangeTradingSessionID)}
    });
    ordersJson["SBEHeader"].push_back({
        {"BlockLength", sbeHeader.BlockLength},
        {"TemplateID", sbeHeader.TemplateID},
        {"SchemaID", sbeHeader.SchemaID},
        {"Version", sbeHeader.Version}
    });
    ordersJson["RootBlock"].push_back({
        {"MDEntryID", static_cast<int64_t>(ordExec.MDEntryID)},
        {"MDEntryPx", {{"mantissa", static_cast<int64_t>(ordExec.MDEntryPx.mantissa)},{"exponent", -5}}},
        {"MDEntrySize", static_cast<int64_t>(ordExec.MDEntrySize)},
        {"LastPx", {{"mantissa", static_cast<int64_t>(ordExec.MDEntryPx.mantissa)},{"exponent", -5}}},
        {"LastQty", static_cast<int64_t>(ordExec.LastQty)},
        {"TradeID", static_cast<int64_t>(ordExec.TradeID)},
        {"MDFlags", static_cast<uint64_t>(ordExec.MDFlags)},
        {"MDFlags2", static_cast<uint64_t>(ordExec.MDFlags2)},
        {"SecurityID", static_cast<int32_t>(ordExec.SecurityID)},
        {"RptSeq", static_cast<uint32_t>(ordExec.RptSeq)},
        {"MDUpdateAction", ordExec.MDUpdateAction},
        {"MDEntryType", ordExec.MDEntryType}
    });
    fjson << ordersJson.dump() << "\n";
}

void TParser::SaveOrderBookSnapshot(const TMarketDataPacHeader& headerPac, const TSBEHeader& sbeHeader,
                                    const TOrderBookSnapshot& orderBookSnapshot,
                                    const TRepeatingSectionHeader& repeatHeader, nlohmann::ordered_json& ordersJson)
{
    ordersJson["MarketDataPacketHeader"].push_back({
        {"MsgSeqNum", headerPac.MsgSeqNum},
        {"MsgSize", headerPac.MsgSize},
        {"MsgFlags", headerPac.MsgFlags},
        {"SendingTime", headerPac.SendingTime}
    });
    ordersJson["SBEHeader"].push_back({
        {"BlockLength", sbeHeader.BlockLength},
        {"TemplateID", sbeHeader.TemplateID},
        {"SchemaID", sbeHeader.SchemaID},
        {"Version", sbeHeader.Version}
    });
    ordersJson["RootBlock"].push_back({
        {"SecurityID", orderBookSnapshot.SecurityID},
        {"LastMsgSeqNumProcessed", orderBookSnapshot.LastMsgSeqNumProcessed},
        {"RptSeq", orderBookSnapshot.RptSeq},
        {"ExchangeTradingSessionID", orderBookSnapshot.ExchangeTradingSessionID}
    });
    ordersJson["NoMDEntries"].emplace_back();
}

void TParser::SaveOBSEntry(const TOBSEntry& obsEntry, nlohmann::ordered_json& ordersJson)
{
    auto& entry = ordersJson["NoMDEntries"].back();
    entry["MDEntryID"] = static_cast<int64_t>(obsEntry.MDEntryID);
    entry["TransactTime"] = static_cast<uint64_t>(obsEntry.TransactTime);
    entry["MDEntryPx"] = {{"mantissa", static_cast<int64_t>(obsEntry.MDEntryPx.mantissa)},{"exponent", -5}};
    entry["MDEntrySize"] = static_cast<int64_t>(obsEntry.MDEntrySize);
    entry["TradeID"] = static_cast<int64_t>(obsEntry.TradeID);
    entry["MDFlags"] = static_cast<uint64_t>(obsEntry.MDFlags);
    entry["MDFlags2"] = static_cast<uint64_t>(obsEntry.MDFlags2);
    entry["MDEntryType"] = obsEntry.MDEntryType;
}
