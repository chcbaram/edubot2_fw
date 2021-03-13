/*
 * ap.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "ap.h"



const uint8_t usb_uart_ch = _DEF_UART1;
const uint8_t dxl_uart_ch = _DEF_UART2;


uint8_t  mode;
uint8_t  buf[128];
uint16_t buf_len;
uint32_t baud;



void cliEdubot(cli_args_t *args);
void u2d2Mode(void);


void apInit(void)
{
  cliOpen(_DEF_UART1, 57600);   // USB
  cliAdd("edubot", cliEdubot);

  uartOpen(_DEF_UART2, 57600);  // XL-330
  uartOpen(_DEF_UART3, 115200); // ESP8266
}

void apMain(void)
{
  uint32_t pre_time;
  uint32_t led_time;


  baud = uartGetBaud(_DEF_UART1);
  led_time = 500;
  mode = 0;

  pre_time = millis();
  while(1)
  {
    if (millis()-pre_time >= led_time)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
    }

    if (buttonGetPressed(_DEF_BUTTON1))
    {
      delay(200);
      while(buttonGetPressed(_DEF_BUTTON1));

      mode = (mode + 1)%2;
    }

    if (mode == 0)
    {
      led_time = 500;
      cliMain();
    }
    else
    {
      led_time = 50;
      u2d2Mode();
    }
  }
}

void u2d2Mode(void)
{
  if (baud != uartGetBaud(usb_uart_ch))
  {
    baud = uartGetBaud(usb_uart_ch);
    uartOpen(dxl_uart_ch, baud);
  }

  // USB -> XL-330
  buf_len = uartAvailable(usb_uart_ch);
  if (buf_len > 0)
  {
    if (buf_len > 128)
    {
      buf_len = 128;
    }
    for (int i=0; i<buf_len; i++)
    {
      buf[i] = uartRead(usb_uart_ch);
    }
    uartWrite(dxl_uart_ch, &buf[0], buf_len);
  }

  // XL-330 -> USB
  buf_len = uartAvailable(dxl_uart_ch);
  if (buf_len > 0)
  {
    if (buf_len > 128)
    {
      buf_len = 128;
    }
    for (int i=0; i<buf_len; i++)
    {
      buf[i] = uartRead(dxl_uart_ch);
    }
    uartWrite(usb_uart_ch, &buf[0], buf_len);
  }
}



void cliEdubot(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("edubot info\n");
  }
}
