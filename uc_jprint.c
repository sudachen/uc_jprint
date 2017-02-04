

#include <~sudachen/uc_jprint/import.h>
#include <sdk_config.h>
#if defined(NRF_LOG_ENABLED) && (NRF_LOG_ENABLED > 0)
#include <nrf_log.h>
#include <nrf_log_ctrl.h>
#endif

typedef struct {
  const     char*    name;          // Optional name. Standard names so far are: "Terminal", "SysView", "J-Scope_t4i4"
            char*    buffer;        // Pointer to start of buffer
            uint32_t sizeOfBuffer;  // Buffer size in bytes.
                                    // Note that one byte is lost, as this implementation does not fill up the buffer
                                    // in order to avoid the problem of being unable to distinguish between full and empty.
  volatile  uint32_t wrOff;         // Position of next item to be written by either target/host.
  volatile  uint32_t rdOff;         // Position of next item to be read by host/target.
            uint32_t flags;         // Contains configuration flags
} SEGGER_RTT_BUFFER;

typedef struct {
  char              acID[16];       // Initialized to "SEGGER RTT"
  int32_t           maxnum[2];
  SEGGER_RTT_BUFFER bf;
} SEGGER_RTT_CB;

char uc_jprint$Buffer[UC_PRINT_BUFFER_SIZE];

SEGGER_RTT_CB uc_jprint$cb =
{
    .acID = { 249, 239, 237, 237, 239, 248, 138, 248, 254, 254, 0},
    .maxnum = { 1, 0 },
    .bf =
    {
        .name          = "ucPrint",
        .buffer        = uc_jprint$Buffer,
        .sizeOfBuffer  = sizeof(uc_jprint$Buffer),
        .rdOff         = 0u,
        .wrOff         = 0u,
        .flags         = 0u,
    }
};

void uc_jprint$initCB()
{
    if ( uc_jprint$cb.acID[0] != 'S' )
        for ( char *p = uc_jprint$cb.acID; *p; ++p ) *p ^= 0xaa;
}

#define INIT_CB() do { if ( uc_jprint$cb.acID[0] != 'S' ) uc_jprint$initCB(); } while(0)

void ucPutS(const char *text, bool complete)
{
    static bool inactive = false;

    SEGGER_RTT_BUFFER *bf;
    uint32_t wrOff;

    INIT_CB();

    bf = &uc_jprint$cb.bf;

    wrOff = bf->wrOff;

    while ( *text )
    {
        uint32_t rdOff = bf->rdOff;
        uint32_t bfSpace = rdOff > wrOff ? rdOff - wrOff - 1u : bf->sizeOfBuffer - (wrOff - rdOff - 1u);

        if ( bfSpace == 0 )
        {
            if ( !complete )
            {
                if ( inactive ) return;
                inactive = true;
            }
            for ( int count = 1000; count; --count ) {
                __NOP();
                if ( bf->rdOff != rdOff ) break;
            }
            continue;
        }

        inactive = false;

        for (; *text && bfSpace; --bfSpace, ++text )
          bf->buffer[(wrOff++)%bf->sizeOfBuffer] = *text;
        bf->wrOff = wrOff%bf->sizeOfBuffer;
    }
}

void ucSetup_Print(void)
{
    INIT_CB();
#if defined(NRF_LOG_ENABLED) && (NRF_LOG_ENABLED > 0)
    NRF_LOG_INIT(0);
#endif
}

