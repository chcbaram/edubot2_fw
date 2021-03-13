/*
 * esp.c
 *
 *  Created on: 2021. 2. 13.
 *      Author: baram
 */


#include "esp.h"
#include "cli.h"
#include "uart.h"

#ifdef _USE_HW_ESP


static uint8_t uart_ch = _DEF_UART3;


#ifdef _USE_HW_CLI
static void cliEsp(cli_args_t *args);
#endif


bool espInit(void)
{

#ifdef _USE_HW_CLI
  cliAdd("esp", cliEsp);
#endif
  return true;
}





void cliEsp(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 2 && args->isStr(0, "at"))
  {
    uint32_t pre_time;

    cliPrintf("=>%s\n", args->getStr(1));
    cliPrintf("<=\n");

    uartPrintf(uart_ch, "%s\r\n", args->getStr(1));

    pre_time = millis();
    while(cliKeepLoop())
    {
      if (uartAvailable(uart_ch) > 0)
      {
        pre_time = millis();

        cliPrintf("%c", uartRead(uart_ch));
      }

      if (millis()-pre_time >= 100)
      {

        break;
      }
    }
    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("esp at [commands]\n");
  }
}

#endif
