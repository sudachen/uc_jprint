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

#include <sdk_common.h>

#if defined(NRF_LOG_ENABLED) && (NRF_LOG_ENABLED > 0)

#include <nrf_log_backend.h>
#include <nrf_error.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

void uc_jprint$print(int channel, const char *text, bool complete);

#define HEXDUMP_BYTES_PER_LINE               16
#define HEXDUMP_HEXBYTE_AREA                 3 // Two bytes for hexbyte and space to separate
#define HEXDUMP_MAX_STR_LEN (NRF_LOG_BACKEND_MAX_STRING_LENGTH -          \
                            (HEXDUMP_HEXBYTE_AREA*HEXDUMP_BYTES_PER_LINE +\
                             4 +/* Color ANSI Escape Code */              \
                             2)) /* Separators */

static bool m_initialized   = false;
static bool m_blocking_mode = false;
static const char m_default_color[] = "\x1B[0m";

ret_code_t nrf_log_backend_init(bool blocking)
{

    if (m_initialized && (blocking == m_blocking_mode))
    {
        return NRF_SUCCESS;
    }
    
    m_initialized   = true;
    m_blocking_mode = blocking;
    return NRF_SUCCESS;
}

static bool buf_len_update(uint32_t * p_buf_len, int32_t new_len)
{
    bool ret;
    if (new_len < 0)
    {
        ret = false;
    }
    else
    {
        *p_buf_len += (uint32_t)new_len;
        ret = true;
    }
    return ret;
}

static bool nrf_log_backend_rtt_std_handler(
    uint8_t                severity_level,
    const uint32_t * const p_timestamp,
    const char * const     p_str,
    uint32_t             * p_args,
    uint32_t               nargs)
{
    char     str[NRF_LOG_BACKEND_MAX_STRING_LENGTH+1];
    int32_t  tmp_str_len     = 0;
    uint32_t buffer_len      = 0;
    bool     status          = true;

    switch (nargs)
    {
        case 0:
        {
            tmp_str_len = strlen(p_str);
            if ((tmp_str_len + buffer_len) < NRF_LOG_BACKEND_MAX_STRING_LENGTH)
            {
                memcpy(&str[buffer_len], p_str, tmp_str_len);
            }
            break;
        }

        case 1:
            tmp_str_len = snprintf(&str[buffer_len], NRF_LOG_BACKEND_MAX_STRING_LENGTH-buffer_len, p_str, p_args[0]);

            break;

        case 2:
            tmp_str_len = snprintf(&str[buffer_len], NRF_LOG_BACKEND_MAX_STRING_LENGTH-buffer_len, p_str, p_args[0], p_args[1]);
            break;

        case 3:
            tmp_str_len = snprintf(&str[buffer_len], NRF_LOG_BACKEND_MAX_STRING_LENGTH-buffer_len, p_str, p_args[0], p_args[1], p_args[2]);
            break;

        case 4:
            tmp_str_len =
                snprintf(&str[buffer_len], NRF_LOG_BACKEND_MAX_STRING_LENGTH-buffer_len, p_str, p_args[0], p_args[1], p_args[2], p_args[3]);
            break;

        case 5:
            tmp_str_len =
                snprintf(&str[buffer_len],
                        NRF_LOG_BACKEND_MAX_STRING_LENGTH-buffer_len,
                        p_str,
                        p_args[0],
                        p_args[1],
                        p_args[2],
                        p_args[3],
                        p_args[4]);
            break;

        case 6:
            tmp_str_len =
                snprintf(&str[buffer_len],
                        NRF_LOG_BACKEND_MAX_STRING_LENGTH-buffer_len,
                        p_str,
                        p_args[0],
                        p_args[1],
                        p_args[2],
                        p_args[3],
                        p_args[4],
                        p_args[5]);
            break;

        default:
            break;
    }
    status = buf_len_update(&buffer_len, tmp_str_len);
    uint32_t full_buff_len = NRF_LOG_USES_COLORS ?
            buffer_len + sizeof(m_default_color)-1 : buffer_len;
    if (status && (full_buff_len <= NRF_LOG_BACKEND_MAX_STRING_LENGTH))
    {
        if (NRF_LOG_USES_COLORS)
        {
            memcpy(&str[buffer_len], m_default_color, sizeof(m_default_color)-1);
            buffer_len = full_buff_len;
        }
        str[buffer_len] = 0;
        uc_jprint$print(1,str,false);
        return true;
    }
    else
    {
        // error, snprintf failed.
        return false;
    }
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
    char     str[NRF_LOG_BACKEND_MAX_STRING_LENGTH+1];
    uint32_t slen;
    char   * p_hex_part;
    char   * p_char_part;
    uint8_t  c;
    uint32_t byte_in_line;
    uint32_t buffer_len    = 0;
    uint32_t byte_cnt      = offset;
    uint32_t length        = buf0_length + buf1_length;

    // If it is the first part of hexdump print the header
    if (offset == 0)
    {
        slen = strlen(p_str);
        // Saturate string if it's too long.
        slen = (slen > HEXDUMP_MAX_STR_LEN) ? HEXDUMP_MAX_STR_LEN : slen;
        memcpy(&str[buffer_len], p_str, slen);
        buffer_len += slen;
    }

    do
    {
        uint32_t hex_part_offset  = buffer_len;
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
        *p_hex_part++  = ' ';
        buffer_len    +=
                         (HEXDUMP_BYTES_PER_LINE * HEXDUMP_HEXBYTE_AREA + 1) + // space for hex dump and separator between hexdump and string
                         HEXDUMP_BYTES_PER_LINE +                              // space for stringS dump
                         2;                                                    // space for new line
        if (NRF_LOG_USES_COLORS)
        {
            memcpy(&str[buffer_len], m_default_color, sizeof(m_default_color)-1);
            buffer_len +=  sizeof(m_default_color)-1;
        }

        str[buffer_len] = 0;
        uc_jprint$print(1,str,false);
        buffer_len = 0;
    }
    while (byte_cnt < length);
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
