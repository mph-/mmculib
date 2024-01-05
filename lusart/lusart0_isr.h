/** @file   lusart0_isr.h
    @author M. P. Hayes, UCECE
    @date   5 Jan 2024
    @brief
*/
#ifndef LUSART0_ISR_H
#define LUSART0_ISR_H

#ifdef __cplusplus
extern "C" {
#endif


#include "irq.h"
#include "usart0.h"
#include "usart0_defs.h"


#define USART0_TX_IRQ_ENABLED_P() ((USART0->US_IMR & US_CSR_TXEMPTY) != 0)

#define USART0_TX_IRQ_DISABLE()  (USART0->US_IDR = US_CSR_TXEMPTY)

#define USART0_TX_IRQ_ENABLE() (USART0->US_IER = US_CSR_TXEMPTY)

#define USART0_RX_IRQ_DISABLE() (USART0->US_IDR = US_CSR_RXRDY)

#define USART0_RX_IRQ_ENABLE() (USART0->US_IER = US_CSR_RXRDY)

#ifndef USART0_TX_FLOW_CONTROL
#define USART0_TX_FLOW_CONTROL
#endif

#ifndef USART0_IRQ_PRIORITY
#define USART0_IRQ_PRIORITY 4
#endif


static lusart_dev_t lusart0_dev;


static void
lusart0_tx_irq_enable (void)
{
    if (! USART0_TX_IRQ_ENABLED_P ())
        USART0_TX_IRQ_ENABLE ();
}


static void
lusart0_rx_irq_enable (void)
{
    USART0_RX_IRQ_ENABLE ();
}


static bool
lusart0_tx_finished_p (void)
{
    return USART0_WRITE_FINISHED_P ();
}


static void
lusart0_isr (void)
{
    lusart_dev_t *dev = &lusart0_dev;
    uint32_t status;

    status = USART0->US_CSR;

    if (USART0_TX_IRQ_ENABLED_P () && ((status & US_CSR_TXRDY) != 0))
    {
        if (dev->tx_in == dev->tx_out)
        {
            char ch;

            ch = dev->tx_buffer[dev->tx_in];
            dev->tx_out++;
            if (dev->tx_out >= dev->tx_size)
                dev->tx_out = 0;

            USART0_TX_FLOW_CONTROL;
            USART0_WRITE (ch);
        }
        else
            USART0_TX_IRQ_DISABLE ();
    }

    if ((status & US_CSR_RXRDY) != 0)
    {
        char ch;

        ch = USART0_READ ();

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
lusart0_init (uint16_t baud_divisor)
{
    lusart_dev_t *dev = &lusart0_dev;

    dev->tx_irq_enable = lusart0_tx_irq_enable;
    dev->rx_irq_enable = lusart0_rx_irq_enable;
    dev->tx_finished_p = lusart0_tx_finished_p;

    usart0_init (baud_divisor);

    irq_config (ID_USART0, USART0_IRQ_PRIORITY, lusart0_isr);

    irq_enable (ID_USART0);

    return dev;
}


#ifdef __cplusplus
}
#endif
#endif /* LUSART0_ISR_H  */
