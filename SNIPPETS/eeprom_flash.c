/**
 * Change variable stored in FLASH
 * !!! You can change only const values (non-constants are initializes on program start)
 * @param addr - variable address
 * @param new value
 * @return 0 in case of error
 */
U8 change_progmem_value(U8 *addr, U8 val){
    // unlock memory
    FLASH_PUKR = EEPROM_KEY2;
    FLASH_PUKR = EEPROM_KEY1;
    // check bit PUL=1 in FLASH_IAPSR
    if(!FLASH_IAPSR & 0x02)
        return 0;
    *addr = val;
    // clear PUL to lock write
    FLASH_IAPSR &= ~0x02;
    return 1;
}

/**
 * Change data in EEPROM
 * @param addr - EEPROM address
 * @param new value
 * @return 0 in case of error
 */
U8 change_eeprom_value(U8 *addr, U8 val){
    // unlock eeprom
    FLASH_DUKR = EEPROM_KEY1;
    FLASH_DUKR = EEPROM_KEY2;
    // check bit DUL=1 in FLASH_IAPSR
    if(!FLASH_IAPSR & 0x08)
        return 0;
    *addr = val;
    // clear DUL to lock write
    FLASH_IAPSR &= ~0x08;
    return 1;
}
*/
