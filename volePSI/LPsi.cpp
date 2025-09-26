#include "LPsi.h"

namespace volePSI
{
    //
    // ===== LPsiBase =====
    //

    constexpr size_t DIGEST_SIZE = 32;

    details::LPsiBase::LPsiBase()
    {
        // if (core_init() != RLC_OK || pc_param_set_any() != RLC_OK)
        // {
        //     throw std::runtime_error("Relic initialization failed in LPsiBase.");
        // }
    }

    details::LPsiBase::~LPsiBase()
    {
        if (mSetup)
        {
            bn_free(s);
            g1_free(SS);

            if (S != nullptr)
            {
                for (u64 i = 0; i <= mReceiverSize; i++)
                {
                    g2_free(S[i]);
                }
                delete[] S;
            }
            core_clean();
        }
    }

    void details::LPsiBase::init(
        u64 senderSize,
        u64 receiverSize,
        u64 statSecParam,
        block seed,
        u64 numThreads)
    {
        mSenderSize = senderSize;
        mReceiverSize = receiverSize;
        mSsp = statSecParam;
        mPrng.SetSeed(seed);
        mNumThreads = numThreads;
        mSetup = true;

        bn_t ord;
        bn_null(ord);
        bn_new(ord);
        pc_get_ord(ord);

        bn_null(s);
        bn_new(s);

        g1_null(SS);
        g1_new(SS);

        bn_read_str(s, SK, strlen(SK), 16);

        g1_mul_gen(SS, s);

        S = new g2_t[receiverSize + 1];
        ;

        g2_get_gen(S[0]);

        for (u64 i = 1; i <= receiverSize; ++i)
        {
            g2_mul(S[i], S[i - 1], s);
        }

        bn_free(ord);
    }

    //
    // ===== LPSISender =====
    //

    int LPSISender::cp_pbpsi_ans(
        std::vector<gt_t> &t,
        std::vector<g1_t> &u,
        const g1_t &ss,
        const g2_t &d,
        const bn_t *y)
    {
        int result = RLC_OK;

        bn_t q, tj;
        g1_t g1;
        g2_t g2;

        bn_null(q);
        bn_null(tj);
        g1_null(g1);
        g2_null(g2);

        try
        {
            bn_new(q);
            bn_new(tj);
            g1_new(g1);
            g2_new(g2);

            // 무작위 permutation 생성
            std::vector<uint_t> shuffle(mSenderSize);
            util_perm(shuffle.data(), mSenderSize);

            pc_get_ord(q);
            g2_get_gen(g2);

            for (size_t j = 0; j < mSenderSize; j++)
            {
                // 무작위 지수 tj
                bn_rand_mod(tj, q);

                // g1 = tj * G1
                g1_mul_gen(g1, tj);

                // t[j] = e(g1, d)
                pc_map(t[j], g1, d);

                // u[j] = (ss - y[shuffle[j]]*G1) * tj
                g1_mul_gen(u[j], y[shuffle[j]]);
                g1_sub(u[j], ss, u[j]);
                g1_mul(u[j], u[j], tj);
            }
        }
        catch (...)
        {
            result = RLC_ERR;
        }

        bn_free(q);
        bn_free(tj);
        g1_free(g1);
        g2_free(g2);

        return result;
    }

    Proto LPSISender::run(bn_t *inputs, Socket &chl)
    {
        setTimePoint("LpsiSender::run-begin");

        // --- Step 1: d 수신 ---
        g2_t d;
        g2_null(d);
        g2_new(d);

        uint8_t buffer[4 * RLC_PC_BYTES + 1];
        memset(buffer, 0x00, 4 * RLC_PC_BYTES + 1);

        co_await chl.recv(buffer);
        g2_read_bin(d, buffer, 4 * RLC_PC_BYTES + 1);

        setTimePoint("LpsiSender::receive R");

        // --- Step 2: cp_pbpsi_ans 호출 ---
        std::vector<g1_t> u(mSenderSize);
        std::vector<gt_t> t(mSenderSize);

        for (u64 i = 0; i < mSenderSize; i++)
        {
            g1_null(u[i]);
            g1_new(u[i]);
            gt_null(t[i]);
            gt_new(t[i]);
        }

        int ans_res = cp_pbpsi_ans(t, u, SS, d, inputs);

        if (ans_res != RLC_OK)
            throw std::runtime_error("cp_pbpsi_ans failed");

        // --- Step 3: 직렬화 ---
        std::vector<uint8_t> tmp(12 * RLC_PC_BYTES);
        const size_t digestSize = 32; // SHA3-256 크기
        const size_t itemSize = digestSize + (RLC_PC_BYTES + 1);

        for (u64 i = 0; i < mSenderSize; ++i)
        {
            // 개별 메시지 버퍼 생성
            std::vector<uint8_t> itemBuffer(itemSize);

            // GT 직렬화
            gt_write_bin(tmp.data(), tmp.size(), t[i], 0);

            // Digest 계산
            std::array<uint8_t, 32> digest;
            oc::RandomOracle ro(digestSize);
            ro.Update(tmp.data(), tmp.size());
            ro.Final(digest.data());

            // digest 복사
            memcpy(itemBuffer.data(), digest.data(), digestSize);

            // G1 직렬화
            g1_write_bin(itemBuffer.data() + digestSize, RLC_PC_BYTES + 1, u[i], 1);

            // 개별 전송
            co_await chl.send(itemBuffer);
        }

        setTimePoint("LpsiSender::Second Round");

        // --- Step 5: 메모리 해제 ---
        for (u64 i = 0; i < mSenderSize; i++)
        {
            g1_free(u[i]);
            gt_free(t[i]);
        }
        g2_free(d);

        co_await chl.flush();
    }

    //
    // ===== LpsiReceiver =====
    //

    int LPSIReceiver::cp_pbpsi_inth(
        g2_t d[],
        bn_t x[],
        std::vector<uint8_t *> &t,
        g1_t u[])

    {
        int result = RLC_OK;
        gt_t e;
        uint8_t buffer[12 * RLC_PC_BYTES];

        gt_null(e);

        try
        {
            gt_new(e);

            mIntersection.clear(); // 교집합 결과 초기화

            if (mReceiverSize > 0)
            {
                for (int k = 0; k < mReceiverSize; k++)
                {
                    for (int j = 0; j < mSenderSize; j++)
                    {
                        // pairing
                        pc_map(e, u[j], d[k + 1]);

                        // serialize gt element
                        gt_write_bin(buffer, sizeof(buffer), e, 0);

                        // hash with volePSI/cryptoTools (SHA3-256)
                        std::array<uint8_t, DIGEST_SIZE> digest;
                        oc::RandomOracle ro(DIGEST_SIZE);
                        ro.Update(buffer, sizeof(buffer));
                        ro.Final(digest.data());

                        // compare with stored hash
                        if (memcmp(digest.data(), t[j], DIGEST_SIZE) == 0 &&
                            !gt_is_unity(e))
                        {
                            std::array<uint8_t, RLC_FC_BYTES> bin;
                            bn_write_bin(bin.data(), RLC_FC_BYTES, x[k]);
                            mIntersection.push_back(bin);
                        }
                    }
                }
            }
        }
        catch (...)
        {
            result = RLC_ERR;
        }

        gt_free(e);
        return result;
    }

    Proto LPSIReceiver::run(bn_t *inputs, Socket &chl)
    {
        setTimePoint("LpsiReceiver::run-begin");

        g2_t *d = new g2_t[mReceiverSize + 1];

        for (u64 i = 0; i <= mReceiverSize; i++)
        {
            g2_null(d[i]);
            g2_new(d[i]);
        }

        setTimePoint("LpsiReceiver::Setup");

        bn_t r;
        bn_null(r);
        bn_new(r);

        int ask_res = cp_pbpsi_ask(d, r, inputs, S, mReceiverSize);

        if (ask_res != RLC_OK)
            throw std::runtime_error("cp_pbpsi_ask failed");

        uint8_t buffer[4 * RLC_PC_BYTES + 1];
        memset(buffer, 0x00, 4 * RLC_PC_BYTES + 1);

        g2_write_bin(buffer, 4 * RLC_PC_BYTES + 1, d[0], 0);

        co_await chl.send(buffer);

        setTimePoint("LpsiReceiver::First Round");

        g1_t *u = new g1_t[mSenderSize];
        std::vector<uint8_t *> t(mSenderSize);

        for (u64 i = 0; i < mSenderSize; i++)
        {
            g1_null(u[i]);
            g1_new(u[i]);
            t[i] = new uint8_t[DIGEST_SIZE];
        }

        // Matrix로 메시지 수신
        const size_t itemSize = DIGEST_SIZE + (RLC_PC_BYTES + 1);
        for (u64 i = 0; i < mSenderSize; i++)
        {
            std::vector<uint8_t> itemBuffer(itemSize);
            co_await chl.recv(itemBuffer);

            // digest 복사
            memcpy(t[i], itemBuffer.data(), DIGEST_SIZE);
            // G1 원소 읽기
            g1_read_bin(u[i], itemBuffer.data() + DIGEST_SIZE, RLC_PC_BYTES + 1);
        }

        setTimePoint("LpsiReceiver::Second Round");

        // =============================
        // Third Round: 교집합 판정
        // =============================

        int inth_res = cp_pbpsi_inth(
            d, inputs,
            t, u);

        if (inth_res != RLC_OK)
            throw std::runtime_error("cp_pbpsi_inth failed");

        setTimePoint("LpsiReceiver::Intersecion retrieve");

        // =============================
        // Clean up
        // =============================

        for (u64 i = 0; i < mSenderSize; i++)
        {
            g1_free(u[i]);
            delete[] t[i];
        }
        for (u64 i = 0; i <= mReceiverSize; i++)
        {
            g2_free(d[i]);
        }

        bn_free(r);
        delete[] u;
        delete[] d;

        setTimePoint("LpsiReceiver::run-end");

        co_await chl.flush();
    }

} // namespace volePSI
