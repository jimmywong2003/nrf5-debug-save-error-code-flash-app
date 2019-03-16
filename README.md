# nrf5-debug-save-error-code-flash-app

This example is to demo how to write the error information inside the internal flash during the app error handling.

For example, the address of the flash region is defined

```
#define N52_FLASH_PAGESIZE 4096
#define N52_PAGE_NUM       100

uint32_t pg_size = N52_FLASH_PAGESIZE;
uint32_t pg_num  = N52_PAGE_NUM;       // Use last page in flash

```

We add the routine of flash operation by using the NVMC register directly.


```
/** @brief Function for erasing a page in flash.
 *
 * @param page_address Address of the first word in the page to be erased.
 */
static void flash_page_erase(uint32_t * page_address)
{
        // Turn on flash erase enable and wait until the NVMC is ready:
        NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);

        while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
        {
                // Do nothing.
        }

        // Erase page:
        NRF_NVMC->ERASEPAGE = (uint32_t)page_address;

        while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
        {
                // Do nothing.
        }

        // Turn off flash erase enable and wait until the NVMC is ready:
        NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);

        while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
        {
                // Do nothing.
        }
}

/** @brief Function for filling a page in flash with a value.
 *
 * @param[in] address Address of the first word in the page to be filled.
 * @param[in] value Value to be written to flash.
 */
static void flash_word_write(uint32_t * address, uint32_t value)
{
        // Turn on flash write enable and wait until the NVMC is ready:
        NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);

        while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
        {
                // Do nothing.
        }

        *address = value;

        while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
        {
                ; // Do nothing.
        }

        // Turn off flash write enable and wait until the NVMC is ready:
        NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);

        while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
        {
                // Do nothing.
        }
}

```

Inside the app_error_save_stop routine, it adds to store the error on flash.

```

void app_error_save_and_stop(uint32_t id, uint32_t pc, uint32_t info)
{
...
        case NRF_FAULT_ID_SDK_ERROR:
                m_error_data.p_error_info = (error_info_t *)info;
                m_error_data.err_code     = m_error_data.p_error_info->err_code;
                m_error_data.line_num     = m_error_data.p_error_info->line_num;
                m_error_data.p_file_name  = m_error_data.p_error_info->p_file_name;
                path_addr=(uint32_t *)m_error_data.p_error_info->p_file_name;


                // Erase page:
                flash_page_erase(flash_addr);
                flash_word_write(flash_addr, m_error_data.err_code);
                flash_addr++;
                flash_word_write(flash_addr, m_error_data.line_num);
                flash_addr++;
                for(store_length=0; store_length<1000;)
                {

                        if(m_error_data.p_file_name[store_length]!=0)
                        {
                                flash_word_write(flash_addr, *path_addr);
                                flash_addr++;
                                path_addr++;
                                if(m_error_data.p_file_name[store_length+1]==0  \
                                   || m_error_data.p_file_name[store_length+2]==0 \
                                   || m_error_data.p_file_name[store_length+3]==0)
                                {
                                        break;
                                }
                                store_length+=4;
                        }
                        else
                        {
                                break;
                        }

                }

                break;
```

## Requirement

* Nordic SDK 15.2
* NRF52832 DK
* IDE : Segger Embedded Studio

You can place this example inside the SDK15.2/example/ble_peripheral folder.




