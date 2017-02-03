
#pragma once
#pragma uccm require(source)+= [@inc]/~sudachen/uc_jprint/uc_jprint.c

#include <uccm/board.h>

/*

  void ucSetup_Print(void);
  void ucPutS(const char *text, bool complete);
    API defined in uccm/uccm.h

  it's an backend implementation module only

*/

#ifdef __nRF5x_UC__

#include <nrf_log.h>

#ifdef _DEBUG
#pragma uccm let(NRF_LOG_LEVEL)?=2
#else
#pragma uccm let(NRF_LOG_LEVEL)?=0
#endif

#pragma uccm file(sdk_config_1.h) ~= \
\n\
#ifndef NRF_LOG_ENABLED\n\
#if {$NRF_LOG_LEVEL} > 0\n\
#define NRF_LOG_ENABLED 1\n\
#endif\n\
#endif\n\
\n\
#ifndef NRF_LOG_DEFAULT_LEVEL\n\
#define NRF_LOG_DEFAULT_LEVEL {$NRF_LOG_LEVEL}\n\
#endif\n\
#ifndef NRF_LOG_USES_TIMESTAMP\n\
#define NRF_LOG_USES_TIMESTAMP 0\n\
#endif\n\
#ifndef NRF_LOG_USES_COLORS\n\
#define NRF_LOG_USES_COLORS 0\n\
#endif\n\
#ifndef NRF_LOG_DEFERRED\n\
#define NRF_LOG_DEFERRED 0\n\
#endif\n\
#ifndef NRF_LOG_BACKEND_MAX_STRING_LENGTH\n\
#define NRF_LOG_BACKEND_MAX_STRING_LENGTH 128\n\
#endif\n\
#ifndef NRF_LOG_BACKEND_SERIAL_USES_RTT\n\
#define NRF_LOG_BACKEND_SERIAL_USES_RTT 1\n\
#endif\n

#pragma uccm require(module)+= {NRF_LIBRARIES}/log/src/nrf_log_frontend.c
#pragma uccm require(source)+= [@inc]/~sudachen/uc_jprint/nrf_log_backend_rtt.c

#endif
