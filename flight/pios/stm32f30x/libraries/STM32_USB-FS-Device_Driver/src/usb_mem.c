/**
  ******************************************************************************
  * @file    usb_mem.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    28-August-2012
  * @brief   Utility functions for memory transfers to/from PMA
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usb_lib.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

#if defined(STM32F303xC)                         || \
    defined(STM32F303x8) || defined(STM32F334x8) || \
    defined(STM32F301x8)                         || \
    defined(STM32F373xC) || defined(STM32F378xx) || \
    defined(STM32F302xC)
/*******************************************************************************
* Function Name  : UserToPMABufferCopy
* Description    : Copy a buffer from user memory area to packet memory area (PMA)
* Input          : - pbUsrBuf: pointer to user memory area.
*                  - wPMABufAddr: address into PMA.
*                  - wNBytes: no. of bytes to be copied.
* Output         : None.
* Return         : None	.
*******************************************************************************/
void UserToPMABufferCopy(uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes)
{
  uint32_t n = (wNBytes + 1) >> 1;   /* n = (wNBytes + 1) / 2 */
  uint32_t i, temp1, temp2;
  uint16_t *pdwVal;
  pdwVal = (uint16_t *)(wPMABufAddr * 2 + PMAAddr);
  for (i = n; i != 0; i--)
  {
    temp1 = (uint16_t) * pbUsrBuf;
    pbUsrBuf++;
    temp2 = temp1 | (uint16_t) * pbUsrBuf << 8;
    *pdwVal++ = temp2;
    pdwVal++;
    pbUsrBuf++;
  }
}

/*******************************************************************************
* Function Name  : PMAToUserBufferCopy
* Description    : Copy a buffer from packet memory area (PMA) to user memory area
* Input          : - pbUsrBuf    = pointer to user memory area.
*                  - wPMABufAddr = address into PMA.
*                  - wNBytes     = no. of bytes to be copied.
* Output         : None.
* Return         : None.
*******************************************************************************/
void PMAToUserBufferCopy(uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes)
{
  uint32_t n = (wNBytes + 1) >> 1;/* /2*/
  uint32_t i;
  uint32_t *pdwVal;
  pdwVal = (uint32_t *)(wPMABufAddr * 2 + PMAAddr);
  for (i = n; i != 0; i--)
  {
    *(uint16_t*)pbUsrBuf++ = *pdwVal++;
    pbUsrBuf++;
  }
}
#endif /* STM32F303xC                || */
       /* STM32F303x8 || STM32F334x8 || */
       /* STM32F301x8                || */
       /* STM32F373xC || STM32F378xx    */
#if defined(STM32F302xE) || defined(STM32F303xD) || defined(STM32F303xE)
/*******************************************************************************
* Function Name  : UserToPMABufferCopy
* Description    : Copy a buffer from user memory area to packet memory area (PMA)
* Input          : - pbUsrBuf: pointer to user memory area.
*                  - wPMABufAddr: address into PMA.
*                  - wNBytes: no. of bytes to be copied.
* Output         : None.
* Return         : None	.
*******************************************************************************/
void UserToPMABufferCopy(uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes)
{
  uint32_t n =  ((uint32_t)((uint32_t)wNBytes + 1U)) >> 1U;
  uint32_t i;
  uint16_t temp1, temp2;
  uint16_t *pdwVal;
  pdwVal = (uint16_t *)((uint32_t)(wPMABufAddr + PMAAddr));
  for (i = n; i != 0U; i--)
  {
    temp1 = (uint16_t) * pbUsrBuf;
    pbUsrBuf++;
    temp2 = temp1 | ((uint16_t)((uint16_t)  * pbUsrBuf << 8U)) ;
    *pdwVal++ = temp2;
    pbUsrBuf++;
  }
}

/*******************************************************************************
* Function Name  : PMAToUserBufferCopy
* Description    : Copy a buffer from packet memory area (PMA) to user memory area
* Input          : - pbUsrBuf    = pointer to user memory area.
*                  - wPMABufAddr = address into PMA.
*                  - wNBytes     = no. of bytes to be copied.
* Output         : None.
* Return         : None.
*******************************************************************************/
void PMAToUserBufferCopy(uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes)
{
  uint32_t n =  ((uint32_t)((uint32_t)wNBytes + 1U)) >> 1U;
  uint32_t i;
  uint16_t *pdwVal;
  pdwVal = (uint16_t *)((uint32_t)(wPMABufAddr + PMAAddr));
  for (i = n; i != 0U; i--)
  {
    *(uint16_t*)((uint32_t)pbUsrBuf++) = *pdwVal++;
    pbUsrBuf++;
  }
}
#endif /* STM32F302xE || STM32F303xD || STM32F303xE */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
