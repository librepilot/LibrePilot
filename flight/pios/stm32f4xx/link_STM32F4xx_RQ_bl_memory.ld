MEMORY
{
    FLASH (rx)    : ORIGIN = 0x08000000,        LENGTH = 0x008000 - 0x000080
    BD_INFO (r)   : ORIGIN = 0x08008000 - 0x80, LENGTH =            0x000080
    RSVD (rx)     : ORIGIN = 0x08008000,        LENGTH = 0x020000 - 0x008000
    FW    (rx)    : ORIGIN = 0x08020000,        LENGTH = 0x1e0000
    CCSRAM (rw)   : ORIGIN = 0x10000000,        LENGTH = 0x010000
    SRAM (rwx)    : ORIGIN = 0x20000000,        LENGTH = 0x030000
}
