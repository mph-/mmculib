/** @file   gsm862.c
    @author M. P. Hayes, UCECE
    @date   4 August 2009
    @note   Driver for the GSM862 GSM module
*/

#include <string.h>
#include <stdio.h>
#include "pio.h"
#include "gsm862.h"
#include "target.h"
#include "delay.h"

/* Network requests can take a long time, up to a minute?
   Do we want the driver blocking for this time?
*/


#define GSM862_LINE_SIZE 80


#ifndef GSM862_ATTEMPTS
#define GSM862_ATTEMPTS 10
#endif


/* TX ring buffer size.  */
#ifndef GSM862_TX_SIZE
#define GSM862_TX_SIZE 16
#endif


/* RX ring buffer size.  */
#ifndef GSM862_RX_SIZE
#define GSM862_RX_SIZE 16
#endif


#ifdef GSM_DEBUG_PIO
#define GSM_DEBUG(state)(state) ? pio_output_high(GSM_DEBUG_PIO) : pio_output_low(GSM_DEBUG_PIO)
#else
#define GSM_DEBUG(state)
#endif


/* Currently, only support a single instance.  */
static gsm862_dev_t gsm862_dev;
static char gsm862_tx_buffer[GSM862_TX_SIZE];
static char gsm862_rx_buffer[GSM862_RX_SIZE];

char *
gsm862_gets (gsm862_t gsm, char *buffer, uint16_t size, uint16_t timeout_ms)
{
    int i;

    for (i = 0; i < size; i++)
    {
        if (!busart_read_ready_p (gsm->busart))
        {
            if (!timeout_ms)
                return NULL;
            delay_ms (1);
            timeout_ms--;
        }

        buffer[i] = busart_getc (gsm->busart);
        if (buffer[i] == '\n')
        {
            if (i != size - 1)
                buffer[i + 1] = '\0';
            break;
        }
    }
    return buffer;        
}


gsm862_ret_t 
gsm862_puts (gsm862_t gsm, const char *command)
{
    char line[GSM862_LINE_SIZE];
	
    busart_puts (gsm->busart, command);

    /* Read echoed command.  */
    return gsm862_gets (gsm, line, sizeof (line), 1000) ? GSM862_OK : GSM862_TIMEOUT;
}


static gsm862_ret_t
gsm862_response (gsm862_t gsm, char *response, uint16_t size,
                 gsm862_timeout_t timeout)
{
    char line[GSM862_LINE_SIZE];

    while (1)
    {
        char *str = line;
        
        if (!gsm862_gets (gsm, line, sizeof (line), timeout))
            return GSM862_TIMEOUT;
        
        if (line[0] == 'O' && line[1] == 'K')
            return GSM862_OK;                

        else if (line[5] == 'E' && line[6] == 'R' && line[7] == 'R' && line[12] == '3' && line[13] == '2' && line[14] == '1'){
            response[1]='E';
            response[2]='R';
            response[3]='R';
            return GSM862_OK;
        }

        else 
        {
            /* Copy response.  */
            for (; *str && size > 0; size--){
                *response++ = *str++;
            }
            return GSM862_OK;
        }
    }
}


static gsm862_ret_t
gsm862_at_command (gsm862_t gsm, const char *command, char *response,
                   uint16_t size, uint8_t max_attempts,
                   gsm862_timeout_t timeout)
{
    uint8_t attempts;

    /* FIXME, There might have to be a delay of 200 ms between commands.  */
    delay_ms(200);

    for (attempts = 0; attempts < max_attempts; attempts++)
    {
        if (!gsm862_puts (gsm, command) == GSM862_OK)
            continue;

        if (gsm862_response (gsm, response, size, timeout) == GSM862_OK)
            return GSM862_OK;                
    }
    return GSM862_TIMEOUT;
}


static bool
gsm862_poweron (gsm862_t gsm)
{
    pio_output_high (gsm->cfg->onoff);
    GSM_DEBUG (1);
    
    /* Wait for GSM to boot up.  */
    delay_ms (4000);
    
    /* Release onoff signal.  */
    pio_output_low (gsm->cfg->onoff);
    GSM_DEBUG (0);
    return 1;
}


bool
gsm862_shutdown (gsm862_t gsm)
{
    return gsm862_at_command (gsm, "AT#SHDN\r", NULL, 0, 10, 1000) == GSM862_OK;
}


/* Max of 160 characters.  */
gsm862_ret_t 
gsm862_sms_send (gsm862_t gsm, const char *txt, const char *destination)
{
    char line[GSM862_LINE_SIZE];
    gsm862_ret_t ret;
    
    strcpy (line, "AT+CMGS=\"");
    strcat (line, destination);
    strcat (line, "\"\r");

    ret = gsm862_puts (gsm, line);
    if (!ret == GSM862_OK)
        return ret;

    /* Read prompt "> "  */
    if (!gsm862_gets (gsm, line, 2, 1000))
        return GSM862_TIMEOUT;
                
    if (line[0] != '>')
        return GSM862_MYSTERY;

    /* Download the message.  */
    busart_puts (gsm->busart, txt);

    /* Read echoed message.  */
    if (!gsm862_gets (gsm, line, strlen (txt), 1000))
        return GSM862_TIMEOUT;

    /* Send ctrl-z to send message or ESC to abort.  */
#ifdef GSM862_NOSEND
    busart_puts (gsm->busart, "\033");
#else
    busart_puts (gsm->busart, "\032");
#endif

    return gsm862_response (gsm, line, sizeof (line), 1000);
}


gsm862_ret_t 
gsm862_sms_read (gsm862_t gsm, uint8_t index, char *response, uint16_t size)
{
    char line[GSM862_LINE_SIZE];
    sprintf (line, "AT+CMGR=%d\r", index);
    return gsm862_at_command (gsm, line, response, size, 1, 5000);
}


gsm862_t
gsm862_init (const gsm862_cfg_t *cfg)
{
    gsm862_t gsm = &gsm862_dev;

    pio_config_set (cfg->onoff, PIO_OUTPUT_HIGH);
    pio_config_set (cfg->reset, PIO_OUTPUT_HIGH);
    pio_config_set (cfg->pwrmon, PIO_INPUT);
    pio_config_set (GSM_DEBUG_PIO, PIO_OUTPUT_HIGH);

    gsm->cfg = cfg;
    gsm->busart = busart_init (cfg->channel, cfg->baud_divisor,
                               gsm862_tx_buffer, sizeof (gsm862_tx_buffer),
                               gsm862_rx_buffer, sizeof (gsm862_rx_buffer));

    gsm862_poweron (gsm);

    delay_ms (3000);

    gsm862_at_command (gsm, "AT+IPR=38400\r", NULL, 0, GSM862_ATTEMPTS, 2000);
    gsm862_at_command (gsm, "AT&K0\r", NULL,0, GSM862_ATTEMPTS, 2000);
    gsm862_at_command (gsm, "AT+CMGF=1\r", NULL, 0, GSM862_ATTEMPTS, 2000);

    #ifdef GSM_DEBUG_PIO
    pio_config_set (GSM_DEBUG_PIO, PIO_OUTPUT_LOW);
    #endif
	
    GSM_DEBUG (1);
    delay_ms (15000);
    //gsm862_at_command (gsm, "AT+CMGD=1\r", NULL, 0, GSM862_ATTEMPTS, 2000);
    GSM_DEBUG (0);
    return gsm;
}


void
gsm862_gps_locate (gsm862_t gsm)
{
    char gpsloc[200];

    gsm862_at_command (gsm, "AT$GPSP?", gpsloc, 199, 1, 5000);
    gsm862_sms_send (gsm, gpsloc, "02102565676");
}


void
gsm862_gprs_init (gsm862_t gsm)
{
    gsm862_at_command (gsm, "AT+CGDCONT=1,IP,http://www.vodafone.net.nz,0.0.0.0\r", NULL, 0, 1, 5000);
    gsm862_at_command (gsm, "AT#USERID=\r", NULL, 0, 1, 5000);
    gsm862_at_command (gsm, "AT#PASSW=\r", NULL, 0, 1, 5000);
    gsm862_at_command (gsm, "AT#PKTSZ=300\r", NULL, 0, 1, 5000);
    gsm862_at_command (gsm, "AT#DSTO=50\r", NULL, 0, 1, 5000);
    gsm862_at_command (gsm, "AT#SKTTO=60000\r", NULL, 0, 1, 5000);
    gsm862_at_command (gsm, "AT#SKTCT=600\r", NULL, 0, 1, 5000);
    gsm862_at_command (gsm, "AT#SKTSET=0,80,www.agresearch.co.nz,0,0\r", NULL, 0, 1, 5000);
    gsm862_at_command (gsm, "AT#SKTSAV\r", NULL, 0, 1, 5000);
}


void
gsm862_gprs_senddata (gsm862_t gsm, char *data)
{
    char connectconfirm[9];
    char datahtml[1001];
    gsm862_at_command (gsm, "AT#SKTOP\r", connectconfirm, 9, 1, 20000);
    gsm862_at_command (gsm, "GET /about/whoweare.asp HTTP/1.1\r\n", datahtml, 50, 1, 20000);
}


char
gsm862_control (gsm862_t gsm)
{
    char lastsms[200];
    gsm862_sms_read(gsm, 1, lastsms, 199);
    if((lastsms[1]=='E' && lastsms[2]=='R' && lastsms[3]=='R'))
    {
        return 'N';
    }
    else
    {
        int i = 0;
        int n = 0;
        int u = 0;
        int m = 0;
        int uflag = 0;
        int uflag2 = 0;
        char unumber[15];
        //checking user
        for(u=0; u<100; u++){
            if(lastsms[u] ==',' && uflag == 0){
                uflag2 = 1;
            }
            if (uflag2 == 1){
                if(lastsms[u+2] =='"'){
                    uflag = 1;
                }
                unumber[m]= lastsms[(u+2)];
                m++;
            }
        }
        unumber[0] = '+';
        unumber[1] = '6';
        unumber[2] = '4';
        unumber[3] = '2';
        unumber[4] = '1';
        unumber[5] = '0';
        unumber[6] = '2';
        unumber[7] = '5';
        unumber[8] = '6';
        unumber[9] = '5';
        unumber[10] = '6';
        unumber[11] = '7';
        unumber[12] = '6';
        if(!(unumber[0] == '+' && unumber[1] == '6' && unumber[2] == '4' && unumber[3] == '2' && unumber[4] == '1' && unumber[5] == '0' && unumber[6] == '2' && unumber[7] == '5' && unumber[8] == '6' && unumber[9] == '5' && unumber[10] == '6' && unumber[11] == '7' && unumber[12] == '6')){
            //gsm862_at_command (gsm, "AT+CMGD=1\r", NULL, 0, GSM862_ATTEMPTS, 2000);
            return 'N';
        }
        //finished checking user
        for(i=0; i<200; i++){
            if(lastsms[i] == '\r'){
                n = (i+2);
            }
        }
        if((lastsms[n]== 'G' && lastsms[(n+1)]== 'P' && lastsms[(n+2)]== 'S' && lastsms[(n+3)]== 'L')){
            gsm862_gps_locate(gsm);
            gsm862_at_command (gsm, "AT+CMGD=1\r", NULL, 0, GSM862_ATTEMPTS, 2000);			
            return 'G';
        }
        else if((lastsms[n]== 'S' && lastsms[(n+1)]== 'H' && lastsms[(n+2)]== 'U' && lastsms[(n+3)]== 'T')){
            gsm862_at_command (gsm, "AT+CMGD=1\r", NULL, 0, GSM862_ATTEMPTS, 2000);
            gsm862_shutdown(gsm);
            return 'S';
        }
        else if((lastsms[n]== 'P' && lastsms[(n+1)]== 'I' && lastsms[(n+2)]== 'C' && lastsms[(n+3)]== 'T') || 1){
            gsm862_gprs_init(gsm);
            gsm862_gprs_senddata(gsm, lastsms);
            //gsm862_at_command (gsm, "AT+CMGD=1\r", NULL, 0, GSM862_ATTEMPTS, 2000);
            return 'P';
        }
        else if((lastsms[n]== 'F' && lastsms[(n+1)]== 'R' && lastsms[(n+2)]== 'E' && lastsms[(n+3)]== 'Q')){
            gsm862_at_command (gsm, "AT+CMGD=1\r", NULL, 0, GSM862_ATTEMPTS, 2000);
            return 'F';
        }
    }
}
