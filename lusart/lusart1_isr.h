/** @file   lusart1_isr.h
    @author M. P. Hayes, UCECE
    @date   5 Jan 2024
    @brief
*/
#ifndef LUSART1_ISR_H
#define LUSART1_ISR_H

#ifdef __cplusplus
extern "C" {
#endif


#include "irq.h"
#include "usart1.h"
#include "usart1_defs.h"


#define USART1_TX_IRQ_ENABLED_P() ((USART1->US_IMR & US_CSR_TXEMPTY) != 0)

#define USART1_TX_IRQ_DISABLE()  (USART1->US_IDR = US_CSR_TXEMPTY)

#define USART1_TX_IRQ_ENABLE() (USART1->US_IER = US_CSR_TXEMPTY)

#define USART1_RX_IRQ_DISABLE() (USART1->US_IDR = US_CSR_RXRDY)

#define USART1_RX_IRQ_ENABLE() (USART1->US_IER = US_CSR_RXRDY)

#ifndef USART1_TX_FLOW_CONTROL
#define USART1_TX_FLOW_CONTROL
#endif

#ifndef USART1_IRQ_PRIORITY
#define USART1_IRQ_PRIORITY 4
#endif


static lusart_dev_t lusart1_dev;


static void
lusart1_tx_irq_enable (void)
{
    if (! USART1_TX_IRQ_ENABLED_P ())
        USART1_TX_IRQ_ENABLE ();
}


static void
lusart1_rx_irq_enable (void)
{
    USART1_RX_IRQ_ENABLE ();
}


static bool
lusart1_tx_finished_p (void)
{
    return USART1_WRITE_FINISHED_P ();
}


static void
lusart1_isr (void)
{
    lusart_dev_t *dev = &lusart1_dev;
    uint32_t status;

    status = USART1->US_CSR;

    if (USART1_TX_IRQ_ENABLED_P () && ((status & US_CSR_TXRDY) != 0))
    {
        if (dev->tx_in == dev->tx_out)
        {
            char ch;

            ch = dev->tx_buffer[dev->tx_in];
            dev->tx_out++;
            if (dev->tx_out >= dev->tx_size)
                dev->tx_out = 0;

            USART1_TX_FLOW_CONTROL;
            USART1_WRITE (ch);
        }
        else
            USART1_TX_IRQ_DISABLE ();
    }

    if ((status & US_CSR_RXRDY) != 0)
    {
        char ch;

        ch = USART1_READ ();

        /* What about buffer overflow?  */
        dev->rx_buffer[dev->rx_in] = ch;
        if (ch == '\n')
            dev->rx_nl_in++;

        dev->rx_in++;
        if (dev->rx_in >= dev->rx_size)
            dev->rx_in = 0;
    }
}


static lusart_dev_t *
lusart1_init (uint16_t baud_divisor)
{
    lusart_dev_t *dev = &lusart1_dev;

    dev->tx_irq_enable = lusart1_tx_irq_enable;
    dev->rx_irq_enable = lusart1_rx_irq_enable;
    dev->tx_finished_p = lusart1_tx_finished_p;

    usart1_init (baud_divisor);

    irq_config (ID_USART1, USART1_IRQ_PRIORITY, lusart1_isr);

    irq_enable (ID_USART1);

    return dev;
}


#ifdef __cplusplus
}
#endif
#endif /* LUSART1_ISR_H  */
