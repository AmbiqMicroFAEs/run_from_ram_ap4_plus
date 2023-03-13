// Run from RAM example
// Mariusz Lacina, Ambiq, 2023
//*****************************************************************************
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <arm_math.h>
#include "apollo4p.h"
#include "am_bsp.h"
#include "am_util.h"

//=============================================================================
//
// Timer used to measure time.
//
//=============================================================================
#define TIMER_NUM       0       // Timer number to be used in the example

//=============================================================================
//
// RAM function.
//
//=============================================================================
#define RAM_FUNC_SIZE	512
extern float32_t arm_sin_f32(float32_t); //CMSIS DSP sin()

// Select the SRAM type used:
//unsigned char ram_func_buf[RAM_FUNC_SIZE]; 
//unsigned char *ram_func_buf = (unsigned char*)0x10060000+2000; //System SRAM
unsigned char *ram_func_buf = (unsigned char*)0x10030000+2000; //TCM

typedef float32_t (*ram_func_t)(float32_t in); 
ram_func_t ram_func;


//=============================================================================
//
// Memory config
//
//=============================================================================
void memory_config(void)
{
			
		am_hal_pwrctrl_mcu_memory_config_t McuMemCfg =
    {
      .eCacheCfg    = AM_HAL_PWRCTRL_CACHE_ALL,
      .bRetainCache = true,
      .eDTCMCfg     = AM_HAL_PWRCTRL_DTCM_384K,
      .eRetainDTCM  = AM_HAL_PWRCTRL_DTCM_384K,
      .bEnableNVM0  = true,
      .bRetainNVM0  = false
    };

    am_hal_pwrctrl_sram_memcfg_t SRAMMemCfg =
    {
      .eSRAMCfg         = AM_HAL_PWRCTRL_SRAM_ALL,
      .eActiveWithMCU   = AM_HAL_PWRCTRL_SRAM_NONE,
      .eActiveWithDSP   = AM_HAL_PWRCTRL_SRAM_NONE,
      .eSRAMRetain      = AM_HAL_PWRCTRL_SRAM_ALL
    };
		
		am_hal_pwrctrl_dsp_memory_config_t ExtSRAMMemCfg =
    {
    .bEnableICache      = false,  //Should always be "false"
    .bRetainCache       = false,  //Should always be "false"
    .bEnableRAM         = true,   //Controls Extended RAM power when MCU awake
    .bActiveRAM         = false,  //Should be "false"
    .bRetainRAM         = true 	  //true configures Extended RAM to be retained in deep sleep
    };
		
		
    am_hal_pwrctrl_mcu_memory_config(&McuMemCfg);
    am_hal_pwrctrl_sram_config(&SRAMMemCfg);
		am_hal_pwrctrl_dsp_memory_config(AM_HAL_DSP0, &ExtSRAMMemCfg);
		am_hal_pwrctrl_dsp_memory_config(AM_HAL_DSP1, &ExtSRAMMemCfg);	
}

//=============================================================================
//
// Timer init
//
//=============================================================================
void timer_init(void)
{
am_hal_timer_config_t   TimerConfig;

    //
    // Set up the desired TIMER.
    // The default config parameters include:
    //  eInputClock = AM_HAL_TIMER_CLOCK_HFRC_DIV16
    //  eFunction = AM_HAL_TIMER_FN_EDGE
    //  Compare0 and Compare1 maxed at 0xFFFFFFFF
    //
    am_hal_timer_default_config_set(&TimerConfig);

    //
    // Modify the default parameters.
    // Configure the timer to a 1s period via ui32Compare1.
    //
    TimerConfig.eFunction			= AM_HAL_TIMER_FN_UPCOUNT;
		//TimerConfig.eInputClock  	= AM_HAL_TIMER_CLOCK_HFRC_DIV4;
   
    //
    // Configure the timer
    //
		am_hal_timer_config(TIMER_NUM, &TimerConfig);
   
    //
    // Clear the timer and its interrupt
    //
    am_hal_timer_clear(TIMER_NUM);
    

} // timer_init()

//=============================================================================
//
// Main function.
//
//=============================================================================
int main(void)
{
float32_t s1, s2, arg;
uint32_t	exec_time;
	
    //
    // Set the default cache configuration
    //
    am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
    am_hal_cachectrl_enable();

    //
    // Configure the board for low power operation.
    //
    am_bsp_low_power_init();
	
		//
		// Configure the memory 
		//
		memory_config();
	
	
		//
		// LED config
		//
		am_devices_led_array_init(am_bsp_psLEDs, AM_BSP_NUM_LEDS);
		//
    // Turn the LEDs off, but initialize LED1 on so user will see something.
    //
    for (int ix = 0; ix < AM_BSP_NUM_LEDS; ix++)
    {
        am_devices_led_off(am_bsp_psLEDs, ix);
    }
		
    //
    // Global interrupt enable
    //
    am_hal_interrupt_master_enable();

    //
    // Enable printing to the console.
    //
		// Check the config in am_bsp.c:  static am_bsp_print_interface_e g_ePrintInterface = AM_BSP_PRINT_IF_UART;
		//
    am_bsp_debug_printf_enable();
		am_util_stdio_printf("\nApollo4 Plus run from RAM demo!\r\n");

		am_util_delay_ms(1000);	
		
		//
		// Cope the CMSIS DSP sin func to RAM buffer
		//
		memcpy(ram_func_buf,(void*)((uint32_t)&arm_sin_f32 & ~1),RAM_FUNC_SIZE);

			
		am_hal_timer_clear_stop(TIMER_NUM);
	
		//
		// Initialize RAM func pointer
		//
		ram_func = (ram_func_t)&ram_func_buf[1];
					
		//
		//Calling SIN() from SRAM
		//	
		arg = 1.2;	
		am_hal_timer_clear(TIMER_NUM);
		s1 = ram_func(arg);
		exec_time = am_hal_timer_read(TIMER_NUM);
			
		am_util_stdio_printf("\nRAM (SRAM) SIN() Result: SIN(%f)= %f\r\n",arg,s1);
		am_util_stdio_printf("RAM (SRAM) SIN() exec time= %u\r\n",exec_time);			
			
		//
		//Calling SIN() from ROM (MRAM)
		//	
		arg = 1.2;		
		am_hal_timer_clear(TIMER_NUM);	
		s2 = arm_sin_f32(arg);
		exec_time = am_hal_timer_read(TIMER_NUM);	
			
		am_util_stdio_printf("\nROM (MRAM) SIN() Result: SIN(%f)= %f\r\n",arg,s2);		
		am_util_stdio_printf("ROM (MRAM) SIN() exec time= %u\r\n",exec_time);	
		
		//
		//Compare results
		// 	
		if(s1 != s2)
			{
				am_util_stdio_printf("\nRAM SIN and ROM SIN results don't match!\r\n");
			}else
			{
				am_util_stdio_printf("\nRAM SIN and ROM SIN results match!\r\n");
			}
 
			
			
			
		//
    // Loop forever.
    //
    while (1)
    {
		am_devices_led_toggle(am_bsp_psLEDs, 0);
		am_util_delay_ms(1000);	
    }

} // main()
