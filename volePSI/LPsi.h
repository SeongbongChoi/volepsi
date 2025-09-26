#pragma once

#include "volePSI/Defines.h"
#include "volePSI/config.h"

#include "cryptoTools/Crypto/PRNG.h"
#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Common/Timer.h"
#include "cryptoTools/Crypto/RandomOracle.h"

#include "relic.h"
#include <vector>

#define SK "258B8F5E39671B337C1E3B87559B579D3F878E5293DF2B01DE1B9E10CA9EC9D0"

namespace volePSI
{
    namespace details
    {
        struct LPsiBase
        {
            u64 mSenderSize = 0;
            u64 mReceiverSize = 0;
            u64 mSsp = 0;

            u64 mNumThreads = 0;
            oc::PRNG mPrng;

            bool mSetup = false;

            bn_t s;
            g1_t SS;
            g2_t* S = nullptr;

            LPsiBase();
            ~LPsiBase();

            void init(
                u64 senderSize,
                u64 receiverSize,
                u64 statSecParam,
                block seed,
                u64 numThreads);
        };
    }

    class LPSIReceiver : public details::LPsiBase, public oc::TimerAdapter
    {
    public:
        std::vector<std::array<uint8_t, RLC_FC_BYTES>> mIntersection;

        int cp_pbpsi_inth(
            g2_t d[], 
            bn_t x[],
            std::vector<uint8_t *> &t, 
            g1_t u[]);

        // 프로토콜 실행
        Proto run(bn_t* inputs, Socket &chl);
    };

    class LPSISender : public details::LPsiBase, public oc::TimerAdapter
    {
    public:
        int cp_pbpsi_ans(
            std::vector<gt_t> &t,
            std::vector<g1_t> &u,
            const g1_t &ss,
            const g2_t &d,
            const bn_t* y);

        Proto run(bn_t* inputs, Socket &chl);
    };

} // namespace volePSI
