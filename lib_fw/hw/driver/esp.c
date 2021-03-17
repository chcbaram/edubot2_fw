/*
 * esp.c
 *
 *  Created on: 2021. 2. 13.
 *      Author: baram
 */


#include "esp.h"
#include "cli.h"
#include "uart.h"
#include "qbuffer.h"


#ifdef _USE_HW_ESP


#define ESP_BUF_MAX     1024
#define ESP_LINE_MAX    32

static uint8_t uart_ch = _DEF_UART3;
static uint32_t uart_baud = 115200;

typedef struct
{
  bool     is_log;
  bool     is_open;
  bool     is_connected;
  uint8_t  state_update;
  uint16_t line_cnt;
  uint16_t line_pos[ESP_LINE_MAX];
  uint16_t buf_index;
  uint8_t  buf[ESP_BUF_MAX];

  uint8_t  cmd_buf[256];

  uint32_t pre_time;
  qbuffer_t q_rx;
  uint8_t   q_buf[512];
  uint8_t   rx_header[16];
  uint8_t   rx_header_index;
  uint16_t  rx_header_len;
} esp_resp_t;



static esp_resp_t esp_resp;


static bool espClientUpdate(void);


#ifdef _USE_HW_CLI
static void cliEsp(cli_args_t *args);
#endif


bool espInit(void)
{
  esp_resp.line_cnt = 0;
  esp_resp.is_log = false;
  esp_resp.is_open = false;
  esp_resp.is_connected = false;

  qbufferCreate(&esp_resp.q_rx, &esp_resp.q_buf[0], 512);

#ifdef _USE_HW_CLI
  cliAdd("esp", cliEsp);
#endif
  return true;
}

bool espOpen(uint8_t ch, uint32_t baud)
{
  bool ret;

  ret = uartOpen(ch, baud);
  if (ret)
  {
    uart_ch = ch;
    uart_baud = baud;
    esp_resp.is_open = true;
  }

  return ret;
}

bool espIsOpen(void)
{
  return esp_resp.is_open;
}

void espLogEnable(void)
{
  esp_resp.is_log = true;
}

void espLogDisable(void)
{
  esp_resp.is_log = false;
}

uint32_t espAvailable(void)
{
  espClientUpdate();

  return qbufferAvailable(&esp_resp.q_rx);
}

uint32_t espWrite(uint8_t *p_data, uint32_t length)
{
  int len;
  bool ret;

  //len = snprintf((char *)esp_resp.cmd_buf, 256, "AT+CIPSEND=0,%d", (int)length);
  //uartWrite(uart_ch, esp_resp.cmd_buf, len);
  //uartWrite(uart_ch, p_data, length);

  len = snprintf((char *)esp_resp.cmd_buf, 256, "AT+CIPSEND=0,%d", (int)length);
  ret = espCmd((char *)esp_resp.cmd_buf, 10);
  if (ret)
  {
    uartWrite(uart_ch, p_data, length);
    espWaitOK(100);
  }

  return len;
}

uint8_t espRead(void)
{
  uint8_t rx_data;

  qbufferRead(&esp_resp.q_rx, &rx_data, 1);

  return rx_data;
}

uint32_t espPrintf(char *fmt, ...)
{
  char buf[256];
  va_list args;
  int len;
  uint32_t ret;

  va_start(args, fmt);
  len = vsnprintf(buf, 256, fmt, args);

  ret = espWrite((uint8_t *)buf, len);

  va_end(args);

  return ret;
}

bool espCmd(const char *cmd_str, uint32_t timeout)
{
  bool ret = false;
  uint32_t pre_time;
  uint32_t end_ok;
  uint8_t rx_data;


  uartPrintf(uart_ch, "%s\r\n", cmd_str);

  esp_resp.line_cnt = 0;
  esp_resp.buf_index = 0;

  esp_resp.line_pos[0] = 0;

  end_ok = 0;
  pre_time = millis();
  while(1)
  {
    if (uartAvailable(uart_ch) > 0)
    {
      pre_time = millis();

      rx_data = uartRead(uart_ch);

      if (esp_resp.is_log)
      {
        cliPrintf("%c", rx_data);
      }

      esp_resp.buf[esp_resp.buf_index] = rx_data;
      end_ok <<= 8;
      end_ok  |= rx_data;

      if ((end_ok & 0x0000FFFF) == 0x00000D0A)
      {
        if (esp_resp.line_cnt < ESP_LINE_MAX)
        {
          esp_resp.line_cnt++;
          esp_resp.buf_index++;
          esp_resp.buf[esp_resp.buf_index] = 0;

          esp_resp.line_pos[esp_resp.line_cnt] = esp_resp.buf_index+1;
        }
      }

      esp_resp.buf_index++;
      if (esp_resp.buf_index >= ESP_BUF_MAX)
      {
        break;
      }

      if (end_ok == 0x4F4B0D0A)
      {
        ret = true;
        break;
      }
    }

    if (millis()-pre_time >= timeout)
    {
      break;
    }
  }

  return ret;
}

bool espWaitOK(uint32_t timeout)
{
  bool ret = false;
  uint32_t pre_time;
  uint32_t end_ok;
  uint8_t rx_data;


  esp_resp.buf_index = 0;
  end_ok = 0;
  pre_time = millis();
  while(1)
  {
    if (uartAvailable(uart_ch) > 0)
    {
      pre_time = millis();

      rx_data = uartRead(uart_ch);

      if (esp_resp.is_log)
      {
        cliPrintf("%c", rx_data);
      }

      end_ok <<= 8;
      end_ok  |= rx_data;

      esp_resp.buf_index++;
      if (esp_resp.buf_index >= ESP_BUF_MAX)
      {
        break;
      }

      if (end_ok == 0x4F4B0D0A)
      {
        ret = true;
        break;
      }
    }

    if (millis()-pre_time >= timeout)
    {
      break;
    }
  }

  return ret;
}

bool espConnectWifi(char *ssd_str, char *pswd_str, uint32_t timeout)
{
  bool ret;

  snprintf((char *)esp_resp.cmd_buf, 256, "AT+CWJAP=\"%s\",\"%s\"", ssd_str, pswd_str);
  ret = espCmd((char *)esp_resp.cmd_buf, timeout);

  return ret;
}

bool espPing(uint32_t timeout)
{
  bool ret;

  ret = espCmd("AT", timeout);

  return ret;
}

bool espClientBegin(char *ip_str, char *port_str, uint32_t timeout)
{
  bool ret;

  esp_resp.state_update = 0;
  while(1)
  {
    espCmd("AT+CIPCLOSE=0", 100);

    ret = espCmd("AT+CWMODE=3", 100);
    if (ret != true) break;

    ret = espCmd("AT+CIPMUX=1", 100);
    if (ret != true) break;

    ret = espCmd("AT+CIPSERVER=1", 100);
    if (ret != true) break;

    snprintf((char *)esp_resp.cmd_buf, 256, "AT+CIPSTART=0,\"TCP\",\"%s\",%s", ip_str, port_str);
    ret = espCmd((char *)esp_resp.cmd_buf, timeout);
    if (ret != true) break;

    esp_resp.is_connected = true;
    break;
  }

  return ret;
}

bool espClientEnd(void)
{
  bool ret;

  if (esp_resp.is_connected == true)
  {
    ret = espCmd("AT+CIPCLOSE=0", 100);
    esp_resp.is_connected = false;
  }

  return ret;
}


bool espClientUpdate(void)
{
  bool ret = true;
  uint8_t rx_data;


  if (esp_resp.is_connected == false)
  {
    return false;
  }

  if (uartAvailable(uart_ch) > 0)
  {
    rx_data = uartRead(uart_ch);
    if (esp_resp.is_log)
    {
      //cliPrintf("%c", rx_data, rx_data);
    }

    switch(esp_resp.state_update)
    {
      case 0:
        if (rx_data == 0x0D)
        {
          esp_resp.state_update = 1;
        }
        break;

      case 1:
        if (rx_data == 0x0A)
        {
          esp_resp.rx_header_index = 0;
          esp_resp.state_update = 2;
        }
        break;

      case 2:
        if (rx_data == ':')
        {
          esp_resp.rx_header[esp_resp.rx_header_index] = 0;
          uint8_t str_cnt = 0;
          uint8_t str_pos = 0;
          for(int i=0; i<esp_resp.buf_index; i++)
          {
            if (esp_resp.rx_header[i] == ',' )
            {
              str_cnt++;

              if (str_cnt == 2)
              {
                str_pos = i+1;
                break;
              }
            }
          }
          //cliPrintf("cnt : %s\n", &esp_resp.buf[str_pos]);

          esp_resp.rx_header_len = (int16_t)strtoul((const char * )&esp_resp.rx_header[str_pos], (char **)NULL, (int) 0);
          //cliPrintf("cnt : %d\n", esp_resp.rx_header_len);
          if (esp_resp.rx_header_len > 0)
          {
            esp_resp.pre_time = millis();
            esp_resp.state_update = 3;
          }
          else
          {
            esp_resp.state_update = 0;
          }
        }
        else
        {
          esp_resp.rx_header[esp_resp.rx_header_index] = rx_data;
          esp_resp.rx_header_index++;
          if (esp_resp.rx_header_index >= 12)
          {
            esp_resp.state_update = 0;
          }
        }
        break;

      case 3:
        qbufferWrite(&esp_resp.q_rx, &rx_data, 1);

        esp_resp.rx_header_len--;
        if (esp_resp.rx_header_len == 0)
        {
          esp_resp.state_update = 0;
        }

        if (millis()-esp_resp.pre_time >= 10)
        {
          esp_resp.state_update = 0;
        }
        esp_resp.pre_time = millis();
        break;
    }
  }


  return ret;
}


void cliEsp(cli_args_t *args)
{
  bool ret = false;
  uint32_t pre_time;
  uint32_t exe_time;

  if (args->argc == 3 && args->isStr(0, "open"))
  {
    uint8_t ch;
    uint32_t baud;

    ch   = args->getData(1);
    baud = args->getData(2);

    if (ch > 0)
    {
      ch--;
    }
    ch = constrain(ch, 0, UART_MAX_CH);

    if (espOpen(ch, baud))
    {
      cliPrintf("espOpen ch%d %d, OK\n", ch+1, baud);
    }
    else
    {
      cliPrintf("espOpen Fail\n");
    }

    ret = true;
  }

  if (args->argc == 3 && args->isStr(0, "at"))
  {
    uint32_t timeout;

    timeout = args->getData(2);

    pre_time = millis();
    if (espCmd(args->getStr(1), timeout) == true)
    {
      exe_time = millis()-pre_time;
      cliPrintf("exe_time : %d ms\n", exe_time);
      cliPrintf("line_cnt : %d\n", esp_resp.line_cnt);
      for (int i=0; i<esp_resp.line_cnt; i++)
      {
        cliPrintf("%d:%s", i, (char *)&esp_resp.buf[esp_resp.line_pos[i]]);
      }
    }

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "ping"))
  {
    if (espPing(100) == true)
    {
      cliPrintf("espPing OK\n");
    }
    else
    {
      cliPrintf("espPing Fail\n");
    }

    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "wifi") && args->isStr(1, "list"))
  {
    uint32_t timeout;

    timeout = 5000;

    pre_time = millis();
    if (espCmd("AT+CWLAP", timeout) == true)
    {
      exe_time = millis()-pre_time;
      cliPrintf("exe_time : %d ms\n", exe_time);
      cliPrintf("line_cnt : %d\n", esp_resp.line_cnt);
      for (int i=0; i<esp_resp.line_cnt; i++)
      {
        cliPrintf("%d:%s", i, (char *)&esp_resp.buf[esp_resp.line_pos[i]]);
      }
    }
    else
    {
      cliPrintf("espCmd Fail\n");
    }

    ret = true;
  }

  if (args->argc == 4 && args->isStr(0, "wifi") && args->isStr(1, "connect"))
  {
    uint32_t timeout;

    timeout = 10000;

    pre_time = millis();
    if (espConnectWifi(args->getStr(2), args->getStr(3), timeout) == true)
    {
      exe_time = millis()-pre_time;
      cliPrintf("exe_time : %d ms\n", exe_time);
      cliPrintf("line_cnt : %d\n", esp_resp.line_cnt);
      for (int i=0; i<esp_resp.line_cnt; i++)
      {
        cliPrintf("%d:%s", i, (char *)&esp_resp.buf[esp_resp.line_pos[i]]);
      }
    }
    else
    {
      cliPrintf("espConnectWifi Fail\n");
    }


    ret = true;
  }

  if (args->argc == 4 && args->isStr(0, "client") && args->isStr(1, "begin"))
  {
    uint32_t timeout;

    timeout = 5000;

    espLogEnable();

    pre_time = millis();
    if (espClientBegin(args->getStr(2), args->getStr(3), timeout) == true)
    {
      exe_time = millis()-pre_time;
      cliPrintf("exe_time : %d ms\n", exe_time);
      cliPrintf("press q key to exit\n");
      while(espClientUpdate())
      {
        if (cliAvailable() > 0)
        {
          uint8_t rx_data;
          rx_data = cliRead();

          if (rx_data == 'q')
          {
            break;
          }

          espPrintf("Rx : 0x%X\n", rx_data);
        }

        if (espAvailable() > 0)
        {
          uint8_t rx_data;

          rx_data = espRead();

          cliPrintf("rx : 0x%X(%c)\n", rx_data, rx_data);
        }
      }
      cliPrintf("espClient Exit\n");
    }
    else
    {
      cliPrintf("espClientBegin Fail\n");
    }

    espClientEnd();

    espLogDisable();
    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("esp open 1~%d baud\n", UART_MAX_CH+1);
    cliPrintf("esp at [commands] timeout\n");
    cliPrintf("esp ping \n");
    cliPrintf("esp wifi list \n");
    cliPrintf("esp wifi connect ssd password \n");
    cliPrintf("esp client begin ip_addr port\n");
  }
}

#endif
