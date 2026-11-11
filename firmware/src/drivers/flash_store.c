#include "drivers/flash_store.h"

#include <string.h>

#include "stm32g0xx_hal.h"

#define STORE_ADDR 0x0807F800u // last 2 kB page, reserved in the linker script
#define STORE_MAGIC 0x42554844u // "DHUB"

typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t len;
    uint8_t reserved[2];
    uint8_t data[FLASH_STORE_MAX_LEN];
    uint32_t crc;
} record_t;

// Flash is programmed by double words.
#define RECORD_DWORDS ((sizeof(record_t) + 7u) / 8u)

static uint32_t crc32(const uint8_t *p, uint32_t n) {
    uint32_t crc = 0xFFFFFFFFu;
    while (n--) {
        crc ^= *p++;
        for (uint8_t i = 0; i < 8; i++) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

bool flash_store_load(uint8_t version, void *data, uint8_t len) {
    const record_t *r = (const record_t *)STORE_ADDR;
    if (len > FLASH_STORE_MAX_LEN || r->magic != STORE_MAGIC || r->version != version || r->len != len) {
        return false;
    }
    if (crc32((const uint8_t *)r, offsetof(record_t, crc)) != r->crc) {
        return false;
    }
    memcpy(data, r->data, len);
    return true;
}

bool flash_store_save(uint8_t version, const void *data, uint8_t len) {
    if (len > FLASH_STORE_MAX_LEN) {
        return false;
    }
    union {
        record_t r;
        uint64_t dw[RECORD_DWORDS];
    } buf;
    memset(&buf, 0xFF, sizeof buf);
    buf.r.magic = STORE_MAGIC;
    buf.r.version = version;
    buf.r.len = len;
    buf.r.reserved[0] = buf.r.reserved[1] = 0;
    memset(buf.r.data, 0, sizeof buf.r.data);
    memcpy(buf.r.data, data, len);
    buf.r.crc = crc32((const uint8_t *)&buf.r, offsetof(record_t, crc));

    // Page index inside its bank: on a dual-bank device the last page is
    // page 127 of bank 2, otherwise page 255 of the only bank.
    FLASH_EraseInitTypeDef erase = {.TypeErase = FLASH_TYPEERASE_PAGES, .NbPages = 1};
#if defined(FLASH_DBANK_SUPPORT)
    if (FLASH_BANK_NB == 2U) { // 512 kB parts are always dual-bank (RM0444 3.3.2)
        erase.Banks = FLASH_BANK_2;
        erase.Page = ((STORE_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE) - FLASH_PAGE_NB; // FLASH_PAGE_NB is per bank
    } else
#endif
    {
        erase.Banks = FLASH_BANK_1;
        erase.Page = (STORE_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE;
    }

    bool ok = false;
    uint32_t page_error = 0;
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_SR_CLEAR); // stale error flags would fail the erase
    if (HAL_FLASHEx_Erase(&erase, &page_error) == HAL_OK) {
        ok = true;
        for (uint32_t i = 0; i < RECORD_DWORDS && ok; i++) {
            ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, STORE_ADDR + 8u * i, buf.dw[i]) == HAL_OK;
        }
    }
    HAL_FLASH_Lock();
    return ok && memcmp((const void *)STORE_ADDR, &buf.r, sizeof(record_t)) == 0;
}
