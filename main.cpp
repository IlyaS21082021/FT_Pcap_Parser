#include <iostream>
#include "parser.h"

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "It should be 2 input parameters\n";
        return 0;
    }

    try
    {
        TParser parser;
        //parser.ReadPacket("../../2023-10-10.0845-0905.pcap", "../../output.json");
        parser.ReadPacket(argv[1], argv[2]);
        std::cout << ">> finished\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
    }

    return 0;
}
