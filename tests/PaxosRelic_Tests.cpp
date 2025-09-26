// #include "PaxosRelic_Tests.h"
// #include "volePSI/PaxosRelic.h"
// #include "volePSI/PxUtilRelic.h"
// #include "cryptoTools/Crypto/PRNG.h"
// #include "relic.h"
// #include <thread>
// #include <cmath>
// #include <unordered_set>
// #include <random>

// using namespace volePSI;

// auto& ZeroBlock = oc::ZeroBlock;
// auto& lout = oc::lout;
// using PRNG = oc::PRNG;
// using CLP = oc::CLP;

// // Helper function to initialize RELIC if not already done
// void ensureRelicInit() {
//     static bool initialized = false;
//     if (!initialized) {
//         if (core_init() != RLC_OK) {
//             throw std::runtime_error("Failed to initialize RELIC core");
//         }
//         if (pc_param_set_any() != RLC_OK) {
//             throw std::runtime_error("Failed to set RELIC parameters");
//         }
//         initialized = true;
//     }
// }

// // Helper function to generate random bn_t
// void randomBn(bn_t& result, PRNG& prng) {
//     uint8_t bytes[32];
//     prng.get(bytes, sizeof(bytes));
//     bn_read_bin(result, bytes, sizeof(bytes));
// }

// // Helper function to generate random g1_t
// void randomG1(g1_t& result, PRNG& prng) {
//     bn_t scalar;
//     bn_null(scalar);
//     bn_new(scalar);
//     randomBn(scalar, prng);
//     g1_mul_gen(result, scalar);
//     bn_free(scalar);
// }

// void RelicPaxos_buildRow_Test(const oc::CLP& cmd)
// {
//     ensureRelicInit();

//     u64 n = cmd.getOr("n", 1ull << cmd.getOr("nn", 10));
//     u64 t = cmd.getOr("t", 1ull << cmd.getOr("tt", 4));
//     auto s = cmd.getOr("s", 0);
    
//     RelicPaxosHash<u16> h;
//     h.init(n, 3, block(0, s));
    
//     std::vector<bn_t> keys(32);
//     std::vector<std::array<u16, 3>> rows(32);
//     PRNG prng(block(1, s));

//     // Initialize bn_t elements
//     for (auto& key : keys) {
//         bn_null(key);
//         bn_new(key);
//     }

//     for (u64 tt = 0; tt < t; ++tt)
//     {
//         // Generate random keys
//         for (auto& key : keys) {
//             randomBn(key, prng);
//         }

//         // Build rows from keys
//         for (u64 i = 0; i < 32; ++i) {
//             h.getRows(keys[i], span<u16>(rows[i].data(), 3));
            
//             // Verify uniqueness within each row
//             for (u64 j = 0; j < 3; ++j) {
//                 for (u64 k = j + 1; k < 3; ++k) {
//                     if (rows[i][j] == rows[i][k]) {
//                         throw std::runtime_error("Duplicate indices in row");
//                     }
//                 }
//             }
//         }
//     }

//     // Cleanup
//     for (auto& key : keys) {
//         bn_free(key);
//     }
// }

// void RelicPaxos_solve_Test(const oc::CLP& cmd)
// {
//     ensureRelicInit();

//     u64 n = cmd.getOr("n", 1ull << cmd.getOr("nn", 15));
//     u64 w = cmd.getOr("w", 3);
//     u64 s = cmd.getOr("s", 0);
//     u64 t = cmd.getOr("t", 1);

//     for (auto dt : { RelicPaxosParam::G1Element, RelicPaxosParam::BNElement })
//     {
//         for (u64 tt = 0; tt < t; ++tt)
//         {
//             RelicPaxos<u16> paxos;
//             RelicPaxos<u32> px2;
            
//             paxos.init(n, w, 40, dt, ZeroBlock);
//             px2.init(n, w, 40, dt, ZeroBlock);

//             std::vector<bn_t> keys(n);
//             std::vector<g1_t> values(n), values2(n);
//             std::vector<g1_t> p(paxos.size());
//             PRNG prng(block(tt, s));

//             // Initialize RELIC elements
//             for (u64 i = 0; i < n; ++i) {
//                 bn_null(keys[i]); bn_new(keys[i]);
//                 g1_null(values[i]); g1_new(values[i]);
//                 g1_null(values2[i]); g1_new(values2[i]);
                
//                 randomBn(keys[i], prng);
//                 randomG1(values[i], prng);
//             }
            
//             for (auto& elem : p) {
//                 g1_null(elem); g1_new(elem);
//             }

//             paxos.setInput(keys);
//             px2.setInput(keys);

//             // Verify that both paxos instances produce same row structure
//             for (u64 i = 0; i < paxos.mRows.rows(); ++i) {
//                 for (u64 j = 0; j < w; ++j) {
//                     auto v0 = paxos.mRows(i, j);
//                     auto v1 = px2.mRows(i, j);
//                     if (v0 != v1) {
//                         throw std::runtime_error("Row mismatch between paxos instances");
//                     }
//                 }
//             }

//             paxos.encode(values, p);
//             paxos.decode(keys, values2, p);

//             // Verify decode correctness
//             for (u64 i = 0; i < n; ++i) {
//                 if (g1_cmp(values2[i], values[i]) != RLC_EQ) {
//                     throw std::runtime_error("Decode verification failed");
//                 }
//             }

//             // Cleanup
//             for (u64 i = 0; i < n; ++i) {
//                 bn_free(keys[i]);
//                 g1_free(values[i]);
//                 g1_free(values2[i]);
//             }
//             for (auto& elem : p) {
//                 g1_free(elem);
//             }
//         }
//     }
// }

// void RelicPaxos_solve_g1_Test(const oc::CLP& cmd)
// {
//     ensureRelicInit();

//     u64 n = cmd.getOr("n", 1ull << cmd.getOr("nn", 15));
//     u64 w = cmd.getOr("w", 3);
//     u64 s = cmd.getOr("s", 0);
//     u64 t = cmd.getOr("t", 1);

//     for (u64 tt = 0; tt < t; ++tt)
//     {
//         RelicPaxos<u16> paxos;
//         paxos.init(n, w, 40, RelicPaxosParam::G1Element, ZeroBlock);

//         std::vector<bn_t> keys(n);
//         std::vector<g1_t> values(n), values2(n), p(paxos.size());
//         PRNG prng(block(tt, s));

//         // Initialize elements
//         for (u64 i = 0; i < n; ++i) {
//             bn_null(keys[i]); bn_new(keys[i]);
//             g1_null(values[i]); g1_new(values[i]);
//             g1_null(values2[i]); g1_new(values2[i]);
            
//             randomBn(keys[i], prng);
//             randomG1(values[i], prng);
//         }
        
//         for (auto& elem : p) {
//             g1_null(elem); g1_new(elem);
//         }

//         paxos.solve(keys, values, p, &prng);
//         paxos.decode(keys, values2, p);

//         for (u64 i = 0; i < n; ++i) {
//             if (g1_cmp(values2[i], values[i]) != RLC_EQ) {
//                 throw std::runtime_error("G1 solve test failed");
//             }
//         }

//         // Cleanup
//         for (u64 i = 0; i < n; ++i) {
//             bn_free(keys[i]);
//             g1_free(values[i]);
//             g1_free(values2[i]);
//         }
//         for (auto& elem : p) {
//             g1_free(elem);
//         }
//     }
// }

// void RelicPaxos_solve_mtx_Test(const oc::CLP& cmd)
// {
//     ensureRelicInit();

//     u64 n = cmd.getOr("n", 1ull << cmd.getOr("nn", 12));
//     u64 c = 4; // Number of columns in matrix
//     u64 w = cmd.getOr("w", 3);
//     u64 s = cmd.getOr("s", 0);
//     u64 t = cmd.getOr("t", 1);

//     for (u64 tt = 0; tt < t; ++tt)
//     {
//         RelicPaxos<u16> paxos;
//         paxos.init(n, w, 40, RelicPaxosParam::G1Element, ZeroBlock);

//         std::vector<bn_t> keys(n);
//         RelicG1Matrix values(n, c), values2(n, c);
//         RelicG1Matrix p(paxos.size(), c);
//         PRNG prng(block(tt, s));

//         // Initialize keys
//         for (u64 i = 0; i < n; ++i) {
//             bn_null(keys[i]); bn_new(keys[i]);
//             randomBn(keys[i], prng);
//         }

//         // Initialize values matrix
//         for (u64 i = 0; i < n; ++i) {
//             for (u64 j = 0; j < c; ++j) {
//                 randomG1(*values[i] + j, prng);
//             }
//         }

//         paxos.setInput(keys);

//         // Note: Matrix operations would need specialized encoding/decoding
//         // This is a simplified test
        
//         // Cleanup
//         for (u64 i = 0; i < n; ++i) {
//             bn_free(keys[i]);
//         }
//     }
// }

// void RelicPaxos_solve_gap_Test(const oc::CLP& cmd)
// {
//     ensureRelicInit();

//     u64 n = cmd.getOr("n", 1ull << cmd.getOr("nn", 10));
//     u64 w = cmd.getOr("w", 3);
//     u64 s = cmd.getOr("s", 0);
//     u64 t = cmd.getOr("t", 1);
//     u64 g = cmd.getOr("g", 3); // Gap size

//     for (u64 tt = 0; tt < t; ++tt)
//     {
//         RelicPaxos<u64> paxos;
//         RelicPaxosParam param(n, w, 40, RelicPaxosParam::G1Element);
//         paxos.init(n, param, ZeroBlock);

//         std::vector<bn_t> keys(n);
//         std::vector<g1_t> values(n), p(paxos.size());
//         PRNG prng(block(tt, s));

//         // Initialize elements
//         for (u64 i = 0; i < n; ++i) {
//             bn_null(keys[i]); bn_new(keys[i]);
//             g1_null(values[i]); g1_new(values[i]);
            
//             randomBn(keys[i], prng);
//             randomG1(values[i], prng);
//         }
        
//         for (auto& elem : p) {
//             g1_null(elem); g1_new(elem);
//         }

//         // Create some duplicate keys to force gaps
//         for (u64 i = 0; i < g && i < n/2; ++i) {
//             bn_copy(keys[n - 1 - i], keys[i]);
//         }

//         paxos.solve(keys, values, p, &prng);

//         // Test individual decoding
//         for (u64 i = 0; i < n - g; ++i) // Skip duplicate keys
//         {
//             g1_t decoded;
//             g1_null(decoded); g1_new(decoded);
            
//             std::array<bn_t, 1> keyArray = { keys[i] };
//             std::array<g1_t, 1> valueArray = { decoded };
            
//             paxos.decode(keyArray, valueArray, p);
            
//             if (g1_cmp(decoded, values[i]) != RLC_EQ) {
//                 g1_free(decoded);
//                 throw std::runtime_error("Gap test decode failed");
//             }
            
//             g1_free(decoded);
//         }

//         // Cleanup
//         for (u64 i = 0; i < n; ++i) {
//             bn_free(keys[i]);
//             g1_free(values[i]);
//         }
//         for (auto& elem : p) {
//             g1_free(elem);
//         }
//     }
// }

// void RelicPaxos_solve_rand_Test(const oc::CLP& cmd)
// {
//     ensureRelicInit();

//     u64 n = cmd.getOr("n", 1ull << cmd.getOr("nn", 10));
//     u64 s = cmd.getOr("s", 0);
//     u64 t = cmd.getOr("t", 1);

//     for (auto dt : { RelicPaxosParam::G1Element, RelicPaxosParam::BNElement })
//     {
//         for (u64 tt = 0; tt < t; ++tt)
//         {
//             RelicPaxos<u64> paxos;
//             RelicPaxosParam param(n, 3, 40, dt);
//             paxos.init(n, param, ZeroBlock);

//             std::vector<bn_t> keys(n);
//             std::vector<g1_t> values(n), values2(n), p(paxos.size());
//             PRNG prng(block(tt, s));

//             // Initialize elements
//             for (u64 i = 0; i < n; ++i) {
//                 bn_null(keys[i]); bn_new(keys[i]);
//                 g1_null(values[i]); g1_new(values[i]);
//                 g1_null(values2[i]); g1_new(values2[i]);
                
//                 randomBn(keys[i], prng);
//                 randomG1(values[i], prng);
//             }
            
//             for (auto& elem : p) {
//                 g1_null(elem); g1_new(elem);
//                 g1_set_infty(elem); // Initialize to zero
//             }

//             paxos.solve(keys, values, p, &prng);

//             // Verify randomization occurred
//             u64 nonZeroCount = 0;
//             for (const auto& elem : p) {
//                 if (!g1_is_infty(elem)) {
//                     nonZeroCount++;
//                 }
//             }
            
//             if (nonZeroCount == 0) {
//                 throw std::runtime_error("No randomization occurred");
//             }

//             paxos.decode(keys, values2, p);

//             for (u64 i = 0; i < n; ++i) {
//                 if (g1_cmp(values2[i], values[i]) != RLC_EQ) {
//                     throw std::runtime_error("Random solve test failed");
//                 }
//             }

//             // Cleanup
//             for (u64 i = 0; i < n; ++i) {
//                 bn_free(keys[i]);
//                 g1_free(values[i]);
//                 g1_free(values2[i]);
//             }
//             for (auto& elem : p) {
//                 g1_free(elem);
//             }
//         }
//     }
// }