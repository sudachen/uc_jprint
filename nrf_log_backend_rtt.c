/* Copyright (c) 2016 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/*
 * Modified by Alexey Sudachen
 *  snprintf replaced by microPrint for smaller code size and memory footprint
 */

#include <sdk_common.h>

#if defined(NRF_LOG_ENABLED) && (NRF_LOG_ENABLED > 0)

#include <nrf_log_backend.h>
#include <nrf_error.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

void uccm$printUnsigned10(uint32_t value);
void uccm$printUnsigned16(uint32_t value, size_t width, bool uppercase);
void uccm$printFloat(float v, int width2);
void uccm$printChar(char c);
void uccm$printStr(const char *s);
void uccm$flushPrint();
void uccm$printComplete(bool complete);
bool uccm$irqCriticalEnter();
void uccm$irqCriticalExit(bool nested);

static void microPrintf(const char *fstr, size_t argno, uint32_t *args)
{
    uint8_t *fmt = (uint8_t*)fstr;
    bool nested = uccm$irqCriticalEnter();
    uccm$printComplete(false);

    for ( int j = 0 ; *fmt ; )
    {
        if ( *fmt == '%' && fmt[1] && fmt[1] != '%' )
        {
            int width1 = 0, width2 = -1;
            bool zfiller = false;
            bool uppercase = false;
            int what = 0;

            ++fmt;
            if ( *fmt == '0' && isdigit(fmt[1]) ) { zfiller = true; ++fmt; }
            if ( isdigit(*fmt) ) width1 = strtol((char*)fmt,(char**)&fmt,10);
            if ( *fmt == '.' )
            {
                ++fmt;
                if ( isdigit(*fmt) ) width2 = strtol((char*)fmt,(char**)&fmt,10);
            }
            if ( isupper(*fmt) ) uppercase = true;
            what = tolower(*fmt++);

            if ( j >= argno )
            {
                uccm$printStr("<noarg>");
                continue;
            }

            switch ( what )
            {
                case 'u':
                    uccm$printUnsigned10(args[j]);
                    break;

                case 'i':
                {
                    int32_t val = args[j];
                    if ( val < 0 )
                    {
                        val = -val;
                        uccm$printChar('-');
                    }
                    uccm$printUnsigned10(val);
                    break;
                }

                case 'x':
                    uccm$printUnsigned16(args[j],zfiller?width1:0,uppercase);
                    break;

                case 'f':
                    uccm$printFloat(*(float*)(args+j),width2);
                    break;

                case 'p':
                    uccm$printChar('#');
                    uccm$printUnsigned16(args[j],8,true);
                    break;

                case 's':
                    uccm$printStr((char*)args[j]);
                    break;
            }

            ++j;
        }
        else if ( *fmt == '%' && fmt[1] == '%' )
        {
            uccm$printChar('%');
            fmt+=2;
        }
        else
        {
            uccm$printChar(*fmt);
            ++fmt;
        }
    }

    uccm$flushPrint();
    uccm$irqCriticalExit(nested);
}

static const char m_default_color[] = "\x1B[0m";

ret_code_t nrf_log_backend_init(bool blocking)
{
    (void)blocking;
    return NRF_SUCCESS;
}

static bool nrf_log_backend_rtt_std_handler(
    uint8_t                severity_level,
    const uint32_t * const p_timestamp,
    const char * const     p_str,
    uint32_t             * p_args,
    uint32_t               nargs)
{
    microPrintf(p_str,nargs,p_args);
    if (NRF_LOG_USES_COLORS)
        uccm$printStr(m_default_color);
    return true;
}

static void byte2hex(const uint8_t c, char * p_out)
{
    uint8_t  nibble;
    uint32_t i = 2;

    while (i-- != 0)
    {
        nibble       = (c >> (4 * i)) & 0x0F;
        p_out[1 - i] = (nibble > 9) ? ('A' + nibble - 10) : ('0' + nibble);
    }
}

#define HEXDUMP_BYTES_PER_LINE               16
#define HEXDUMP_HEXBYTE_AREA                 3 // Two bytes for hexbyte and space to separate
#define HEXDUMP_MAX_STR_LEN (HEXDUMP_BYTES_PER_LINE*2 +         \
                            (HEXDUMP_HEXBYTE_AREA*HEXDUMP_BYTES_PER_LINE +\
                             4 +/* Color ANSI Escape Code */              \
                             2)) /* Separators */

static uint32_t nrf_log_backend_rtt_hexdump_handler(
    uint8_t                severity_level,
    const uint32_t * const p_timestamp,
    const char * const     p_str,
    uint32_t               offset,
    const uint8_t * const  p_buf0,
    uint32_t               buf0_length,
    const uint8_t * const  p_buf1,
    uint32_t               buf1_length)
{
    char     str[HEXDUMP_MAX_STR_LEN+1];
    char   * p_hex_part;
    char   * p_char_part;
    uint8_t  c;
    uint32_t byte_in_line;
    uint32_t byte_cnt      = offset;
    uint32_t length        = buf0_length + buf1_length;

    bool nested = uccm$irqCriticalEnter();
    uccm$printStr(p_str);

    do
    {
        uint32_t hex_part_offset  = 0;
        uint32_t char_part_offset = hex_part_offset +
                                    (HEXDUMP_BYTES_PER_LINE * HEXDUMP_HEXBYTE_AREA + 1);

        p_hex_part  = &str[hex_part_offset];
        p_char_part = &str[char_part_offset];

        for (byte_in_line = 0; byte_in_line < HEXDUMP_BYTES_PER_LINE; byte_in_line++)
        {
            if (byte_cnt >= length)
            {
                // file the blanks
                *p_hex_part++  = ' ';
                *p_hex_part++  = ' ';
                *p_hex_part++  = ' ';
                *p_char_part++ = ' ';
            }
            else
            {
                if (byte_cnt < buf0_length)
                {
                    c = p_buf0[byte_cnt];
                }
                else
                {
                    c = p_buf1[byte_cnt - buf0_length];
                }
                byte2hex(c, p_hex_part);
                p_hex_part    += 2; // move the pointer since byte in hex was added.
                *p_hex_part++  = ' ';
                *p_char_part++ = isprint(c) ? c : '.';
                byte_cnt++;
            }
        }
        *p_char_part++ = '\r';
        *p_char_part++ = '\n';
        *p_char_part++ = '\0';
        *p_hex_part++  = ' ';

        uccm$printStr(str);
        if (NRF_LOG_USES_COLORS)
            uccm$printStr(m_default_color);

    }
    while (byte_cnt < length);

    uccm$flushPrint();
    uccm$irqCriticalExit(nested);

    return byte_cnt;
}


nrf_log_std_handler_t nrf_log_backend_std_handler_get(void)
{
    return nrf_log_backend_rtt_std_handler;
}


nrf_log_hexdump_handler_t nrf_log_backend_hexdump_handler_get(void)
{
    return nrf_log_backend_rtt_hexdump_handler;
}

#endif
