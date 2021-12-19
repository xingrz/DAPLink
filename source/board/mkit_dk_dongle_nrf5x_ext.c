/**
 * @file    mkit_dk_dongle_nrf5x_ext.c
 * @brief   board ID for the Nordic nRF5x developments boards
 *
 * DAPLink Interface Firmware
 * Copyright (c) 2009-2021, Arm Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sam3u2c.h"
#include "DAP_config.h"
#include "target_config.h"
#include "util.h"
#include "flash_manager.h"
#include "target_family.h"
#include "target_board.h"

// BOARD_ID pointer will be set during run time to point to one of these
const char *board_id_nrf51_mkit = "1070";
const char *board_id_nrf51_dk = "1100";
const char *board_id_nrf51_dongle = "1120";
const char *board_id_nrf52_dk = "1101";
const char *board_id_nrf52840_dk = "1102";

const char *board_name_nrf51_mkit = "nRF51822-mKIT";
const char *board_name_nrf51_dk = "nRF51-DK";
const char *board_name_nrf51_dongle = "nRF51-Dongle";
const char *board_name_nrf52_dk = "nRF52-DK";
const char *board_name_nrf52840_dk = "nRF52840-DK";
static char board_name[14];

extern target_cfg_t target_device_nrf51822_16;
extern target_cfg_t target_device_nrf51822_32;
extern target_cfg_t target_device_nrf52_64;
extern target_cfg_t target_device_nrf52840;
target_cfg_t target_device;
static uint8_t device_type;

static void set_target_device(uint32_t device)
{
    device_type = device;
    if (device == 0) {
        target_device = target_device_nrf51822_16;
    } else if (device == 1) {
        target_device = target_device_nrf51822_32;
    } else if (device == 2) {
        target_device = target_device_nrf52_64;
    } else if (device == 3) {
        target_device = target_device_nrf52840;
    }
}

static void nrf_prerun_board_config(void)
{
    // Work around for setting the correct board id based on GPIOs
    uint8_t bit1;
    uint8_t bit2;
    uint8_t bit3;

    PIOB->PIO_PER = (1 << 1); // Enable PIO pin PB1
    PIOB->PIO_PER = (1 << 2); // Enable PIO pin PB2
    PIOB->PIO_PER = (1 << 3); // Enable PIO pin PB3
    PIOB->PIO_ODR = (1 << 1); // Disabe output
    PIOB->PIO_ODR = (1 << 2); // Disabe output
    PIOB->PIO_ODR = (1 << 3); // Disabe output
    PIOB->PIO_PUER = (1 << 1); // Enable pull-up
    PIOB->PIO_PUER = (1 << 2); // Enable pull-up
    PIOB->PIO_PUER = (1 << 3); // Enable pull-up

    bit1 = (PIOB->PIO_PDSR >> 1) & 1; // Read PB1
    bit2 = (PIOB->PIO_PDSR >> 2) & 1; // Read PB2
    bit3 = (PIOB->PIO_PDSR >> 3) & 1; // Read PB3

    /* pins translate to board-ids as follow
     *
     *  PB3|PB2|PB1|BOARD ID| BOARD
     *  ----------------------------------
     *   0 | 0 | 0 |  1120  | nRF51-Dongle
     *   0 | 0 | 1 |  1100  | nRF51-DK
     *   0 | 1 | 0 |  1101  | nRF52-DK
     *   0 | 1 | 1 |  1102  | nRF52840-DK
     *   1 | 1 | 1 |  1070  | nRF51822-mKIT
     *   1 | 0 | 0 |    undefined
     *   1 | 0 | 1 |    undefined
     *   1 | 1 | 0 |    undefined
     */

    if (bit3) {
        set_target_device(0);
        target_device.rt_family_id = kNordic_Nrf51_FamilyID;
        target_device.rt_board_id = board_id_nrf51_mkit;  // 1070
        memcpy(board_name, board_name_nrf51_mkit, sizeof(board_name_nrf51_mkit));
        // Note only a setting of 111 is defined
        util_assert(bit2 && bit1);
    } else {
        if (!bit2 && bit1) {
            set_target_device(1);
            target_device.rt_family_id = kNordic_Nrf51_FamilyID;
            target_device.rt_board_id = board_id_nrf51_dk;  // 1100
            memcpy(board_name, board_name_nrf51_dk, sizeof(board_name_nrf51_dk));
        } else if (!bit2 && !bit1) {
            set_target_device(1);
            target_device.rt_family_id = kNordic_Nrf51_FamilyID;
            target_device.rt_board_id = board_id_nrf51_dongle;  // 1120
            memcpy(board_name, board_name_nrf51_dongle, sizeof(board_name_nrf51_dongle));
        } else if (bit2 && !bit1) {
            set_target_device(2);
            target_device.rt_family_id = kNordic_Nrf52_FamilyID;
            target_device.rt_board_id = board_id_nrf52_dk;  // 1101
            memcpy(board_name, board_name_nrf52_dk, sizeof(board_name_nrf52_dk));
        } else { //(bit2 && bit1)
            set_target_device(3);
            target_device.rt_family_id = kNordic_Nrf52_FamilyID;
            target_device.rt_board_id = board_id_nrf52840_dk;  // 1102
            memcpy(board_name, board_name_nrf52840_dk, sizeof(board_name_nrf52840_dk));
        }
    }

    PIOB->PIO_PUDR = (1 << 1); // Disable pull-up
    PIOB->PIO_PUDR = (1 << 2); // Disable pull-up
    PIOB->PIO_PUDR = (1 << 3); // Disable pull-up

    // EXTERNAL TARGET DETECTION
    // - supports nRF51-DK, nRF52-DK, nRF52840-DK
    // - nRF51-Dongle (discontinued) has no external target lines
    // - nRF51822-mKIT (discontinued), no external target lines
    // - need to code shield detection and priority between external and shield-mounted targets

    if ( !bit3 && (bit2 || bit1) ) {
        // 001 / 010 / 011 above
        
        // EXT_VTG (high if external target is powered)
        PIOB->PIO_PUDR = (1 << 6); // pull-up disable
        PIOB->PIO_ODR  = (1 << 6); // input
        PIOB->PIO_PER  = (1 << 6); // GPIO control
        bit1 = (PIOB->PIO_PDSR >> 6) & 1;  // Read PB6

        // EXT_GND_DETECT (high if no external target is connected)
        PIOB->PIO_PUER = (1 << 18); // pull-up enable
        PIOB->PIO_ODR  = (1 << 18); // input
        PIOB->PIO_PER  = (1 << 18); // GPIO control
        bit2 = (PIOB->PIO_PDSR >> 18) & 1; // Read PB18
        PIOB->PIO_PUDR = (1 << 18); // pull-up disable

        // if external target is detected, re-route SWD signals
        if (bit1 && !bit2) {
            pin_nRESET_port = PIN_EXT_nRESET_PORT;
            pin_nRESET_bit  = PIN_EXT_nRESET_BIT;
            pin_nRESET      = PIN_EXT_nRESET;

            pin_SWCLK_port  = PIN_EXT_SWCLK_PORT;
            pin_SWCLK_bit   = PIN_EXT_SWCLK_BIT;
            pin_SWCLK       = PIN_EXT_SWCLK;

            pin_SWDIO_port  = PIN_EXT_SWDIO_PORT;
            pin_SWDIO_bit   = PIN_EXT_SWDIO_BIT;
            pin_SWDIO       = PIN_EXT_SWDIO; 
        }

        // switch on red LED if external target is in use
        PIOA->PIO_PUDR = (1 << 28); // pull-up disable
        PIOA->PIO_OER  = (1 << 28); // output
        PIOA->PIO_PER  = (1 << 28); // GPIO control
        if (bit1 && !bit2)
            PIOA->PIO_CODR = (1 << 28); // set low
        else
            PIOA->PIO_SODR = (1 << 28); // set high        
    }
}

static void nrf_swd_set_target_reset(uint8_t asserted){
    if (asserted && device_type == 0) {
        PIOA->PIO_OER = PIN_SWDIO;
        PIOA->PIO_OER = PIN_SWCLK;
    } else if(!asserted) {
        PIOA->PIO_MDER = PIN_SWDIO | PIN_SWCLK | PIN_nRESET;
    }
}

const board_info_t g_board_info = {
    .info_version = kBoardInfoVersion,
    .flags = kEnablePageErase,
    .prerun_board_config = nrf_prerun_board_config,
    .swd_set_target_reset = nrf_swd_set_target_reset,
    .target_cfg = &target_device,
    .board_vendor = "Nordic Semiconductor",
    .board_name = board_name,
};
