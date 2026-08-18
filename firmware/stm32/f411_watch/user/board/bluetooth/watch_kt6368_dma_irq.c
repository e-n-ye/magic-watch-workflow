#include "board/bluetooth/watch_kt6368_board.h"

void DMA2_Stream7_IRQHandler(void)
{
    watch_kt6368_board_dma_tx_irq();
}
