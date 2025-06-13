#ifndef PARSER_H
#define PARSER_H
#include <fstream>
#include "nlohmann/json.hpp"
#include "parser_structs.h"

class TParser
{
    size_t pacNum;
    std::ofstream fjson;

    bool ReadNetLevel(std::ifstream& f);
    void ReadEthLevel(std::ifstream& f);
    void ReadIpLevel(std::ifstream& f);
    void ReadUDPLevel(std::ifstream& f);
    bool ReadAppLevel(std::ifstream& f);
    bool ReadIncrementalPacket(std::ifstream& f, const TMarketDataPacHeader& headerPac,
                               const TIncPacHeader& incPacHeader, int byteCount);
    bool ReadSnapshotPacket(std::ifstream& f, const TMarketDataPacHeader& headerPac, int byteCount);
    void SaveOrderUpdate(const TMarketDataPacHeader& headerPac, const TIncPacHeader& incPacHeader,
                         const TSBEHeader& sbeHeader, const TOrderUpdate& ordUpdate);
    void SaveOrderExecution(const TMarketDataPacHeader& headerPac, const TIncPacHeader& incPacHeader,
                            const TSBEHeader& sbeHeader, const TOrderExecution& ordExec);
    void SaveOrderBookSnapshot(const TMarketDataPacHeader& headerPac, const TSBEHeader& sbeHeader,
                                        const TOrderBookSnapshot& orderBookSnapshot,
                                        const TRepeatingSectionHeader& repeatHeader,
                                        nlohmann::ordered_json& ordersJson);
    void SaveOBSEntry(const TOBSEntry& obsEntry, nlohmann::ordered_json& ordersJson);

public:
    TParser() = default;
    void ReadPacket(const std::string& inpFile, const std::string& outFile);
};

#endif // PARSER_H
