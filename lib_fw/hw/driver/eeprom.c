/*
 * eeprom.c
 *
 *  Created on: 2021. 2. 12.
 *      Author: baram
 */


#include "eeprom.h"
#include "i2c.h"
#include "cli.h"


#ifdef _USE_HW_EEPROM



#ifdef _USE_HW_CLI
void cliEeprom(cli_args_t *args);
#endif


#define EEPROM_MAX_SIZE   256


static bool is_init = false;
static uint8_t i2c_ch = _DEF_I2C1;
static uint8_t i2c_addr = (0xA0>>1);
static uint8_t last_error = 0;


bool eepromInit()
{
  bool ret;


  ret = i2cBegin(i2c_ch, 400);


  if (eepromValid(0x00) == false)
  {
    return false;
  }

#ifdef _USE_HW_CLI
  cliAdd("eeprom", cliEeprom);
#endif

  is_init = ret;

  return ret;
}

bool eepromIsInit(void)
{
  return is_init;
}

bool eepromValid(uint32_t addr)
{
  uint8_t data;
  bool ret;

  if (addr >= EEPROM_MAX_SIZE)
  {
    return false;
  }

  ret = i2cReadByte(i2c_ch, i2c_addr, addr & 0xFF, &data, 100);

  return ret;
}

uint8_t eepromReadByte(uint32_t addr)
{
  uint8_t data;
  bool ret;

  if (addr >= EEPROM_MAX_SIZE)
  {
    return false;
  }

  ret = i2cReadByte(i2c_ch, i2c_addr, addr & 0xFF, &data, 100);

  last_error = 0;
  if (ret != true) last_error = 1;

  return data;
}

bool eepromWriteByte(uint32_t addr, uint8_t data_in)
{
  uint32_t pre_time;
  bool ret;

  if (addr >= EEPROM_MAX_SIZE)
  {
    return false;
  }

  ret = i2cWriteByte(i2c_ch, i2c_addr, addr & 0xFF, data_in, 10);


  pre_time = millis();
  while(millis()-pre_time < 100)
  {

    ret = i2cReadByte(i2c_ch, i2c_addr, addr, &data_in, 1);
    if (ret == true)
    {
      break;
    }
    delay(1);
  }

  return ret;
}

bool eepromRead(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  bool ret = true;
  uint32_t i;


  for (i=0; i<length; i++)
  {
    p_data[i] = eepromReadByte(addr);

    if (last_error != 0)
    {
      ret = false;
      break;
    }
  }

  return ret;
}

bool eepromWrite(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  bool ret;
  uint32_t i;


  for (i=0; i<length; i++)
  {
    ret = eepromWriteByte(addr, p_data[i]);
    if (ret == false)
    {
      break;
    }
  }

  return ret;
}

uint32_t eepromGetLength(void)
{
  return EEPROM_MAX_SIZE;
}

bool eepromFormat(void)
{
  return true;
}




#ifdef _USE_HW_CLI
void cliEeprom(cli_args_t *args)
{
  bool ret = true;
  uint32_t i;
  uint32_t addr;
  uint32_t length;
  uint8_t  data;
  uint32_t pre_time;
  bool eep_ret;


  if (args->argc == 1)
  {
    if(args->isStr(0, "info") == true)
    {
      cliPrintf("eeprom init   : %d\n", eepromIsInit());
      cliPrintf("eeprom length : %d bytes\n", eepromGetLength());
    }
    else if(args->isStr(0, "format") == true)
    {
      if (eepromFormat() == true)
      {
        cliPrintf("format OK\n");
      }
      else
      {
        cliPrintf("format Fail\n");
      }
    }
    else
    {
      ret = false;
    }
  }
  else if (args->argc == 3)
  {
    if(args->isStr(0, "read") == true)
    {
      addr   = (uint32_t)args->getData(1);
      length = (uint32_t)args->getData(2);

      if (length > eepromGetLength())
      {
        cliPrintf( "length error\n");
      }
      for (i=0; i<length; i++)
      {
        data = eepromReadByte(addr+i);
        cliPrintf( "addr : %d\t 0x%02X\n", addr+i, data);
      }
    }
    else if(args->isStr(0, "write") == true)
    {
      addr = (uint32_t)args->getData(1);
      data = (uint8_t )args->getData(2);

      pre_time = millis();
      eep_ret = eepromWriteByte(addr, data);

      cliPrintf( "addr : %d\t 0x%02X %dms\n", addr, data, millis()-pre_time);
      if (eep_ret)
      {
        cliPrintf("OK\n");
      }
      else
      {
        cliPrintf("FAIL\n");
      }
    }
    else
    {
      ret = false;
    }
  }
  else
  {
    ret = false;
  }


  if (ret == false)
  {
    cliPrintf( "eeprom info\n");
    cliPrintf( "eeprom format\n");
    cliPrintf( "eeprom read  [addr] [length]\n");
    cliPrintf( "eeprom write [addr] [data]\n");
  }

}
#endif /* _USE_HW_CMDIF_EEPROM */


#endif /* _USE_HW_EEPROM */
