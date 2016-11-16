#include <iostream>

using namespace std;
#include "UnitTests.h" 
#include "Common/Defines.h"
using namespace osuCrypto;

#include "OT/TwoChooseOne/KosOtExtReceiver.h"
#include "OT/TwoChooseOne/KosOtExtSender.h"
#include "Network/BtChannel.h"
#include "Network/BtEndpoint.h"
#include <numeric>
#include "Common/Log.h"
int miraclTestMain();

#include "OT/Tools/BchCode.h"
#include "OT/NChooseOne/Oos/OosNcoOtReceiver.h"
#include "OT/NChooseOne/Oos/OosNcoOtSender.h"
#include "OT/NChooseOne/KkrtNcoOtReceiver.h"
#include "OT/NChooseOne/KkrtNcoOtSender.h"

#include "OT/TwoChooseOne/IknpOtExtReceiver.h"
#include "OT/TwoChooseOne/IknpOtExtSender.h"

#include "OT/NChooseK/AknOtReceiver.h"
#include "OT/NChooseK/AknOtSender.h"
#include "OT/TwoChooseOne/LzKosOtExtReceiver.h"
#include "OT/TwoChooseOne/LzKosOtExtSender.h"

#include "CLP.h"



void kkrt_test(int i)
{
    Log::setThreadName("Sender");

    PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

    u64 step = 1024;
    u64 numOTs = 1 << 24;
    u64 numThreads = 1;

    u64 otsPer = numOTs / numThreads;

    std::string name = "n";
    BtIOService ios(0);
    BtEndpoint ep0(ios, "localhost", 1212, i, name);
    std::vector<Channel*> chls(numThreads);

    for (u64 k = 0; k < numThreads; ++k)
        chls[k] = &ep0.addChannel(name + ToString(k), name + ToString(k));



    u64 ncoinputBlkSize = 1, baseCount = 4 * 128;
    u64 codeSize = (baseCount + 127) / 128;

    std::vector<block> baseRecv(baseCount);
    std::vector<std::array<block, 2>> baseSend(baseCount);
    BitVector baseChoice(baseCount);
    baseChoice.randomize(prng0);

    prng0.get((u8*)baseSend.data()->data(), sizeof(block) * 2 * baseSend.size());
    for (u64 i = 0; i < baseCount; ++i)
    {
        baseRecv[i] = baseSend[i][baseChoice[i]];
    }

    std::vector<block> choice(ncoinputBlkSize), correction(codeSize);
    prng0.get((u8*)choice.data(), ncoinputBlkSize * sizeof(block));

    std::vector< thread> thds(numThreads);

    SHA1 sha;
    u8 hashout[20];
    if (i == 0)
    {

        for (u64 k = 0; k < numThreads; ++k)
        {
            thds[k] = std::thread(
                [&, k]()
            {
                KkrtNcoOtReceiver r;
                r.setBaseOts(baseSend);
                auto& chl = *chls[k];

                r.init(otsPer);
                block encoding1, encoding2;
                for (u64 i = 0; i < otsPer; i += step)
                {
                    for (u64 j = 0; j < step; ++j)
                    {
                        r.encode(i + j, choice, encoding1);
                    }

                    r.sendCorrection(chl, step);
                }
                r.check(chl);

                chl.close();
            });
        }
        for (u64 k = 0; k < numThreads; ++k)
            thds[k].join();
    }
    else
    {
        Timer time;
        time.setTimePoint("start");
        block encoding1, encoding2;

        for (u64 k = 0; k < numThreads; ++k)
        {
            thds[k] = std::thread(
                [&, k]()
            {
                KkrtNcoOtSender s;
                s.setBaseOts(baseRecv, baseChoice);
                auto& chl = *chls[k];

                s.init(otsPer);
                for (u64 i = 0; i < otsPer; i += step)
                {

                    s.recvCorrection(chl, step);

                    for (u64 j = 0; j < step; ++j)
                    {
                        s.encode(i + j, choice, encoding2);
                    }
                }
                s.check(chl);
                chl.close();
            });
        }


        for (u64 k = 0; k < numThreads; ++k)
            thds[k].join();

        time.setTimePoint("finish");
        Log::out << time << Log::endl;
    }


    //for (u64 k = 0; k < numThreads; ++k)
        //chls[k]->close();

    ep0.stop();
    ios.stop();
}


void oos_test(int i)
{
    Log::setThreadName("Sender");

    PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

    u64 step = 1024;
    u64 numOTs = 1 << 24;
    u64 numThreads = 1;

    u64 otsPer = numOTs / numThreads;

    std::string name = "n";
    BtIOService ios(0);
    BtEndpoint ep0(ios, "localhost", 1212, i, name);
    std::vector<Channel*> chls(numThreads);

    for (u64 k = 0; k < numThreads; ++k)
        chls[k] = &ep0.addChannel(name + ToString(k), name + ToString(k));


    BchCode code;
    code.loadBinFile(std::string(SOLUTION_DIR) + "/libOTe/OT/Tools/bch511.bin");




    u64 ncoinputBlkSize = 1, baseCount = 4 * 128; 
    u64 codeSize = (baseCount + 127) / 128;

    std::vector<block> baseRecv(baseCount);
    std::vector<std::array<block, 2>> baseSend(baseCount);
    BitVector baseChoice(baseCount);
    baseChoice.randomize(prng0);

    prng0.get((u8*)baseSend.data()->data(), sizeof(block) * 2 * baseSend.size());
    for (u64 i = 0; i < baseCount; ++i)
    {
        baseRecv[i] = baseSend[i][baseChoice[i]];
    }

    std::vector<block> choice(ncoinputBlkSize), correction(codeSize);
    prng0.get((u8*)choice.data(), ncoinputBlkSize * sizeof(block));

    std::vector< thread> thds(numThreads);


    if (i == 0)
    {

        for (u64 k = 0; k < numThreads; ++k)
        {
            thds[k] = std::thread(
                [&, k]()
            {
                OosNcoOtReceiver r(code);
                r.setBaseOts(baseSend);
                auto& chl = *chls[k];

                r.init(otsPer);
                block encoding1, encoding2;
                for (u64 i = 0; i < otsPer; i += step)
                {
                    for (u64 j = 0; j < step; ++j)
                    {
                        r.encode(i + j, choice, encoding1);
                    }

                    r.sendCorrection(chl, step);
                }
                r.check(chl);
            });
        }
        for (u64 k = 0; k < numThreads; ++k)
            thds[k].join();
    }
    else
    {
        Timer time;
        time.setTimePoint("start");
        block encoding1, encoding2;

        for (u64 k = 0; k < numThreads; ++k)
        {
            thds[k] = std::thread(
                [&, k]()
            {
                OosNcoOtSender s(code);// = sender[k];
                s.setBaseOts(baseRecv, baseChoice);
                auto& chl = *chls[k];

                s.init(otsPer);
                for (u64 i = 0; i < otsPer; i += step)
                {

                    s.recvCorrection(chl, step);

                    for (u64 j = 0; j < step; ++j)
                    {
                        s.encode(i + j, choice, encoding2);
                    }
                }
                s.check(chl);
            });
        }


        for (u64 k = 0; k < numThreads; ++k)
            thds[k].join();

        time.setTimePoint("finish");
        Log::out << time << Log::endl;
    }


    for (u64 k = 0; k < numThreads; ++k)
        chls[k]->close();

    ep0.stop();
    ios.stop();
}


void kos_test(int i)
{
    Log::setThreadName("Sender");

    PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

    u64 step = 1024;
    u64 numOTs = 1 << 24;


    // get up the networking
    std::string name = "n";
    BtIOService ios(0);
    BtEndpoint ep0(ios, "localhost", 1212, i, name);
    Channel& chl = ep0.addChannel(name, name);


    // cheat and compute the base OT in the clear.
    u64 baseCount = 128;
    std::vector<block> baseRecv(baseCount);
    std::vector<std::array<block, 2>> baseSend(baseCount);
    BitVector baseChoice(baseCount);
    baseChoice.randomize(prng0);

    prng0.get((u8*)baseSend.data()->data(), sizeof(block) * 2 * baseSend.size());
    for (u64 i = 0; i < baseCount; ++i)
    {
        baseRecv[i] = baseSend[i][baseChoice[i]];
    }




    if (i)
    {
        BitVector choice(numOTs);
        std::vector<block> msgs(numOTs);
        choice.randomize(prng0);
        KosOtExtReceiver r;
        r.setBaseOts(baseSend);

        r.receive(choice, msgs, prng0, chl);
    }
    else
    {
        std::vector<std::array<block, 2>> msgs(numOTs);

        gTimer.setTimePoint("start");
        block encoding1, encoding2;
        KosOtExtSender s;
        s.setBaseOts(baseRecv, baseChoice);

        s.send(msgs, prng0, chl);

        gTimer.setTimePoint("finish");
        Log::out << gTimer << Log::endl;

    }


    chl.close();

    ep0.stop();
    ios.stop();
}



void iknp_test(int i)
{
    Log::setThreadName("Sender");

    PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

    u64 step = 1024;
    u64 numOTs = 1 << 24;


    // get up the networking
    std::string name = "n";
    BtIOService ios(0);
    BtEndpoint ep0(ios, "localhost", 1212, i, name);
    Channel& chl = ep0.addChannel(name, name);


    // cheat and compute the base OT in the clear.
    u64 baseCount = 128;
    std::vector<block> baseRecv(baseCount);
    std::vector<std::array<block, 2>> baseSend(baseCount);
    BitVector baseChoice(baseCount);
    baseChoice.randomize(prng0);

    prng0.get((u8*)baseSend.data()->data(), sizeof(block) * 2 * baseSend.size());
    for (u64 i = 0; i < baseCount; ++i)
    {
        baseRecv[i] = baseSend[i][baseChoice[i]];
    }




    if (i)
    {
        BitVector choice(numOTs);
        std::vector<block> msgs(numOTs);
        choice.randomize(prng0);
        IknpOtExtReceiver r;
        r.setBaseOts(baseSend);

        r.receive(choice, msgs, prng0, chl);
    }
    else
    {
        std::vector<std::array<block, 2>> msgs(numOTs);

        Timer time;
        time.setTimePoint("start");
        block encoding1, encoding2;
        IknpOtExtSender s;
        s.setBaseOts(baseRecv, baseChoice);

        s.send(msgs, prng0, chl);

        time.setTimePoint("finish");
        Log::out << time << Log::endl;

    }


    chl.close();

    ep0.stop();
    ios.stop();
}


void akn_test(int i)
{

    u64 totalOts(149501);
    u64 minOnes(4028);
    u64 avgOnes(5028);
    u64 maxOnes(9363);
    u64 cncThreshold(724);
    double cncProb(0.0999);


    Log::setThreadName("Recvr");

    BtIOService ios(0);
    BtEndpoint  ep0(ios, "127.0.0.1", 1212, i, "ep"); 

    u64 numTHreads(4);

    std::vector<Channel*> chls(numTHreads);
    for (u64 i = 0; i < numTHreads; ++i)
        chls[i] = &ep0.addChannel("chl" + std::to_string(i), "chl" + std::to_string(i));

    PRNG prng(ZeroBlock);

    if(i)
    {
        AknOtSender send;
        LzKosOtExtSender otExtSend;
        send.init(totalOts, cncThreshold, cncProb, otExtSend, chls, prng);
    }
    else
    {

        AknOtReceiver recv;
        LzKosOtExtReceiver otExtRecv;
        recv.init(totalOts, avgOnes, cncProb, otExtRecv, chls, prng);

        if (recv.mOnes.size() < minOnes)
            throw std::runtime_error("");

        if (recv.mOnes.size() > maxOnes)
            throw std::runtime_error("");

    }


    for (u64 i = 0; i < numTHreads; ++i)
        chls[i]->close();

    ep0.stop();
    ios.stop();
}

static const std::vector<std::string>
unitTestTag{ "u", "unitTest" },
kos{ "k", "kos" },
kkrt{ "kk", "kkrt" },
iknp{ "i", "iknp" },
oos{ "o", "oos" },
akn{ "a", "akn" };

int main(int argc, char** argv)
{
    CLP cmd;
    cmd.parse(argc, argv);


    if (cmd.isSet(unitTestTag))
    {
        run_all();
    }
    else if (cmd.isSet(kos) || true)
    {
        if (cmd.hasValue(kos))
        {
            kos_test(cmd.getInt(kos));
        }
        else
        {
            auto thrd = std::thread([]() { kos_test(0); });
            kos_test(1);
            thrd.join();
        }
    }
    else if (cmd.isSet(kkrt))
    {
        if (cmd.hasValue(kkrt))
        {
            kkrt_test(cmd.getInt(kkrt));
        }
        else
        {
            auto thrd = std::thread([]() { kkrt_test(0); });
            kkrt_test(1);
            thrd.join();
        }
    }
    else if (cmd.isSet(iknp))
    {
        if (cmd.hasValue(iknp))
        {
            iknp_test(cmd.getInt(iknp));
        }
        else
        {
            auto thrd = std::thread([]() { iknp_test(0); });
            iknp_test(1);
            thrd.join();
        }
    }
    else if (cmd.isSet(oos))
    {
        if (cmd.hasValue(oos))
        {
            oos_test(cmd.getInt(oos));
        }
        else
        {
            auto thrd = std::thread([]() { oos_test(0); });
            oos_test(1);
            thrd.join();
        }
    }
    else if (cmd.isSet(akn))
    {
        if (cmd.hasValue(akn))
        {
            akn_test(cmd.getInt(akn));
        }
        else
        {
            auto thrd = std::thread([]() { akn_test(0); });
            akn_test(1);
            thrd.join();
        }
    }
    else
    {
        Log::out << "this program takes a runtime argument.\n\nTo run the unit tests, run\n\n"
            << "    frontend.exe -unitTest\n\n"
            << "to run the OOS16 active secure 1-out-of-N OT for N=2^76, run\n\n"
            << "    frontend.exe -oos\n\n"
            << "to run the KOS active secure 1-out-of-2 OT, run\n\n"
            << "    frontend.exe -kos\n\n"
            << "to run the IKNP passive secure 1-out-of-2 OT, run\n\n"
            << "    frontend.exe -iknp\n\n"
            << "to run the RR16 active secure approximate k-out-of-N OT, run\n\n"
            << "    frontend.exe -akn\n\n"
            << "all of these options can take a value in {0,1} in which case the program will\n"
            << "run between two terminals, where each one was set to the opposite value. e.g.\n\n"
            << "    frontend.exe -iknp 0\n\n"
            << "    frontend.exe -iknp 1\n\n"
            << "These programs are fully networked and try to connect at localhost:1212.\n"
            << Log::endl;
    }

    return 0;
}