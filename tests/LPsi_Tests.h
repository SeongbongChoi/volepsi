#pragma once

#include <cryptoTools/Common/CLP.h>
#include <volePSI/config.h>

// 기본 LPSI 테스트들
void LPsi_empty_test(const oc::CLP& cmd);     // 교집합이 비어 있는 경우
void LPsi_partial_test(const oc::CLP& cmd);   // 일부 교집합만 존재하는 경우
void LPsi_full_test(const oc::CLP& cmd);      // 교집합이 전체일 경우

// 실행 시간, 통신량 측정용 (옵션)
void LPsi_perf_test(const oc::CLP& cmd);
