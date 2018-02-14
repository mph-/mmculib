#ifndef TARGET_H
#define TARGET_H

#ifdef __cplusplus
extern "C" {
#endif
    

/* CPU clock frequency.  */
#define F_CPU 8000000

/* Buttons.  */
#define BUTTON1_PORT PORT_D     /* INT0 */
#define BUTTON1_BIT 2
#define BUTTON2_PORT PORT_D     /* INT1 */
#define BUTTON2_BIT 3


/* LEDs.  */
#define LED1_PORT PORT_C
#define LED1_BIT 2
#define LED2_PORT PORT_B
#define LED2_BIT 2
#define LED3_PORT PORT_B
#define LED3_BIT 1
#define LED4_PORT PORT_D
#define LED4_BIT 4
#define LED5_PORT PORT_D
#define LED5_BIT 6
#define LED6_PORT PORT_C
#define LED6_BIT 3
#define LED7_PORT PORT_B
#define LED7_BIT 7
#define LED8_PORT PORT_B
#define LED8_BIT 0
#define LED9_PORT PORT_C
#define LED9_BIT 5
#define LED10_PORT PORT_D
#define LED10_BIT 5
#define LED11_PORT PORT_C
#define LED11_BIT 4
#define LED12_PORT PORT_B
#define LED12_BIT 6
#define LED13_PORT PORT_D
#define LED13_BIT 7



/* NB, to simplify track routing, the following pins are connected together:
   PD0 (RXD) - PC0 (ADC0)
   PD1 (TXD) - ADC7
   PC6 (RESET) - PC1 (ADC1)
*/



#ifdef __cplusplus
}
#endif    
#endif

