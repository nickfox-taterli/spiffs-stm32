/* LL drivers common to all LL examples */
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_rcc.h"
#include "stm32l4xx_ll_system.h"
#include "stm32l4xx_ll_utils.h"
#include "stm32l4xx_ll_pwr.h"
#include "stm32l4xx_ll_exti.h"
#include "stm32l4xx_ll_gpio.h"
/* LL drivers specific to LL examples IPs */
#include "stm32l4xx_ll_adc.h"
#include "stm32l4xx_ll_comp.h"
#include "stm32l4xx_ll_cortex.h"
#include "stm32l4xx_ll_crc.h"
#include "stm32l4xx_ll_dac.h"
#include "stm32l4xx_ll_dma.h"
#include "stm32l4xx_ll_i2c.h"
#include "stm32l4xx_ll_iwdg.h"
#include "stm32l4xx_ll_lptim.h"
#include "stm32l4xx_ll_lpuart.h"
#include "stm32l4xx_ll_opamp.h"
#include "stm32l4xx_ll_rng.h"
#include "stm32l4xx_ll_rtc.h"
#include "stm32l4xx_ll_spi.h"
#include "stm32l4xx_ll_swpmi.h"
#include "stm32l4xx_ll_tim.h"
#include "stm32l4xx_ll_usart.h"
#include "stm32l4xx_ll_wwdg.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

#include "N25Q128.h"
#include "SEGGER_RTT.h"

#include "spiffs.h"
#include "spiffs_config.h"
#include "spiffs_nucleus.h"

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

    LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);

    if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_4)
    {

    }
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);

    LL_RCC_MSI_Enable();

    /* Wait till MSI is ready */
    while(LL_RCC_MSI_IsReady() != 1)
    {

    }
    LL_RCC_MSI_EnableRangeSelection();

    LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_6);

    LL_RCC_MSI_SetCalibTrimming(0);

    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_MSI, LL_RCC_PLLM_DIV_1, 40, LL_RCC_PLLR_DIV_2);

    LL_RCC_PLL_EnableDomain_SYS();

    LL_RCC_PLL_Enable();

    /* Wait till PLL is ready */
    while(LL_RCC_PLL_IsReady() != 1)
    {

    }
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);

    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

    /* Wait till System clock is ready */
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
    {

    }

    LL_SetSystemCoreClock(80000000);
}

static int32_t _spiffs_erase(uint32_t addr, uint32_t len)
{
    uint32_t i = 0;
    uint32_t erase_count = (len + 4096 - 1) / 4096;
    for (i = 0; i < erase_count; i++)
    {
        BSP_QSPI_Erase_Block(addr + i * 4096);
    }
    return 0;
}

static int32_t _spiffs_read(uint32_t addr, uint32_t size, uint8_t *dst)
{
    BSP_QSPI_Read(addr, size, dst);
    return 0;
}

static int32_t _spiffs_write(uint32_t addr, uint32_t size, uint8_t *dst)
{
    BSP_QSPI_Write(addr, size, dst);
    return 0;
}

spiffs_config cfg;
int32_t res;

spiffs fs;

char WriteBuf[] = {"Hi,Budy! if you get this Message......Congratulations!You have succeeded!!"};
char ReadBuf[80];

uint8_t FS_Work_Buf[256 * 2];
uint8_t FS_FDS[32 * 4];
uint8_t FS_Cache_Buf[(256 + 32) * 4];

uint32_t total = 0;
uint32_t used_space = 0;

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

int main(void)
{
    SystemClock_Config();
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC);

    xTaskCreate((TaskFunction_t)SPIFFS_Main, "SPIFFS", 128, NULL, 0, NULL);
    vTaskStartScheduler();
    while (1)
    {
    }

}


