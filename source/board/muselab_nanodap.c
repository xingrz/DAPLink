/**
 * @file    muselab_nanodap.c
 * @brief   board information for MuseLab nanoDAP
 *
 * DAPLink Interface Firmware
 * Copyright (c) 2022 XiNGRZ
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

#include "target_family.h"
#include "target_board.h"
#include "stm32f1xx.h"
#include "DAP_Config.h"
#include "DAP.h"

const board_info_t g_board_info = {
    .info_version = kBoardInfoVersion,
    .board_id = "0000",
    .family_id = kStub_HWReset_FamilyID,
    .target_cfg = &target_device,
    .board_vendor = "MuseLab",
    .board_name = "nanoDAP",
};

static SPI_HandleTypeDef SPI_Handle;

static inline void SPI1_ENABLE(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;

    GPIO_InitStructure.Pin = SWCLK_TCK_PIN;
    HAL_GPIO_Init(SWCLK_TCK_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.Pin = SWDIO_OUT_PIN;
    HAL_GPIO_Init(SWDIO_OUT_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.Pin = SWDIO_IN_PIN;
    HAL_GPIO_Init(SWDIO_IN_PIN_PORT, &GPIO_InitStructure);

    __HAL_RCC_SPI1_CLK_ENABLE();
    SPI_Handle.Instance = SPI1;
    SPI_Handle.Init.Mode = SPI_MODE_MASTER;
    SPI_Handle.Init.Direction = SPI_DIRECTION_2LINES;
    SPI_Handle.Init.DataSize = SPI_DATASIZE_8BIT;
    SPI_Handle.Init.CLKPolarity = SPI_POLARITY_HIGH;
    SPI_Handle.Init.CLKPhase = SPI_PHASE_2EDGE;
    SPI_Handle.Init.NSS = SPI_NSS_SOFT;
    SPI_Handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
    HAL_SPI_Init(&SPI_Handle);
}

static inline void SPI1_DISABLE(void) {
    __HAL_RCC_SPI1_CLK_DISABLE();
    HAL_SPI_DeInit(&SPI_Handle);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.Pull = GPIO_PULLUP;

    GPIO_InitStructure.Pin = SWCLK_TCK_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(SWCLK_TCK_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.Pin = SWDIO_OUT_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(SWDIO_OUT_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.Pin = SWDIO_IN_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(SWDIO_IN_PIN_PORT, &GPIO_InitStructure);
}

#if 0
void SWJ_Sequence(uint32_t count, const uint8_t *data) {
    SPI1_ENABLE();
    HAL_SPI_Transmit(&SPI_Handle, (uint8_t *)data, count / 8, HAL_MAX_DELAY);
    SPI1_DISABLE();
}
#endif

#if 0
void SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi) {
    uint32_t val;
    uint32_t n;

    SPI1_ENABLE();

    n = info & SWD_SEQUENCE_CLK;
    if (n == 0U) {
        n = 64U;
    }

    if (info & SWD_SEQUENCE_DIN) {
        HAL_SPI_Receive(&SPI_Handle, swdi, n / 8, HAL_MAX_DELAY);
    } else {
        HAL_SPI_Transmit(&SPI_Handle, (uint8_t *)swdo, n / 8, HAL_MAX_DELAY);
    }

    SPI1_DISABLE();
}
#endif

// SW Macros

#define PIN_SWCLK_SET PIN_SWCLK_TCK_SET
#define PIN_SWCLK_CLR PIN_SWCLK_TCK_CLR

#define SW_CLOCK_CYCLE()                \
    PIN_SWCLK_CLR();                      \
    PIN_DELAY();                          \
    PIN_SWCLK_SET();                      \
    PIN_DELAY()

#define SW_WRITE_BIT(bit)               \
    PIN_SWDIO_OUT(bit);                   \
    PIN_SWCLK_CLR();                      \
    PIN_DELAY();                          \
    PIN_SWCLK_SET();                      \
    PIN_DELAY()

#define SW_READ_BIT(bit)                \
    PIN_SWCLK_CLR();                      \
    PIN_DELAY();                          \
    bit = PIN_SWDIO_IN();                 \
    PIN_SWCLK_SET();                      \
    PIN_DELAY()

#define PIN_DELAY() PIN_DELAY_SLOW(DAP_Data.clock_delay)

static inline void SWD_TURNAROUND() {
    for (uint32_t n = DAP_Data.swd_conf.turnaround; n; n--) {
        SW_CLOCK_CYCLE();
    }
}

#if 0
uint8_t SWD_Transfer(uint32_t request, uint32_t *data) {
    uint32_t ack;
    uint32_t bit;
    uint32_t val;
    uint32_t parity;
    uint32_t n;

    /* Packet Request */
    SPI1_ENABLE();
    parity = 0U;
    for (n = 0; n < 4; n++) {
        parity += (request >> n);
    }
    request |= ((parity & 0x1) << 5);   /* Parity Bit */
    request |= (0U << 5);               /* Stop Bit */
    request |= (1U << 5);               /* Park Bit */
    HAL_SPI_Transmit(&SPI_Handle, (uint8_t *)&request, 1, HAL_MAX_DELAY);
    SPI1_DISABLE();

    /* Turnaround */
    PIN_SWDIO_OUT_DISABLE();
    SWD_TURNAROUND();

    /* Acknowledge response */
    SW_READ_BIT(bit);
    ack  = bit << 0;
    SW_READ_BIT(bit);
    ack |= bit << 1;
    SW_READ_BIT(bit);
    ack |= bit << 2;

    /* OK response */
    if (ack == DAP_TRANSFER_OK) {
        /* Data transfer */
        if (request & DAP_TRANSFER_RnW) {
            /* Read data */
            SPI1_ENABLE();
            val = 0U;
            parity = 0U;
            HAL_SPI_Receive(&SPI_Handle, (uint8_t *)&val, 4, HAL_MAX_DELAY);
            for (n = 0; n < 31U; n++) {
                parity += ((val >> n) & 0x1);
            }
            if ((parity ^ (val >> 31)) & 1U) {
                ack = DAP_TRANSFER_ERROR;
            }
            if (data) { * data = val; }
            SPI1_DISABLE();
            /* Turnaround */
            SWD_TURNAROUND();
            PIN_SWDIO_OUT_ENABLE();
        } else {
            /* Turnaround */
            SWD_TURNAROUND();
            PIN_SWDIO_OUT_ENABLE();
            /* Write data */
            SPI1_ENABLE();
            val = *data;
            parity = 0U;
            for (n = 0; n < 31U; n++) {
                parity += (val >> n);
            }
            val |= ((parity & 0x1) << 31);
            HAL_SPI_Transmit(&SPI_Handle, (uint8_t *)&val, 4, HAL_MAX_DELAY);
            SPI1_DISABLE();
        }
        /* Capture Timestamp */
        if (request & DAP_TRANSFER_TIMESTAMP) {
            DAP_Data.timestamp = TIMESTAMP_GET();
        }
        /* Idle cycles */
        n = DAP_Data.transfer.idle_cycles;
        if (n) {
            PIN_SWDIO_OUT(0U);
            for (; n; n--) {
                SW_CLOCK_CYCLE();
            }
        }
        PIN_SWDIO_OUT(1U);
        return ((uint8_t)ack);
    }

    /* WAIT or FAULT response */
    if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT)) {
        if (DAP_Data.swd_conf.data_phase && ((request & DAP_TRANSFER_RnW) != 0U)) {
            for (n = 32U + 1U; n; n--) {
                SW_CLOCK_CYCLE();   /* Dummy Read RDATA[0:31] + Parity */
            }
        }
        /* Turnaround */
        SWD_TURNAROUND();
        PIN_SWDIO_OUT_ENABLE();
        if (DAP_Data.swd_conf.data_phase && ((request & DAP_TRANSFER_RnW) == 0U)) {
            PIN_SWDIO_OUT(0U);
            for (n = 32U + 1U; n; n--) {
                SW_CLOCK_CYCLE();   /* Dummy Write WDATA[0:31] + Parity */
            }
        }
        PIN_SWDIO_OUT(1U);
        return ((uint8_t)ack);
    }

    /* Protocol error */
    SWD_TURNAROUND();
    for (n = 32U + 1U; n; n--) {
        SW_CLOCK_CYCLE();   /* Back off data phase */
    }
    PIN_SWDIO_OUT_ENABLE();
    PIN_SWDIO_OUT(1U);

    return ((uint8_t)ack);
}
#endif
