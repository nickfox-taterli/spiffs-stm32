# SPIFFS Example (Base On STM32L4 DISCO)

------

Advantage:

> * Small (embedded) targets, sparse RAM without heap
> * Only big areas of data (blocks) can be erased
> * An erase will reset all bits in block to ones
> * Writing pulls one to zeroes
> * Zeroes can only be pulled to ones by erase
> * Wear leveling
> * Cache
> * Garbage Collecting

Disadvantages:

> * Non-Support For PC
> * Execution time uncertainty
> * Maximum partition only support up to 128MByte

### [How to Build]

> * Keil UV5.24.2.0 or later 
> * STM32L476 DISCO
> * Jlink-OB for RTT Debugger (It is free.)

------

## My configuration

- [x] Debug method:Segger RTT
- [x] Cache For Read
- [ ] Cache For Write[Unexpected power-lost has an impact on this.]
- [ ] Cache Statistics
- [x] Garbage Collecting
- [x] Flash Magic signature[Easy to detect new Flash]
- [ ] Mutex[You can migrate the operating system Mutex API.It is so easy.]
- [x] Single partition
- [x] Flash Size:16MB
- [x] Flash Sector:64K
- [x] Flash SubSector:4KB
- [x] Flash Page:256Bytes
- [x] SPIFFS Cache:(256 + 32) * 4 Bytes


### Main Code

```c
void SPIFFS_Main(void)
{
    SEGGER_RTT_Init();

    BSP_QSPI_Init();

    cfg.hal_erase_f = _spiffs_erase;
    cfg.hal_read_f = _spiffs_read;
    cfg.hal_write_f = _spiffs_write;

    if ((res = SPIFFS_mount(&fs,
                            &cfg,
                            FS_Work_Buf,
                            FS_FDS, sizeof(FS_FDS),
                            FS_Cache_Buf, sizeof(FS_Cache_Buf),
                            NULL)) != SPIFFS_OK &&
            SPIFFS_errno(&fs) == SPIFFS_ERR_NOT_A_FS)
    {
        SEGGER_RTT_printf("formatting spiffs...\n");
        if (SPIFFS_format(&fs) != SPIFFS_OK)
        {
            SEGGER_RTT_printf("SPIFFS format failed: %d\n", SPIFFS_errno(&fs));
        }
        SEGGER_RTT_printf("ok\n");
        SEGGER_RTT_printf("mounting\n");
        res = SPIFFS_mount(&fs,
                           &cfg,
                           FS_Work_Buf,
                           FS_FDS, sizeof(FS_FDS),
                           FS_Cache_Buf, sizeof(FS_Cache_Buf),
                           NULL);
    }
    if (res != SPIFFS_OK)
    {
        SEGGER_RTT_printf("SPIFFS mount failed: %d\n", SPIFFS_errno(&fs));
    }
    else
    {
        SEGGER_RTT_printf("SPIFFS mounted\n");
    }
    SEGGER_RTT_printf("errno %d\n", SPIFFS_errno(&fs));



    for(;;)
    {
        spiffs_file fd = SPIFFS_open(&fs, "my_file", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
        SEGGER_RTT_printf("errno %d\n", SPIFFS_errno(&fs));
        if (SPIFFS_write(&fs, fd, WriteBuf, sizeof(WriteBuf)) < 0)
            SEGGER_RTT_printf("errno %d\n", SPIFFS_errno(&fs));
        SPIFFS_close(&fs, fd);

        fd = SPIFFS_open(&fs, "my_file", SPIFFS_RDWR, 0);
        if (SPIFFS_read(&fs, fd, ReadBuf, sizeof(WriteBuf)) < 0)
            SEGGER_RTT_printf("errno %d\n", SPIFFS_errno(&fs));
        SPIFFS_close(&fs, fd);
        LL_mDelay(1000);
        SPIFFS_info(&fs, &total, &used_space);
        SEGGER_RTT_printf("mybuf = %s\n", ReadBuf);
    }
}
```