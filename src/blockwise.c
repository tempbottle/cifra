/*
 * cifra - embedded cryptography library
 * Written in 2014 by Joseph Birr-Pixton <jpixton@gmail.com>
 *
 * To the extent possible under law, the author(s) have dedicated all
 * copyright and related and neighboring rights to this software to the
 * public domain worldwide. This software is distributed without any
 * warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication
 * along with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#include "blockwise.h"
#include "bitops.h"
#include "handy.h"
#include "tassert.h"

#include <string.h>

void cf_blockwise_accumulate(uint8_t *partial, size_t *npartial, size_t nblock,
                             const void *inp, size_t nbytes,
                             cf_blockwise_in_fn process,
                             void *ctx)
{
  cf_blockwise_accumulate_final(partial, npartial, nblock,
                                inp, nbytes,
                                process, process, ctx);
}

void cf_blockwise_accumulate_final(uint8_t *partial, size_t *npartial, size_t nblock,
                                   const void *inp, size_t nbytes,
                                   cf_blockwise_in_fn process,
                                   cf_blockwise_in_fn process_final,
                                   void *ctx)
{
  const uint8_t *bufin = inp;
  assert(partial && *npartial < nblock);
  assert(inp || !nbytes);
  assert(process && ctx);

  /* If we have partial data, copy in to buffer. */
  if (*npartial && nbytes)
  {
    size_t space = nblock - *npartial;
    size_t taken = MIN(space, nbytes);

    memcpy(partial + *npartial, bufin, taken);

    bufin += taken;
    nbytes -= taken;
    *npartial += taken;

    /* If that gives us a full block, process it. */
    if (*npartial == nblock)
    {
      if (nbytes == 0)
        process_final(ctx, partial);
      else
        process(ctx, partial);
      *npartial = 0;
    }
  }

  /* now nbytes < nblock or *npartial == 0. */

  /* If we have a full block of data, process it directly. */
  while (nbytes >= nblock)
  {
    /* Partial buffer must be empty, or we're ignoring extant data */
    assert(*npartial == 0);

    if (nbytes == nblock)
      process_final(ctx, bufin);
    else
      process(ctx, bufin);
    bufin += nblock;
    nbytes -= nblock;
  }

  /* Finally, if we have remaining data, buffer it. */
  while (nbytes)
  {
    size_t space = nblock - *npartial;
    size_t taken = MIN(space, nbytes);

    memcpy(partial + *npartial, bufin, taken);

    bufin += taken;
    nbytes -= taken;
    *npartial += taken;

    /* If we started with *npartial, we must have copied it
     * in first. */
    assert(*npartial < nblock);
  }
}

void cf_blockwise_xor(uint8_t *partial, size_t *npartial, size_t nblock,
                      const void *inp, void *outp, size_t nbytes,
                      cf_blockwise_out_fn process, void *ctx)
{
  const uint8_t *inb = inp;
  uint8_t *outb = outp;

  assert(partial && *npartial < nblock);
  assert(inp || !nbytes);
  assert(process && ctx);

  while (nbytes)
  {
    /* If we're out of material, and need more, produce a block. */
    if (*npartial == 0)
    {
      process(ctx, partial);
      *npartial = nblock;
    }

    size_t offset = nblock - *npartial;
    size_t taken = MIN(*npartial, nbytes);
    xor_bb(outb, inb, partial + offset, taken);
    *npartial -= taken;
    nbytes -= taken;
    outb += taken;
    inb += taken;
  }
}
