/**
 * Copyright (c) 2014 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup app_error Common application error handler
 * @{
 * @ingroup app_common
 *
 * @brief Common application error handler.
 */

#include "nrf.h"
#include <stdio.h>
#include "app_error.h"
#include "nordic_common.h"
#include "sdk_errors.h"

/**@brief Function for error handling, which is called when an error has occurred.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of error.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name.
 */
void app_error_handler_bare(ret_code_t error_code)
{
        error_info_t error_info =
        {
                .line_num    = 0,
                .p_file_name = NULL,
                .err_code    = error_code,
        };

        app_error_fault_handler(NRF_FAULT_ID_SDK_ERROR, 0, (uint32_t)(&error_info));

        UNUSED_VARIABLE(error_info);
}

/***************************************************************************************/


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

#define N52_FLASH_PAGESIZE 4096
#define N52_PAGE_NUM       100

uint32_t pg_size = N52_FLASH_PAGESIZE;
uint32_t pg_num  = N52_PAGE_NUM;       // Use last page in flash
/*************************************************************************************************/

//void ble_stack_stop(void)
//{
//    uint32_t err_code;
//
//    err_code = nrf_sdh_disable_request();
//    APP_ERROR_CHECK(err_code);
//
//    ASSERT(!nrf_sdh_is_enabled());
//}


void app_error_save_and_stop(uint32_t id, uint32_t pc, uint32_t info)
{
        uint32_t err_code;
        /* static error variables - in order to prevent removal by optimizers */
        static volatile struct
        {
                uint32_t fault_id;
                uint32_t pc;
                uint32_t error_info;
                assert_info_t * p_assert_info;
                error_info_t  * p_error_info;
                ret_code_t err_code;
                uint32_t line_num;
                const uint8_t * p_file_name;
        } m_error_data = {0};

        uint32_t * flash_addr;
        uint32_t * path_addr;
        uint16_t store_length;

        // The following variable helps Keil keep the call stack visible, in addition, it can be set to
        // 0 in the debugger to continue executing code after the error check.
        volatile bool loop = true;
        UNUSED_VARIABLE(loop);

        err_code = nrf_sdh_disable_request();
//    APP_ERROR_CHECK(err_code);

//        softdevice_handler_sd_disable();
        __disable_irq();

        // Start address:
        flash_addr = (uint32_t *)(pg_size * pg_num);

        m_error_data.fault_id   = id;
        m_error_data.pc         = pc;
        m_error_data.error_info = info;

        switch (id)
        {
        case NRF_FAULT_ID_SDK_ASSERT:
                m_error_data.p_assert_info = (assert_info_t *)info;
                m_error_data.line_num      = m_error_data.p_assert_info->line_num;
                m_error_data.p_file_name   = m_error_data.p_assert_info->p_file_name;
                break;

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
        }

        UNUSED_VARIABLE(m_error_data);

        // If printing is disrupted, remove the irq calls,
        // or set the loop variable to 0 in the debugger.
        //__disable_irq();
        while (loop);

        __enable_irq();
}
