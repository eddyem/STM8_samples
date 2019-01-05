// configure UART
    PORT(UART_PORT, DDR) |= UART_TX_PIN; // output
    PORT(UART_PORT, ODR) |= UART_TX_PIN; // torn off N push-down
    // For open-drain output comment next line
    PORT(UART_PORT, CR1) |= UART_TX_PIN; // push-pull
    // 9 bit, no parity, 1 stop (UART_CR3 = 0 - reset value)
    // 57600 on 16MHz: BRR1=0x11, BRR2=0x06
    UART2_BRR1 = 0x11; UART2_BRR2 = 0x06;
    // UART2_CR1  = UART_CR1_M; // M = 1 -- 9bits
    // for several uarts on line don't write UART_CR2_TEN here, write it when need
    UART2_CR2  = UART_CR2_TEN | UART_CR2_REN | UART_CR2_RIEN; // Allow Tx/RX, generate ints on rx
