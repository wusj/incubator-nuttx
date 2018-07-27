/****************************************************************************
 * arch/arm/src/samd5e5/sam_clockconfig.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "up_arch.h"

#include "chip/sam_pm.h"
#include "chip/sam_supc.h"
#include "chip/sam_oscctrl.h"
#include "chip/sam_osc32kctrl.h"
#include "chip/sam_gclk.h"
#include "chip/sam_nvmctrl.h"
#include "sam_gclk.h"

#include "sam_periphclks.h"

#include <arch/board/board.h>  /* Normally must be included last */
#include "sam_clockconfig.h"   /* Needs to be included after board.h */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* This is the currently configured CPU frequency */

static uint32_t g_cpu_frequency;

/* This describes the power-up clock configuration using data provided in the
 * board.h header file.
 */

static const struct sam_clockconfig_s g_initial_clocking =
{
  .waitstates        = BOARD_FLASH_WAITSTATES,
  .cpudiv            = BOARD_MCLK_CPUDIV,
  .glckset1          = BOARD_GCLK_SET1,
  .glckset2          = BOARD_GCLK_SET2,
  .cpu_frequency     = BOARD_CPU_FREQUENCY,
#if BOARD_HAVE_XOSC32K != 0
   .xosc32k          =
   {
     .enable         = BOARD_XOSC32K_ENABLE,
     .highspeed      = BOARD_XOSC32K_HIGHSPEED,
     .extalen        = BOARD_XOSC32K_XTALEN,
     .en32k          = BOARD_XOSC32K_EN32K,
     .en1k           = BOARD_XOSC32K_EN1K,
     .runstdby       = BOARD_XOSC32K_RUNSTDBY,
     .ondemand       = BOARD_XOSC32K_ONDEMAND,
     .cfden          = BOARD_XOSC32K_CFDEN,
     .cfdeo          = BOARD_XOSC32K_CFDEO,
     .caliben        = BOARD_XOSC32K_CALIBEN,
     .startup        = BOARD_XOSC32K_STARTUP,
     .calib          = BOARD_XOSC32K_CALIB,
     .rtcsel         = BOARD_XOSC32K_RTCSEL,
   },
#endif
#if BOARD_HAVE_XOSC0 != 0
   .xosc0            =
   {
     .enable         = BOARD_XOSC0_ENABLE,
     .extalen        = BOARD_XOSC0_XTALEN,
     .runstdby       = BOARD_XOSC0_RUNSTDBY,
     .ondemand       = BOARD_XOSC0_ONDEMAND,
     .lowgain        = BOARD_XOSC0_LOWGAIN,
     .enalc          = BOARD_XOSC0_ENALC,
     .cfden          = BOARD_XOSC0_CFDEN,
     .startup        = BOARD_XOSC0_STARTUP,
     .xosc_frequency = BOARD_XOSC0_FREQUENCY,
   },
#endif
#if BOARD_HAVE_XOSC1 != 0
   .xosc1            =
   {
     .enable         = BOARD_XOSC1_ENABLE,
     .extalen        = BOARD_XOSC1_XTALEN,
     .runstdby       = BOARD_XOSC1_RUNSTDBY,
     .ondemand       = BOARD_XOSC1_ONDEMAND,
     .lowgain        = BOARD_XOSC1_LOWGAIN,
     .enalc          = BOARD_XOSC1_ENALC,
     .cfden          = BOARD_XOSC1_CFDEN,
     .startup        = BOARD_XOSC1_STARTUP,
     .xosc_frequency = BOARD_XOSC1_FREQUENCY,
   },
#endif
   .gclk             =
   {
     {
     .enable        = BOARD_GCLK0_ENABLE,
     .idc           = BOARD_GCLK0_IDC,
     .oov           = BOARD_GCLK0_OOV,
     .oe            = BOARD_GCLK0_OE,
     .divsel        = BOARD_GCLK0_DIVSEL,
     .runstdby      = BOARD_GCLK0_RUNSTDBY,
     .source        = BOARD_GCLK0_SOURCE,
     .div           = BOARD_GCLK0_DIV
     },
     {
     .enable        = BOARD_GCLK1_ENABLE,
     .idc           = BOARD_GCLK1_IDC,
     .oov           = BOARD_GCLK1_OOV,
     .oe            = BOARD_GCLK1_OE,
     .divsel        = BOARD_GCLK1_DIVSEL,
     .runstdby      = BOARD_GCLK1_RUNSTDBY,
     .source        = BOARD_GCLK1_SOURCE,
     .div           = BOARD_GCLK1_DIV
     },
     {
     .enable        = BOARD_GCLK2_ENABLE,
     .idc           = BOARD_GCLK2_IDC,
     .oov           = BOARD_GCLK2_OOV,
     .oe            = BOARD_GCLK2_OE,
     .divsel        = BOARD_GCLK2_DIVSEL,
     .runstdby      = BOARD_GCLK2_RUNSTDBY,
     .source        = BOARD_GCLK2_SOURCE,
     .div           = BOARD_GCLK2_DIV
     },
     {
     .enable        = BOARD_GCLK3_ENABLE,
     .idc           = BOARD_GCLK3_IDC,
     .oov           = BOARD_GCLK3_OOV,
     .oe            = BOARD_GCLK3_OE,
     .divsel        = BOARD_GCLK3_DIVSEL,
     .runstdby      = BOARD_GCLK3_RUNSTDBY,
     .source        = BOARD_GCLK3_SOURCE,
     .div           = BOARD_GCLK3_DIV
     },
     {
     .enable        = BOARD_GCLK4_ENABLE,
     .idc           = BOARD_GCLK4_IDC,
     .oov           = BOARD_GCLK4_OOV,
     .oe            = BOARD_GCLK4_OE,
     .divsel        = BOARD_GCLK4_DIVSEL,
     .runstdby      = BOARD_GCLK4_RUNSTDBY,
     .source        = BOARD_GCLK4_SOURCE,
     .div           = BOARD_GCLK4_DIV
     },
     {
     .enable        = BOARD_GCLK5_ENABLE,
     .idc           = BOARD_GCLK5_IDC,
     .oov           = BOARD_GCLK5_OOV,
     .oe            = BOARD_GCLK5_OE,
     .divsel        = BOARD_GCLK5_DIVSEL,
     .runstdby      = BOARD_GCLK5_RUNSTDBY,
     .source        = BOARD_GCLK5_SOURCE,
     .div           = BOARD_GCLK5_DIV
     },
     {
     .enable        = BOARD_GCLK6_ENABLE,
     .idc           = BOARD_GCLK6_IDC,
     .oov           = BOARD_GCLK6_OOV,
     .oe            = BOARD_GCLK6_OE,
     .divsel        = BOARD_GCLK6_DIVSEL,
     .runstdby      = BOARD_GCLK6_RUNSTDBY,
     .source        = BOARD_GCLK6_SOURCE,
     .div           = BOARD_GCLK6_DIV
     },
     {
     .enable        = BOARD_GCLK7_ENABLE,
     .idc           = BOARD_GCLK7_IDC,
     .oov           = BOARD_GCLK7_OOV,
     .oe            = BOARD_GCLK7_OE,
     .divsel        = BOARD_GCLK7_DIVSEL,
     .runstdby      = BOARD_GCLK7_RUNSTDBY,
     .source        = BOARD_GCLK7_SOURCE,
     .div           = BOARD_GCLK7_DIV
     },
     {
     .enable        = BOARD_GCLK8_ENABLE,
     .idc           = BOARD_GCLK8_IDC,
     .oov           = BOARD_GCLK8_OOV,
     .oe            = BOARD_GCLK8_OE,
     .divsel        = BOARD_GCLK8_DIVSEL,
     .runstdby      = BOARD_GCLK8_RUNSTDBY,
     .source        = BOARD_GCLK8_SOURCE,
     .div           = BOARD_GCLK8_DIV
     },
     {
     .enable        = BOARD_GCLK9_ENABLE,
     .idc           = BOARD_GCLK9_IDC,
     .oov           = BOARD_GCLK9_OOV,
     .oe            = BOARD_GCLK9_OE,
     .divsel        = BOARD_GCLK9_DIVSEL,
     .runstdby      = BOARD_GCLK9_RUNSTDBY,
     .source        = BOARD_GCLK9_SOURCE,
     .div           = BOARD_GCLK9_DIV
     },
     {
     .enable        = BOARD_GCLK10_ENABLE,
     .idc           = BOARD_GCLK10_IDC,
     .oov           = BOARD_GCLK10_OOV,
     .oe            = BOARD_GCLK10_OE,
     .divsel        = BOARD_GCLK10_DIVSEL,
     .runstdby      = BOARD_GCLK10_RUNSTDBY,
     .source        = BOARD_GCLK10_SOURCE,
     .div           = BOARD_GCLK10_DIV
     },
     {
     .enable        = BOARD_GCLK11_ENABLE,
     .idc           = BOARD_GCLK11_IDC,
     .oov           = BOARD_GCLK11_OOV,
     .oe            = BOARD_GCLK11_OE,
     .divsel        = BOARD_GCLK11_DIVSEL,
     .runstdby      = BOARD_GCLK11_RUNSTDBY,
     .source        = BOARD_GCLK11_SOURCE,
     .div           = BOARD_GCLK11_DIV
     }
   }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sam_set_waitsates
 *
 * Description:
 *   Set the number of FLASH wait states.
 *
 ****************************************************************************/

static void sam_set_waitsates(const struct sam_clockconfig_s *config)
{
  DEBUGASSERT(config->waitstates < 16);
  modifyreg16(SAM_NVMCTRL_CTRLA, NVMCTRL_CTRLA_RWS_MASK,
              NVMCTRL_CTRLA_RWS(config->waitstates) );
}

/****************************************************************************
 * Name: sam_xosc32k_configure
 *
 * Description:
 *   Configure XOSC32K
 *
 ****************************************************************************/

#if BOARD_HAVE_XOSC32K != 0
static void sam_xosc32k_configure(const struct sam_xosc32_config_s *config)
{
  uint32_t regval32;
  uint32_t regval16;
  uint32_t regval8;

  /* Configure the OSC32KCTRL register */

  regval16 = OSC32KCTRL_XOSC32K_STARTUP(config->startup);

  if (config->enable)
    {
      regval16 |= OSC32KCTRL_XOSC32K_ENABLE;
    }

  if (config->highspeed)
    {
      regval16 |= OSC32KCTRL_XOSC32K_GCM_HS;
    }

  if (config->extalen)
    {
      regval16 |= OSC32KCTRL_XOSC32K_XTALEN;
    }

  if (config->en32k)
    {
      regval16 |= OSC32KCTRL_XOSC32K_EN32K;
    }

  if (config->en1k)
    {
      regval16 |= OSC32KCTRL_XOSC32K_EN1K;
    }

  if (config->runstdby)
    {
      regval16 |= OSC32KCTRL_XOSC32K_RUNSTDBY;
    }

  if (config->ondemand)
    {
      regval16 |= OSC32KCTRL_XOSC32K_ONDEMAND;
    }

  putreg16(regval16, SAM_OSC32KCTRL_XOSC32K);

  /* Configure the CFDCTRL register */

  regval8 = config->cfden ? OSC32KCTRL_CFDCTRL_CFDEN : 0;
  putreg8(regval8, SAM_OSC32KCTRL_CFDCTRL);

  regval8 = config->cfdeo ? OSC32KCTRL_EVCTRL_CFDEO : 0;
  putreg8(regval8, SAM_OSC32KCTRL_EVCTRL);

  /* Setup OSCULP32K calibration */

  if (config->caliben)
    {
      regval32  = getreg32(SAM_OSC32KCTRL_OSCULP32K);
      regval32 &= ~OSC32KCTRL_OSCULP32K_CALIB_MASK;
      regval32 |= OSC32KCTRL_OSCULP32K_CALIB(config->calib);
    }

  /* Wait for XOSC32 to become ready if it was enabled */

  if (config->enable && ! config->ondemand)
    {
      while ((getreg32(SAM_OSC32KCTRL_STATUS) &
              OSC32KCTRL_INT_XOSC32KRDY) == 0)
        {
        }
    }

  /* Set the RTC clock source */

  putreg8(OSC32KCTRL_RTCCTRL_RTCSEL(config->rtcsel),
          SAM_OSC32KCTRL_RTCCTRL);
}
#endif

/****************************************************************************
 * Name: sam_xoscctrl
 *
 * Description:
 *   Get the appropriate settings for the XOSCCTRL register for XOSC0 or 1.
 *
 ****************************************************************************/

#if BOARD_HAVE_XOSC0 != 0 || BOARD_HAVE_XOSC1 != 0
static uint32_t sam_xoscctrl(const struct sam_xosc_config_s *config)
{
  uint32_t regval;
  uint8_t cfdpresc;
  uint8_t imult;
  uint8_t iptat;

  /* Some settings determined by the crystal frequency */

  if (config->xosc_frequency > 32000000)
    {
      cfdpresc = 0;
      imult    = 7;
      iptat    = 3;
    }
  else if (config->xosc_frequency > 24000000)
    {
      cfdpresc = 1;
      imult    = 6;
      iptat    = 3;
    }
  else if (config->xosc_frequency > 16000000)
    {
      cfdpresc = 2;
      imult    = 5;
      iptat    = 3;
    }
  else if (config->xosc_frequency > 8000000)
    {
      cfdpresc = 3;
      imult    = 4;
      iptat    = 3;
    }

  /* Get the XOSCTCTL register *configuration */

  regval = OSCCTRL_XOSCCTRL_IPTAT(ipta) | OSCCTRL_XOSCCTRL_IMULT(imult) |
           OSCCTRL_XOSCCTRL_STARTUP(config->starup) |
           OSCCTRL_XOSCCTRL_CFDPRESC(cfdpresc);

  if (config->enable)
    {
      regval |= OSCCTRL_XOSCCTRL_ENABLE;
    }

  if (config->extalen)
    {
      regval |= OSCCTRL_XOSCCTRL_XTALEN;
    }

  if (config->runstdby)
    {
      regval |= OSCCTRL_XOSCCTRL_RUNSTDBY;
    }

  if (config->ondemand)
    {
      regval |= OSCCTRL_XOSCCTRL_ONDEMAND;
    }

  if (config->lowgain)
    {
      regval |= OSCCTRL_XOSCCTRL_LOWBUFGAIN;
    }

  if (config->enalc)
    {
      regval |= OSCCTRL_XOSCCTRL_ENALC;
    }

  if (config->cfden)
    {
      regval |= OSCCTRL_XOSCCTRL_CFDEN;
    }

  if (config->swben)
    {
      regval |= OSCCTRL_XOSCCTRL_SWBEN;
    }

  return regval;
}
#endif

/****************************************************************************
 * Name: sam_xosc32k_configure
 *
 * Description:
 *   Configure XOSC32K
 *
 ****************************************************************************/

#if BOARD_HAVE_XOSC0 != 0
static void sam_xosc0_configure(const struct sam_clockconfig_s *config)
{
  uint32_t regval;

  /* Configure the XOSCTCTL register */

  regval = sam_xoscctrl(config);
  putreg32(regval, SAM_OSCCTRL_XOSCCTRL0);

  /* If the XOSC was enabled, then wait for it to become ready */

   /* Wait for XOSC32 to become ready if it was enabled */

   if (config->enable)
     {
       while (getreg32(SAM_OSCCTRL_STATUS) & OSCCTRL_INT_XOSCRDY0) == 0)
         {
         }
     }

  /* Re-select OnDemand */

  if (config->ondemand)
    {
      regval  = getre32(SAM_OSCCTRL_XOSCCTRL0)
      regval |=OSCCTRL_XOSCCTRL_ONDEMAND;
      putreg32(regval, SAM_OSCCTRL_XOSCCTRL0);
    }
}
#endif

#if BOARD_HAVE_XOSC0 != 0
void sam_xosc1_configure(const struct sam_xosc_config_s *config)
{
  uint32_t regval;

  /* Configure the XOSCTCTL register */

  regval = sam_xoscctrl(config);
  putreg32(regval, SAM_OSCCTRL_XOSCCTRL1);

  /* If the XOSC was enabled, then wait for it to become ready */

   /* Wait for XOSC32 to become ready if it was enabled */

   if (config->enable)
     {
       while (getreg32(SAM_OSCCTRL_STATUS) & OSCCTRL_INT_XOSCRDY1) == 0)
         {
         }
     }

  /* Re-select OnDemand */

  if (config->ondemand)
    {
      regval  = getre32(SAM_OSCCTRL_XOSCCTRL1)
      regval |=OSCCTRL_XOSCCTRL_ONDEMAND;
      putreg32(regval, SAM_OSCCTRL_XOSCCTRL1);
    }
}
#endif

/****************************************************************************
 * Name: sam_mclk_configure
 *
 * Description:
 *   Configure master clock generator
 *
 ****************************************************************************/

static inline void sam_mclk_configure(uint8_t cpudiv)
{
  putreg8(cpudiv, SAM_MCLK_CPUDIV);
}

/****************************************************************************
 * Name: sam_fdpll_configure
 *
 * Description:
 *   Configure FDPLL0 and FDPLL1
 *
 ****************************************************************************/

static void sam_fdpll_configure(const struct sam_fdpll_config_s config[2])
{
#warning Missing logic
}

/****************************************************************************
 * Name: sam_gclk_configure
 *
 * Description:
 *   Configure one GLCK
 *
 ****************************************************************************/

static void sam_gclk_configure(uintptr_t regaddr,
                               const struct sam_gclk_config_s *config)
{
  uint32_t regval;

  /* Are we enabling or disabling the GLCK? */

  if (config->enable)
    {
      /* Get the GLCK configuration */

      regval = GCLK_GENCTRL_SRC(config->source) | GCLK_GENCTRL_GENEN |
               GCLK_GENCTRL1_DIV(config->div);


      if (config->idc)
        {
          regval |= GCLK_GENCTRL_IDC;
        }

      if (config->oov)
        {
          regval |= GCLK_GENCTRL_OOV;
        }

      if (config->oe)
        {
          regval |= GCLK_GENCTRL_OE;
        }

      if (config->divsel)
        {
          regval |= GCLK_GENCTRL_DIVSEL;
        }

      if (config->runstdby)
        {
          regval |= GCLK_GENCTRL_RUNSTDBY;
        }
    }
  else
    {
      /* Disable the GLCK */

      regval = 0;
    }

  /* Write the GCLK configuration */

  putreg32(regval, regaddr);
}

/****************************************************************************
 * Name: sam_gclkset_configure
 *
 * Description:
 *   Configure a set of GLCKs
 *
 ****************************************************************************/

static void sam_gclkset_configure(uint16_t gclkset,
                                  const struct sam_gclk_config_s *config)
{
  uint16_t mask;
  int gclk;

  /* Try every GCLK */

  for (gclk = 0; gclk < SAM_NGLCK && gclkset != 0; gclk++)
    {
      /* Check if this one is in the set */

      mask = 1 << gclk;
      if ((gclkset & mask) != 0)
        {
          /* Yes.. Remove it from the set and configure it */

          gclkset &= ~mask;
          sam_gclk_configure(SAM_GCLK_GENCTRL(gclk), &config[gclk]);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sam_clock_configure
 *
 * Description:
 *   Configure the clock sub-system per the provided configuration data.
 *
 *   This should be called only (1) early in the initialization sequence, or
 *   (2) later but within a critical section.
 *
 ****************************************************************************/

void sam_clock_configure(const struct sam_clockconfig_s *config)
{
  /* Check if the clock frequency is increasing or decreasing */

  if (config->cpu_frequency > g_cpu_frequency)
    {
      /* Increasing.  The number of waits states should be larger.  Set the
       * new number of wait states before configuring the clocking.
       */

      sam_set_waitsates(config);
    }

#if BOARD_HAVE_XOSC32K != 0
  /* Configure XOSC32 */

  sam_xosc32k_configure(&config->xosc32k);
#endif

#if BOARD_HAVE_XOSC0 != 0
  /* Configure XOSC0 */

  sam_xosc0_configure(&config->xosc0);
#endif

#if BOARD_HAVE_XOSC1 != 0
  /* Configure XOSC1 */

  sam_xosc1_configure(&config->xosc1);
#endif

  /* Configure master clock generator */

  sam_mclk_configure(config->cpudiv);

  /* Pre-configure some GCLKs before configuring the FDPLLs */

  sam_gclkset_configure(config->glckset1, config->gclk);

  /* Configure the FDPLLs */

  sam_fdpll_configure(config->fdpll);

  /* Configure the renaming GCLKs before configuring the FDPLLs */

  sam_gclkset_configure(config->glckset2, config->gclk);

  /* Check if the clock frequency is increasing or decreasing */

  if (config->cpu_frequency < g_cpu_frequency)
    {
      /* Decreasing.  The number of waits states should be smaller.  Set the
       * new number of wait states after configuring the clocking.
       */

      sam_set_waitsates(config);
    }
}

/****************************************************************************
 * Name: sam_clock_initialize
 *
 * Description:
 *   Configure the initial power-up clocking.  This function may be called
 *   only once by the power-up reset logic.
 *
 ****************************************************************************/

void sam_clock_initialize(void)
{
  /* Clear .bss used by this file in case it is called before .bss is
   * initialized.
   */

  g_cpu_frequency = 0;
  sam_clock_configure(&g_initial_clocking);
}
