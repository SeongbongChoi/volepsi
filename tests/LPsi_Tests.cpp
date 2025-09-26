#include "LPsi_Tests.h"
#include "volePSI/LPsi.h"

#include "cryptoTools/Network/IOService.h"
#include "cryptoTools/Network/Session.h"
#include "coproto/Socket/LocalAsyncSock.h"
#include "Common.h"

using namespace std;
using namespace volePSI;
using namespace oc;
using coproto::LocalAsyncSocket;

namespace
{
    // 공통 실행 함수
    std::vector<std::array<uint8_t, RLC_FC_BYTES>> runLpsi(
        bn_t *recvSet,
        bn_t *sendSet,
        u64 n,
        u64 m,
        u64 nt,
        u64 &interSize) // 🔹 교집합 크기를 받아올 참조
    {

        auto sockets = LocalAsyncSocket::makePair();

        LPSIReceiver recver;
        LPSISender sender;

        recver.init(n, m, 40, oc::toBlock(0, 0), nt);
        sender.init(n, m, 40, oc::toBlock(1, 1), nt);

        auto p0 = recver.run(recvSet, sockets[0]);
        auto p1 = sender.run(sendSet, sockets[1]);

        eval(p0, p1);

        interSize = recver.mIntersection.size();
        return recver.mIntersection;
    }

}

// =============================
// 테스트 함수들
// =============================

void LPsi_empty_test(const oc::CLP &cmd)
{
    if (core_init() != RLC_OK || pc_param_set_any() != RLC_OK)
    {
        throw std::runtime_error("RELIC init failed");
    }

    u64 logn = cmd.getOr("n", 10); // 기본 2^10
    u64 logm = cmd.getOr("m", 10);

    u64 n = 1ull << logn;
    u64 m = 1ull << logm;

    bn_t *sendSet = RLC_ALLOCA(bn_t, n);
    bn_t *recvSet = RLC_ALLOCA(bn_t, m);

    for (u64 i = 0; i < n; ++i)
    {
        bn_null(sendSet[i]);
        bn_new(sendSet[i]);
        bn_set_dig(sendSet[i], i + n + 1000); // 교집합 없음
    }
    for (u64 i = 0; i < m; ++i)
    {
        bn_null(recvSet[i]);
        bn_new(recvSet[i]);
        bn_set_dig(recvSet[i], i + 1);
    }

    u64 interSize;
    auto inter = runLpsi(recvSet, sendSet, n, m, 1, interSize);

    if (interSize != 0) // 교집합이 없어야 함
        throw RTE_LOC;
}

void LPsi_partial_test(const oc::CLP &cmd)
{
    if (core_init() != RLC_OK || pc_param_set_any() != RLC_OK)
    {
        throw std::runtime_error("RELIC init failed");
    }

    u64 logn = cmd.getOr("n", 10);
    u64 logm = cmd.getOr("m", 10);

    u64 n = 1ull << logn;
    u64 m = 1ull << logm;

    bn_t *sendSet = RLC_ALLOCA(bn_t, n);
    bn_t *recvSet = RLC_ALLOCA(bn_t, m);

    // RELIC 원소들 초기화 (필수!)
    for (u64 i = 0; i < n; ++i)
    {
        bn_null(sendSet[i]);
        bn_new(sendSet[i]);
        bn_set_dig(sendSet[i], i + 1);
    }

    for (u64 i = 0; i < m; ++i)
    {
        bn_null(recvSet[i]);
        bn_new(recvSet[i]);
        bn_set_dig(recvSet[i], i + 1);
    }

    // 절반 정도 교집합 제거
    std::set<u64> exp;
    for (u64 i = 0; i < n; ++i)
    {
        if (i % 2 == 0)
        {
            exp.insert(i + 1);
        }
        else
        {
            bn_set_dig(sendSet[i], i + n + 1000);
        }
    }

    u64 interSize;
    auto inter = runLpsi(recvSet, sendSet, n, m, 1, interSize);

    if (interSize != exp.size())
        throw RTE_LOC;
}

void LPsi_full_test(const oc::CLP &cmd)
{
    if (core_init() != RLC_OK || pc_param_set_any() != RLC_OK)
    {
        throw std::runtime_error("RELIC init failed");
    }

    u64 logn = cmd.getOr("n", 10);
    u64 logm = cmd.getOr("m", 10);

    u64 n = 1ull << logn;
    u64 m = 1ull << logm;

    bn_t *sendSet = RLC_ALLOCA(bn_t, n);
    bn_t *recvSet = RLC_ALLOCA(bn_t, m);

    // RELIC 원소들 초기화 (필수!)
    for (u64 i = 0; i < n; ++i)
    {
        bn_null(sendSet[i]);
        bn_new(sendSet[i]);
        bn_set_dig(sendSet[i], i + 1);
    }

    for (u64 i = 0; i < m; ++i)
    {
        bn_null(recvSet[i]);
        bn_new(recvSet[i]);
        bn_set_dig(recvSet[i], i + 1);
    }

    u64 interSize;
    auto inter = runLpsi(recvSet, sendSet, n, m, 1, interSize);

    if (interSize != n) // 교집합 = 전체
        throw RTE_LOC;
}

void LPsi_perf_test(const oc::CLP &cmd)
{
    if (core_init() != RLC_OK || pc_param_set_any() != RLC_OK)
    {
        throw std::runtime_error("RELIC init failed");
    }

    u64 logn = cmd.getOr("n", 10);
    u64 logm = cmd.getOr("m", 10);
    u64 nt = cmd.getOr("nt", 1);

    u64 n = 1ull << logn;
    u64 m = 1ull << logm;

    bn_t *sendSet = RLC_ALLOCA(bn_t, n);
    bn_t *recvSet = RLC_ALLOCA(bn_t, m);

    bn_t q;
    bn_null(q);
    bn_new(q);
    pc_get_ord(q);

    for (u64 i = 0; i < n; ++i)
    {
        bn_null(sendSet[i]);
        bn_new(sendSet[i]);
        bn_rand_mod(sendSet[i], q);
    }

    for (u64 i = 0; i < m; ++i)
    {
        bn_null(recvSet[i]);
        bn_new(recvSet[i]);
        bn_rand_mod(recvSet[i], q);
    }

    bn_free(q);

    Timer timer, s, r;
    auto sockets = LocalAsyncSocket::makePair();

    LPSIReceiver recver;
    LPSISender sender;

    recver.init(n, m, 40, oc::toBlock(0, 0), nt);
    sender.init(n, m, 40, oc::toBlock(1, 1), nt);

    recver.setTimer(r);
    sender.setTimer(s);

    u64 interSize;
    auto p0 = recver.run(recvSet, sockets[0]);
    auto p1 = sender.run(sendSet, sockets[1]);

    timer.setTimePoint("start");

    s.setTimePoint("start");
    r.setTimePoint("start");

    eval(p0, p1);

    timer.setTimePoint("end");

    std::cout << "Total Comm = "
              << (double)(sockets[0].bytesSent() + sockets[1].bytesSent()) / (1 << 20)
              << " MB" << std::endl;
    std::cout << timer << std::endl;
    std::cout << "Sender Timer\n"
              << s << "\nReceiver Timer\n"
              << r << std::endl;

    // co_await sockets[0].close();
    // co_await sockets[1].close();
}
