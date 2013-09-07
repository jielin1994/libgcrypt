/* ec.c -  Elliptic Curve functions
 * Copyright (C) 2007 Free Software Foundation, Inc.
 * Copyright (C) 2013 g10 Code GmbH
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mpi-internal.h"
#include "longlong.h"
#include "g10lib.h"
#include "context.h"
#include "ec-context.h"


#define point_init(a)  _gcry_mpi_point_init ((a))
#define point_free(a)  _gcry_mpi_point_free_parts ((a))


/* Print a point using the log fucntions.  If CTX is not NULL affine
   coordinates will be printed.  */
void
_gcry_mpi_point_log (const char *name, mpi_point_t point, mpi_ec_t ctx)
{
  gcry_mpi_t x, y;
  char buf[100];

  snprintf (buf, sizeof buf - 1, "%s.X", name);

  if (ctx)
    {
      x = gcry_mpi_new (0);
      y = gcry_mpi_new (0);
    }
  if (!ctx || _gcry_mpi_ec_get_affine (x, y, point, ctx))
    {
      log_mpidump (buf, point->x);
      buf[strlen(buf)-1] = 'Y';
      log_mpidump (buf, point->y);
      buf[strlen(buf)-1] = 'Z';
      log_mpidump (buf, point->z);
    }
  else
    {
      buf[strlen(buf)-1] = 'x';
      log_mpidump (buf, x);
      buf[strlen(buf)-1] = 'y';
      log_mpidump (buf, y);

    }
  if (ctx)
    {
      gcry_mpi_release (x);
      gcry_mpi_release (y);
    }
}


/* Create a new point option.  NBITS gives the size in bits of one
   coordinate; it is only used to pre-allocate some resources and
   might also be passed as 0 to use a default value.  */
mpi_point_t
gcry_mpi_point_new (unsigned int nbits)
{
  mpi_point_t p;

  (void)nbits;  /* Currently not used.  */

  p = gcry_xmalloc (sizeof *p);
  _gcry_mpi_point_init (p);
  return p;
}


/* Release the point object P.  P may be NULL. */
void
gcry_mpi_point_release (mpi_point_t p)
{
  if (p)
    {
      _gcry_mpi_point_free_parts (p);
      gcry_free (p);
    }
}


/* Initialize the fields of a point object.  gcry_mpi_point_free_parts
   may be used to release the fields.  */
void
_gcry_mpi_point_init (mpi_point_t p)
{
  p->x = mpi_new (0);
  p->y = mpi_new (0);
  p->z = mpi_new (0);
}


/* Release the parts of a point object. */
void
_gcry_mpi_point_free_parts (mpi_point_t p)
{
  mpi_free (p->x); p->x = NULL;
  mpi_free (p->y); p->y = NULL;
  mpi_free (p->z); p->z = NULL;
}


/* Set the value from S into D.  */
static void
point_set (mpi_point_t d, mpi_point_t s)
{
  mpi_set (d->x, s->x);
  mpi_set (d->y, s->y);
  mpi_set (d->z, s->z);
}


/* Set the projective coordinates from POINT into X, Y, and Z.  If a
   coordinate is not required, X, Y, or Z may be passed as NULL.  */
void
gcry_mpi_point_get (gcry_mpi_t x, gcry_mpi_t y, gcry_mpi_t z,
                    mpi_point_t point)
{
  if (x)
    mpi_set (x, point->x);
  if (y)
    mpi_set (y, point->y);
  if (z)
    mpi_set (z, point->z);
}


/* Set the projective coordinates from POINT into X, Y, and Z and
   release POINT.  If a coordinate is not required, X, Y, or Z may be
   passed as NULL.  */
void
gcry_mpi_point_snatch_get (gcry_mpi_t x, gcry_mpi_t y, gcry_mpi_t z,
                           mpi_point_t point)
{
  mpi_snatch (x, point->x);
  mpi_snatch (y, point->y);
  mpi_snatch (z, point->z);
  gcry_free (point);
}


/* Set the projective coordinates from X, Y, and Z into POINT.  If a
   coordinate is given as NULL, the value 0 is stored into point.  If
   POINT is given as NULL a new point object is allocated.  Returns
   POINT or the newly allocated point object. */
mpi_point_t
gcry_mpi_point_set (mpi_point_t point,
                    gcry_mpi_t x, gcry_mpi_t y, gcry_mpi_t z)
{
  if (!point)
    point = gcry_mpi_point_new (0);

  if (x)
    mpi_set (point->x, x);
  else
    mpi_clear (point->x);
  if (y)
    mpi_set (point->y, y);
  else
    mpi_clear (point->y);
  if (z)
    mpi_set (point->z, z);
  else
    mpi_clear (point->z);

  return point;
}


/* Set the projective coordinates from X, Y, and Z into POINT.  If a
   coordinate is given as NULL, the value 0 is stored into point.  If
   POINT is given as NULL a new point object is allocated.  The
   coordinates X, Y, and Z are released.  Returns POINT or the newly
   allocated point object. */
mpi_point_t
gcry_mpi_point_snatch_set (mpi_point_t point,
                           gcry_mpi_t x, gcry_mpi_t y, gcry_mpi_t z)
{
  if (!point)
    point = gcry_mpi_point_new (0);

  if (x)
    mpi_snatch (point->x, x);
  else
    mpi_clear (point->x);
  if (y)
    mpi_snatch (point->y, y);
  else
    mpi_clear (point->y);
  if (z)
    mpi_snatch (point->z, z);
  else
    mpi_clear (point->z);

  return point;
}


static void
ec_addm (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v, mpi_ec_t ctx)
{
  mpi_addm (w, u, v, ctx->p);
}

static void
ec_subm (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v, mpi_ec_t ctx)
{
  mpi_subm (w, u, v, ctx->p);
}

static void
ec_mulm (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v, mpi_ec_t ctx)
{
#if 0
  /* NOTE: This code works only for limb sizes of 32 bit.  */
  mpi_limb_t *wp, *sp;

  if (ctx->nist_nbits == 192)
    {
      mpi_mul (w, u, v);
      mpi_resize (w, 12);
      wp = w->d;

      sp = ctx->s[0]->d;
      sp[0*2+0] = wp[0*2+0];
      sp[0*2+1] = wp[0*2+1];
      sp[1*2+0] = wp[1*2+0];
      sp[1*2+1] = wp[1*2+1];
      sp[2*2+0] = wp[2*2+0];
      sp[2*2+1] = wp[2*2+1];

      sp = ctx->s[1]->d;
      sp[0*2+0] = wp[3*2+0];
      sp[0*2+1] = wp[3*2+1];
      sp[1*2+0] = wp[3*2+0];
      sp[1*2+1] = wp[3*2+1];
      sp[2*2+0] = 0;
      sp[2*2+1] = 0;

      sp = ctx->s[2]->d;
      sp[0*2+0] = 0;
      sp[0*2+1] = 0;
      sp[1*2+0] = wp[4*2+0];
      sp[1*2+1] = wp[4*2+1];
      sp[2*2+0] = wp[4*2+0];
      sp[2*2+1] = wp[4*2+1];

      sp = ctx->s[3]->d;
      sp[0*2+0] = wp[5*2+0];
      sp[0*2+1] = wp[5*2+1];
      sp[1*2+0] = wp[5*2+0];
      sp[1*2+1] = wp[5*2+1];
      sp[2*2+0] = wp[5*2+0];
      sp[2*2+1] = wp[5*2+1];

      ctx->s[0]->nlimbs = 6;
      ctx->s[1]->nlimbs = 6;
      ctx->s[2]->nlimbs = 6;
      ctx->s[3]->nlimbs = 6;

      mpi_add (ctx->c, ctx->s[0], ctx->s[1]);
      mpi_add (ctx->c, ctx->c, ctx->s[2]);
      mpi_add (ctx->c, ctx->c, ctx->s[3]);

      while ( mpi_cmp (ctx->c, ctx->p ) >= 0 )
        mpi_sub ( ctx->c, ctx->c, ctx->p );
      mpi_set (w, ctx->c);
    }
  else if (ctx->nist_nbits == 384)
    {
      int i;
      mpi_mul (w, u, v);
      mpi_resize (w, 24);
      wp = w->d;

#define NEXT(a) do { ctx->s[(a)]->nlimbs = 12; \
                     sp = ctx->s[(a)]->d; \
                     i = 0; } while (0)
#define X(a) do { sp[i++] = wp[(a)];} while (0)
#define X0(a) do { sp[i++] = 0; } while (0)
      NEXT(0);
      X(0);X(1);X(2);X(3);X(4);X(5);X(6);X(7);X(8);X(9);X(10);X(11);
      NEXT(1);
      X0();X0();X0();X0();X(21);X(22);X(23);X0();X0();X0();X0();X0();
      NEXT(2);
      X(12);X(13);X(14);X(15);X(16);X(17);X(18);X(19);X(20);X(21);X(22);X(23);
      NEXT(3);
      X(21);X(22);X(23);X(12);X(13);X(14);X(15);X(16);X(17);X(18);X(19);X(20);
      NEXT(4);
      X0();X(23);X0();X(20);X(12);X(13);X(14);X(15);X(16);X(17);X(18);X(19);
      NEXT(5);
      X0();X0();X0();X0();X(20);X(21);X(22);X(23);X0();X0();X0();X0();
      NEXT(6);
      X(20);X0();X0();X(21);X(22);X(23);X0();X0();X0();X0();X0();X0();
      NEXT(7);
      X(23);X(12);X(13);X(14);X(15);X(16);X(17);X(18);X(19);X(20);X(21);X(22);
      NEXT(8);
      X0();X(20);X(21);X(22);X(23);X0();X0();X0();X0();X0();X0();X0();
      NEXT(9);
      X0();X0();X0();X(23);X(23);X0();X0();X0();X0();X0();X0();X0();
#undef X0
#undef X
#undef NEXT
      mpi_add (ctx->c, ctx->s[0], ctx->s[1]);
      mpi_add (ctx->c, ctx->c, ctx->s[1]);
      mpi_add (ctx->c, ctx->c, ctx->s[2]);
      mpi_add (ctx->c, ctx->c, ctx->s[3]);
      mpi_add (ctx->c, ctx->c, ctx->s[4]);
      mpi_add (ctx->c, ctx->c, ctx->s[5]);
      mpi_add (ctx->c, ctx->c, ctx->s[6]);
      mpi_sub (ctx->c, ctx->c, ctx->s[7]);
      mpi_sub (ctx->c, ctx->c, ctx->s[8]);
      mpi_sub (ctx->c, ctx->c, ctx->s[9]);

      while ( mpi_cmp (ctx->c, ctx->p ) >= 0 )
        mpi_sub ( ctx->c, ctx->c, ctx->p );
      while ( ctx->c->sign )
        mpi_add ( ctx->c, ctx->c, ctx->p );
      mpi_set (w, ctx->c);
    }
  else
#endif /*0*/
    mpi_mulm (w, u, v, ctx->p);
}

static void
ec_powm (gcry_mpi_t w, const gcry_mpi_t b, const gcry_mpi_t e,
         mpi_ec_t ctx)
{
  mpi_powm (w, b, e, ctx->p);
  /* _gcry_mpi_abs (w); */
}


/* Shortcut for
     ec_powm (B, B, mpi_const (MPI_C_TWO), ctx);
   for easier optimization.  */
static void
ec_pow2 (gcry_mpi_t w, const gcry_mpi_t b, mpi_ec_t ctx)
{
  /* Using mpi_mul is slightly faster (at least on amd64).  */
  /* mpi_powm (w, b, mpi_const (MPI_C_TWO), ctx->p); */
  mpi_mulm (w, b, b, ctx->p);
}


static void
ec_invm (gcry_mpi_t x, gcry_mpi_t a, mpi_ec_t ctx)
{
  if (!mpi_invm (x, a, ctx->p))
    {
      log_error ("ec_invm: inverse does not exist:\n");
      log_mpidump ("  a", a);
      log_mpidump ("  p", ctx->p);
    }
}


/* Force recomputation of all helper variables.  */
void
_gcry_mpi_ec_get_reset (mpi_ec_t ec)
{
  ec->t.valid.a_is_pminus3 = 0;
  ec->t.valid.two_inv_p = 0;
}


/* Accessor for helper variable.  */
static int
ec_get_a_is_pminus3 (mpi_ec_t ec)
{
  gcry_mpi_t tmp;

  if (!ec->t.valid.a_is_pminus3)
    {
      ec->t.valid.a_is_pminus3 = 1;
      tmp = mpi_alloc_like (ec->p);
      mpi_sub_ui (tmp, ec->p, 3);
      ec->t.a_is_pminus3 = !mpi_cmp (ec->a, tmp);
      mpi_free (tmp);
    }

  return ec->t.a_is_pminus3;
}


/* Accessor for helper variable.  */
static gcry_mpi_t
ec_get_two_inv_p (mpi_ec_t ec)
{
  if (!ec->t.valid.two_inv_p)
    {
      ec->t.valid.two_inv_p = 1;
      if (!ec->t.two_inv_p)
        ec->t.two_inv_p = mpi_alloc (0);
      ec_invm (ec->t.two_inv_p, mpi_const (MPI_C_TWO), ec);
    }
  return ec->t.two_inv_p;
}



/* This function initialized a context for elliptic curve based on the
   field GF(p).  P is the prime specifying this field, A is the first
   coefficient.  CTX is expected to be zeroized.  */
static void
ec_p_init (mpi_ec_t ctx, enum gcry_mpi_ec_models model,
           enum ecc_dialects dialect,
           gcry_mpi_t p, gcry_mpi_t a, gcry_mpi_t b)
{
  int i;

  /* Fixme: Do we want to check some constraints? e.g.  a < p  */

  ctx->model = model;
  ctx->dialect = dialect;
  if (dialect == ECC_DIALECT_ED25519)
    ctx->nbits = 256;
  else
    ctx->nbits = mpi_get_nbits (p);
  ctx->p = mpi_copy (p);
  ctx->a = mpi_copy (a);
  if (b && model == MPI_EC_TWISTEDEDWARDS)
    ctx->b = mpi_copy (b);

  _gcry_mpi_ec_get_reset (ctx);

  /* Allocate scratch variables.  */
  for (i=0; i< DIM(ctx->t.scratch); i++)
    ctx->t.scratch[i] = mpi_alloc_like (ctx->p);

  /* Prepare for fast reduction.  */
  /* FIXME: need a test for NIST values.  However it does not gain us
     any real advantage, for 384 bits it is actually slower than using
     mpi_mulm.  */
/*   ctx->nist_nbits = mpi_get_nbits (ctx->p); */
/*   if (ctx->nist_nbits == 192) */
/*     { */
/*       for (i=0; i < 4; i++) */
/*         ctx->s[i] = mpi_new (192); */
/*       ctx->c    = mpi_new (192*2); */
/*     } */
/*   else if (ctx->nist_nbits == 384) */
/*     { */
/*       for (i=0; i < 10; i++) */
/*         ctx->s[i] = mpi_new (384); */
/*       ctx->c    = mpi_new (384*2); */
/*     } */
}


static void
ec_deinit (void *opaque)
{
  mpi_ec_t ctx = opaque;
  int i;

  /* Domain parameter.  */
  mpi_free (ctx->p);
  mpi_free (ctx->a);
  mpi_free (ctx->b);
  gcry_mpi_point_release (ctx->G);
  mpi_free (ctx->n);

  /* The key.  */
  gcry_mpi_point_release (ctx->Q);
  mpi_free (ctx->d);

  /* Private data of ec.c.  */
  mpi_free (ctx->t.two_inv_p);

  for (i=0; i< DIM(ctx->t.scratch); i++)
    mpi_free (ctx->t.scratch[i]);

/*   if (ctx->nist_nbits == 192) */
/*     { */
/*       for (i=0; i < 4; i++) */
/*         mpi_free (ctx->s[i]); */
/*       mpi_free (ctx->c); */
/*     } */
/*   else if (ctx->nist_nbits == 384) */
/*     { */
/*       for (i=0; i < 10; i++) */
/*         mpi_free (ctx->s[i]); */
/*       mpi_free (ctx->c); */
/*     } */
}


/* This function returns a new context for elliptic curve based on the
   field GF(p).  P is the prime specifying this field, A is the first
   coefficient, B is the second coefficient, and MODEL is the model
   for the curve.  This function is only used within Libgcrypt and not
   part of the public API.

   This context needs to be released using _gcry_mpi_ec_free.  */
mpi_ec_t
_gcry_mpi_ec_p_internal_new (enum gcry_mpi_ec_models model,
                             enum ecc_dialects dialect,
                             gcry_mpi_t p, gcry_mpi_t a, gcry_mpi_t b)
{
  mpi_ec_t ctx;

  ctx = gcry_xcalloc (1, sizeof *ctx);
  ec_p_init (ctx, model, dialect, p, a, b);

  return ctx;
}


/* This is a variant of _gcry_mpi_ec_p_internal_new which returns an
   public contect and does some error checking on the supplied
   arguments.  On success the new context is stored at R_CTX and 0 is
   returned; on error NULL is stored at R_CTX and an error code is
   returned.

   The context needs to be released using gcry_ctx_release.  */
gpg_err_code_t
_gcry_mpi_ec_p_new (gcry_ctx_t *r_ctx,
                    enum gcry_mpi_ec_models model,
                    enum ecc_dialects dialect,
                    gcry_mpi_t p, gcry_mpi_t a, gcry_mpi_t b)
{
  gcry_ctx_t ctx;
  mpi_ec_t ec;

  *r_ctx = NULL;
  if (!p || !a || !mpi_cmp_ui (a, 0))
    return GPG_ERR_EINVAL;

  ctx = _gcry_ctx_alloc (CONTEXT_TYPE_EC, sizeof *ec, ec_deinit);
  if (!ctx)
    return gpg_err_code_from_syserror ();
  ec = _gcry_ctx_get_pointer (ctx, CONTEXT_TYPE_EC);
  ec_p_init (ec, model, dialect, p, a, b);

  *r_ctx = ctx;
  return 0;
}


void
_gcry_mpi_ec_free (mpi_ec_t ctx)
{
  if (ctx)
    {
      ec_deinit (ctx);
      gcry_free (ctx);
    }
}


gcry_mpi_t
_gcry_mpi_ec_get_mpi (const char *name, gcry_ctx_t ctx, int copy)
{
  mpi_ec_t ec = _gcry_ctx_get_pointer (ctx, CONTEXT_TYPE_EC);

  return _gcry_ecc_get_mpi (name, ec, copy);
}


gcry_mpi_point_t
_gcry_mpi_ec_get_point (const char *name, gcry_ctx_t ctx, int copy)
{
  mpi_ec_t ec = _gcry_ctx_get_pointer (ctx, CONTEXT_TYPE_EC);

  (void)copy;  /* Not used.  */

  return _gcry_ecc_get_point (name, ec);
}


gpg_err_code_t
_gcry_mpi_ec_set_mpi (const char *name, gcry_mpi_t newvalue,
                      gcry_ctx_t ctx)
{
  mpi_ec_t ec = _gcry_ctx_get_pointer (ctx, CONTEXT_TYPE_EC);

  return _gcry_ecc_set_mpi (name, newvalue, ec);
}


gpg_err_code_t
_gcry_mpi_ec_set_point (const char *name, gcry_mpi_point_t newvalue,
                        gcry_ctx_t ctx)
{
  mpi_ec_t ec = _gcry_ctx_get_pointer (ctx, CONTEXT_TYPE_EC);

  return _gcry_ecc_set_point (name, newvalue, ec);
}


/* Compute the affine coordinates from the projective coordinates in
   POINT.  Set them into X and Y.  If one coordinate is not required,
   X or Y may be passed as NULL.  CTX is the usual context. Returns: 0
   on success or !0 if POINT is at infinity.  */
int
_gcry_mpi_ec_get_affine (gcry_mpi_t x, gcry_mpi_t y, mpi_point_t point,
                         mpi_ec_t ctx)
{
  if (!mpi_cmp_ui (point->z, 0))
    return -1;

  switch (ctx->model)
    {
    case MPI_EC_WEIERSTRASS: /* Using Jacobian coordinates.  */
      {
        gcry_mpi_t z1, z2, z3;

        z1 = mpi_new (0);
        z2 = mpi_new (0);
        ec_invm (z1, point->z, ctx);  /* z1 = z^(-1) mod p  */
        ec_mulm (z2, z1, z1, ctx);    /* z2 = z^(-2) mod p  */

        if (x)
          ec_mulm (x, point->x, z2, ctx);

        if (y)
          {
            z3 = mpi_new (0);
            ec_mulm (z3, z2, z1, ctx);      /* z3 = z^(-3) mod p  */
            ec_mulm (y, point->y, z3, ctx);
            mpi_free (z3);
          }

        mpi_free (z2);
        mpi_free (z1);
      }
      return 0;

    case MPI_EC_MONTGOMERY:
      {
        log_fatal ("%s: %s not yet supported\n",
                   "_gcry_mpi_ec_get_affine", "Montgomery");
      }
      return -1;

    case MPI_EC_TWISTEDEDWARDS:
      {
        gcry_mpi_t z;

        z = mpi_new (0);
        ec_invm (z, point->z, ctx);

        if (x)
          ec_mulm (x, point->x, z, ctx);
        if (y)
          ec_mulm (y, point->y, z, ctx);

        gcry_mpi_release (z);
      }
      return 0;

    default:
      return -1;
    }
}



/*  RESULT = 2 * POINT  (Weierstrass version). */
static void
dup_point_weierstrass (mpi_point_t result, mpi_point_t point, mpi_ec_t ctx)
{
#define x3 (result->x)
#define y3 (result->y)
#define z3 (result->z)
#define t1 (ctx->t.scratch[0])
#define t2 (ctx->t.scratch[1])
#define t3 (ctx->t.scratch[2])
#define l1 (ctx->t.scratch[3])
#define l2 (ctx->t.scratch[4])
#define l3 (ctx->t.scratch[5])

  if (!mpi_cmp_ui (point->y, 0) || !mpi_cmp_ui (point->z, 0))
    {
      /* P_y == 0 || P_z == 0 => [1:1:0] */
      mpi_set_ui (x3, 1);
      mpi_set_ui (y3, 1);
      mpi_set_ui (z3, 0);
    }
  else
    {
      if (ec_get_a_is_pminus3 (ctx))  /* Use the faster case.  */
        {
          /* L1 = 3(X - Z^2)(X + Z^2) */
          /*                          T1: used for Z^2. */
          /*                          T2: used for the right term.  */
          ec_pow2 (t1, point->z, ctx);
          ec_subm (l1, point->x, t1, ctx);
          ec_mulm (l1, l1, mpi_const (MPI_C_THREE), ctx);
          ec_addm (t2, point->x, t1, ctx);
          ec_mulm (l1, l1, t2, ctx);
        }
      else /* Standard case. */
        {
          /* L1 = 3X^2 + aZ^4 */
          /*                          T1: used for aZ^4. */
          ec_pow2 (l1, point->x, ctx);
          ec_mulm (l1, l1, mpi_const (MPI_C_THREE), ctx);
          ec_powm (t1, point->z, mpi_const (MPI_C_FOUR), ctx);
          ec_mulm (t1, t1, ctx->a, ctx);
          ec_addm (l1, l1, t1, ctx);
        }
      /* Z3 = 2YZ */
      ec_mulm (z3, point->y, point->z, ctx);
      ec_mulm (z3, z3, mpi_const (MPI_C_TWO), ctx);

      /* L2 = 4XY^2 */
      /*                              T2: used for Y2; required later. */
      ec_pow2 (t2, point->y, ctx);
      ec_mulm (l2, t2, point->x, ctx);
      ec_mulm (l2, l2, mpi_const (MPI_C_FOUR), ctx);

      /* X3 = L1^2 - 2L2 */
      /*                              T1: used for L2^2. */
      ec_pow2 (x3, l1, ctx);
      ec_mulm (t1, l2, mpi_const (MPI_C_TWO), ctx);
      ec_subm (x3, x3, t1, ctx);

      /* L3 = 8Y^4 */
      /*                              T2: taken from above. */
      ec_pow2 (t2, t2, ctx);
      ec_mulm (l3, t2, mpi_const (MPI_C_EIGHT), ctx);

      /* Y3 = L1(L2 - X3) - L3 */
      ec_subm (y3, l2, x3, ctx);
      ec_mulm (y3, y3, l1, ctx);
      ec_subm (y3, y3, l3, ctx);
    }

#undef x3
#undef y3
#undef z3
#undef t1
#undef t2
#undef t3
#undef l1
#undef l2
#undef l3
}


/*  RESULT = 2 * POINT  (Montgomery version). */
static void
dup_point_montgomery (mpi_point_t result, mpi_point_t point, mpi_ec_t ctx)
{
  (void)result;
  (void)point;
  (void)ctx;
  log_fatal ("%s: %s not yet supported\n",
             "_gcry_mpi_ec_dup_point", "Montgomery");
}


/*  RESULT = 2 * POINT  (Twisted Edwards version). */
static void
dup_point_twistededwards (mpi_point_t result, mpi_point_t point, mpi_ec_t ctx)
{
#define X1 (point->x)
#define Y1 (point->y)
#define Z1 (point->z)
#define X3 (result->x)
#define Y3 (result->y)
#define Z3 (result->z)
#define B (ctx->t.scratch[0])
#define C (ctx->t.scratch[1])
#define D (ctx->t.scratch[2])
#define E (ctx->t.scratch[3])
#define F (ctx->t.scratch[4])
#define H (ctx->t.scratch[5])
#define J (ctx->t.scratch[6])

  /* Compute: (X_3 : Y_3 : Z_3) = 2( X_1 : Y_1 : Z_1 ) */

  /* B = (X_1 + Y_1)^2  */
  ec_addm (B, X1, Y1, ctx);
  ec_pow2 (B, B, ctx);

  /* C = X_1^2 */
  /* D = Y_1^2 */
  ec_pow2 (C, X1, ctx);
  ec_pow2 (D, Y1, ctx);

  /* E = aC */
  ec_mulm (E, ctx->a, C, ctx);

  /* F = E + D */
  ec_addm (F, E, D, ctx);

  /* H = Z_1^2 */
  ec_pow2 (H, Z1, ctx);

  /* J = F - 2H */
  ec_mulm (J, H, mpi_const (MPI_C_TWO), ctx);
  ec_subm (J, F, J, ctx);

  /* X_3 = (B - C - D) · J */
  ec_subm (X3, B, C, ctx);
  ec_subm (X3, X3, D, ctx);
  ec_mulm (X3, X3, J, ctx);

  /* Y_3 = F · (E - D) */
  ec_subm (Y3, E, D, ctx);
  ec_mulm (Y3, Y3, F, ctx);

  /* Z_3 = F · J */
  ec_mulm (Z3, F, J, ctx);

#undef X1
#undef Y1
#undef Z1
#undef X3
#undef Y3
#undef Z3
#undef B
#undef C
#undef D
#undef E
#undef F
#undef H
#undef J
}


/*  RESULT = 2 * POINT  */
void
_gcry_mpi_ec_dup_point (mpi_point_t result, mpi_point_t point, mpi_ec_t ctx)
{
  switch (ctx->model)
    {
    case MPI_EC_WEIERSTRASS:
      dup_point_weierstrass (result, point, ctx);
      break;
    case MPI_EC_MONTGOMERY:
      dup_point_montgomery (result, point, ctx);
      break;
    case MPI_EC_TWISTEDEDWARDS:
      dup_point_twistededwards (result, point, ctx);
      break;
    }
}


/* RESULT = P1 + P2  (Weierstrass version).*/
static void
add_points_weierstrass (mpi_point_t result,
                        mpi_point_t p1, mpi_point_t p2,
                        mpi_ec_t ctx)
{
#define x1 (p1->x    )
#define y1 (p1->y    )
#define z1 (p1->z    )
#define x2 (p2->x    )
#define y2 (p2->y    )
#define z2 (p2->z    )
#define x3 (result->x)
#define y3 (result->y)
#define z3 (result->z)
#define l1 (ctx->t.scratch[0])
#define l2 (ctx->t.scratch[1])
#define l3 (ctx->t.scratch[2])
#define l4 (ctx->t.scratch[3])
#define l5 (ctx->t.scratch[4])
#define l6 (ctx->t.scratch[5])
#define l7 (ctx->t.scratch[6])
#define l8 (ctx->t.scratch[7])
#define l9 (ctx->t.scratch[8])
#define t1 (ctx->t.scratch[9])
#define t2 (ctx->t.scratch[10])

  if ( (!mpi_cmp (x1, x2)) && (!mpi_cmp (y1, y2)) && (!mpi_cmp (z1, z2)) )
    {
      /* Same point; need to call the duplicate function.  */
      _gcry_mpi_ec_dup_point (result, p1, ctx);
    }
  else if (!mpi_cmp_ui (z1, 0))
    {
      /* P1 is at infinity.  */
      mpi_set (x3, p2->x);
      mpi_set (y3, p2->y);
      mpi_set (z3, p2->z);
    }
  else if (!mpi_cmp_ui (z2, 0))
    {
      /* P2 is at infinity.  */
      mpi_set (x3, p1->x);
      mpi_set (y3, p1->y);
      mpi_set (z3, p1->z);
    }
  else
    {
      int z1_is_one = !mpi_cmp_ui (z1, 1);
      int z2_is_one = !mpi_cmp_ui (z2, 1);

      /* l1 = x1 z2^2  */
      /* l2 = x2 z1^2  */
      if (z2_is_one)
        mpi_set (l1, x1);
      else
        {
          ec_pow2 (l1, z2, ctx);
          ec_mulm (l1, l1, x1, ctx);
        }
      if (z1_is_one)
        mpi_set (l2, x2);
      else
        {
          ec_pow2 (l2, z1, ctx);
          ec_mulm (l2, l2, x2, ctx);
        }
      /* l3 = l1 - l2 */
      ec_subm (l3, l1, l2, ctx);
      /* l4 = y1 z2^3  */
      ec_powm (l4, z2, mpi_const (MPI_C_THREE), ctx);
      ec_mulm (l4, l4, y1, ctx);
      /* l5 = y2 z1^3  */
      ec_powm (l5, z1, mpi_const (MPI_C_THREE), ctx);
      ec_mulm (l5, l5, y2, ctx);
      /* l6 = l4 - l5  */
      ec_subm (l6, l4, l5, ctx);

      if (!mpi_cmp_ui (l3, 0))
        {
          if (!mpi_cmp_ui (l6, 0))
            {
              /* P1 and P2 are the same - use duplicate function.  */
              _gcry_mpi_ec_dup_point (result, p1, ctx);
            }
          else
            {
              /* P1 is the inverse of P2.  */
              mpi_set_ui (x3, 1);
              mpi_set_ui (y3, 1);
              mpi_set_ui (z3, 0);
            }
        }
      else
        {
          /* l7 = l1 + l2  */
          ec_addm (l7, l1, l2, ctx);
          /* l8 = l4 + l5  */
          ec_addm (l8, l4, l5, ctx);
          /* z3 = z1 z2 l3  */
          ec_mulm (z3, z1, z2, ctx);
          ec_mulm (z3, z3, l3, ctx);
          /* x3 = l6^2 - l7 l3^2  */
          ec_pow2 (t1, l6, ctx);
          ec_pow2 (t2, l3, ctx);
          ec_mulm (t2, t2, l7, ctx);
          ec_subm (x3, t1, t2, ctx);
          /* l9 = l7 l3^2 - 2 x3  */
          ec_mulm (t1, x3, mpi_const (MPI_C_TWO), ctx);
          ec_subm (l9, t2, t1, ctx);
          /* y3 = (l9 l6 - l8 l3^3)/2  */
          ec_mulm (l9, l9, l6, ctx);
          ec_powm (t1, l3, mpi_const (MPI_C_THREE), ctx); /* fixme: Use saved value*/
          ec_mulm (t1, t1, l8, ctx);
          ec_subm (y3, l9, t1, ctx);
          ec_mulm (y3, y3, ec_get_two_inv_p (ctx), ctx);
        }
    }

#undef x1
#undef y1
#undef z1
#undef x2
#undef y2
#undef z2
#undef x3
#undef y3
#undef z3
#undef l1
#undef l2
#undef l3
#undef l4
#undef l5
#undef l6
#undef l7
#undef l8
#undef l9
#undef t1
#undef t2
}


/* RESULT = P1 + P2  (Montgomery version).*/
static void
add_points_montgomery (mpi_point_t result,
                       mpi_point_t p1, mpi_point_t p2,
                       mpi_ec_t ctx)
{
  (void)result;
  (void)p1;
  (void)p2;
  (void)ctx;
  log_fatal ("%s: %s not yet supported\n",
             "_gcry_mpi_ec_add_points", "Montgomery");
}


/* RESULT = P1 + P2  (Twisted Edwards version).*/
static void
add_points_twistededwards (mpi_point_t result,
                           mpi_point_t p1, mpi_point_t p2,
                           mpi_ec_t ctx)
{
#define X1 (p1->x)
#define Y1 (p1->y)
#define Z1 (p1->z)
#define X2 (p2->x)
#define Y2 (p2->y)
#define Z2 (p2->z)
#define X3 (result->x)
#define Y3 (result->y)
#define Z3 (result->z)
#define A (ctx->t.scratch[0])
#define B (ctx->t.scratch[1])
#define C (ctx->t.scratch[2])
#define D (ctx->t.scratch[3])
#define E (ctx->t.scratch[4])
#define F (ctx->t.scratch[5])
#define G (ctx->t.scratch[6])
#define tmp (ctx->t.scratch[7])

  /* Compute: (X_3 : Y_3 : Z_3) = (X_1 : Y_1 : Z_1) + (X_2 : Y_2 : Z_3)  */

  /* A = Z1 · Z2 */
  ec_mulm (A, Z1, Z2, ctx);

  /* B = A^2 */
  ec_pow2 (B, A, ctx);

  /* C = X1 · X2 */
  ec_mulm (C, X1, X2, ctx);

  /* D = Y1 · Y2 */
  ec_mulm (D, Y1, Y2, ctx);

  /* E = d · C · D */
  ec_mulm (E, ctx->b, C, ctx);
  ec_mulm (E, E, D, ctx);

  /* F = B - E */
  ec_subm (F, B, E, ctx);

  /* G = B + E */
  ec_addm (G, B, E, ctx);

  /* X_3 = A · F · ((X_1 + Y_1) · (X_2 + Y_2) - C - D) */
  ec_addm (tmp, X1, Y1, ctx);
  ec_addm (X3, X2, Y2, ctx);
  ec_mulm (X3, X3, tmp, ctx);
  ec_subm (X3, X3, C, ctx);
  ec_subm (X3, X3, D, ctx);
  ec_mulm (X3, X3, F, ctx);
  ec_mulm (X3, X3, A, ctx);

  /* Y_3 = A · G · (D - aC) */
  ec_mulm (Y3, ctx->a, C, ctx);
  ec_subm (Y3, D, Y3, ctx);
  ec_mulm (Y3, Y3, G, ctx);
  ec_mulm (Y3, Y3, A, ctx);

  /* Z_3 = F · G */
  ec_mulm (Z3, F, G, ctx);


#undef X1
#undef Y1
#undef Z1
#undef X2
#undef Y2
#undef Z2
#undef X3
#undef Y3
#undef Z3
#undef A
#undef B
#undef C
#undef D
#undef E
#undef F
#undef G
#undef tmp
}


/* RESULT = P1 + P2 */
void
_gcry_mpi_ec_add_points (mpi_point_t result,
                         mpi_point_t p1, mpi_point_t p2,
                         mpi_ec_t ctx)
{
  switch (ctx->model)
    {
    case MPI_EC_WEIERSTRASS:
      add_points_weierstrass (result, p1, p2, ctx);
      break;
    case MPI_EC_MONTGOMERY:
      add_points_montgomery (result, p1, p2, ctx);
      break;
    case MPI_EC_TWISTEDEDWARDS:
      add_points_twistededwards (result, p1, p2, ctx);
      break;
    }
}


/* Scalar point multiplication - the main function for ECC.  If takes
   an integer SCALAR and a POINT as well as the usual context CTX.
   RESULT will be set to the resulting point. */
void
_gcry_mpi_ec_mul_point (mpi_point_t result,
                        gcry_mpi_t scalar, mpi_point_t point,
                        mpi_ec_t ctx)
{
  gcry_mpi_t x1, y1, z1, k, h, yy;
  unsigned int i, loops;
  mpi_point_struct p1, p2, p1inv;

  if (ctx->model == MPI_EC_TWISTEDEDWARDS)
    {
      /* Simple left to right binary method.  GECC Algorithm 3.27 */
      unsigned int nbits;
      int j;

      nbits = mpi_get_nbits (scalar);
      mpi_set_ui (result->x, 0);
      mpi_set_ui (result->y, 1);
      mpi_set_ui (result->z, 1);

      for (j=nbits-1; j >= 0; j--)
        {
          _gcry_mpi_ec_dup_point (result, result, ctx);
          if (mpi_test_bit (scalar, j) == 1)
            _gcry_mpi_ec_add_points (result, result, point, ctx);
        }
      return;
    }

  x1 = mpi_alloc_like (ctx->p);
  y1 = mpi_alloc_like (ctx->p);
  h  = mpi_alloc_like (ctx->p);
  k  = mpi_copy (scalar);
  yy = mpi_copy (point->y);

  if ( mpi_has_sign (k) )
    {
      k->sign = 0;
      ec_invm (yy, yy, ctx);
    }

  if (!mpi_cmp_ui (point->z, 1))
    {
      mpi_set (x1, point->x);
      mpi_set (y1, yy);
    }
  else
    {
      gcry_mpi_t z2, z3;

      z2 = mpi_alloc_like (ctx->p);
      z3 = mpi_alloc_like (ctx->p);
      ec_mulm (z2, point->z, point->z, ctx);
      ec_mulm (z3, point->z, z2, ctx);
      ec_invm (z2, z2, ctx);
      ec_mulm (x1, point->x, z2, ctx);
      ec_invm (z3, z3, ctx);
      ec_mulm (y1, yy, z3, ctx);
      mpi_free (z2);
      mpi_free (z3);
    }
  z1 = mpi_copy (mpi_const (MPI_C_ONE));

  mpi_mul (h, k, mpi_const (MPI_C_THREE)); /* h = 3k */
  loops = mpi_get_nbits (h);
  if (loops < 2)
    {
      /* If SCALAR is zero, the above mpi_mul sets H to zero and thus
         LOOPs will be zero.  To avoid an underflow of I in the main
         loop we set LOOP to 2 and the result to (0,0,0).  */
      loops = 2;
      mpi_clear (result->x);
      mpi_clear (result->y);
      mpi_clear (result->z);
    }
  else
    {
      mpi_set (result->x, point->x);
      mpi_set (result->y, yy);
      mpi_set (result->z, point->z);
    }
  mpi_free (yy); yy = NULL;

  p1.x = x1; x1 = NULL;
  p1.y = y1; y1 = NULL;
  p1.z = z1; z1 = NULL;
  point_init (&p2);
  point_init (&p1inv);

  for (i=loops-2; i > 0; i--)
    {
      _gcry_mpi_ec_dup_point (result, result, ctx);
      if (mpi_test_bit (h, i) == 1 && mpi_test_bit (k, i) == 0)
        {
          point_set (&p2, result);
          _gcry_mpi_ec_add_points (result, &p2, &p1, ctx);
        }
      if (mpi_test_bit (h, i) == 0 && mpi_test_bit (k, i) == 1)
        {
          point_set (&p2, result);
          /* Invert point: y = p - y mod p  */
          point_set (&p1inv, &p1);
          ec_subm (p1inv.y, ctx->p, p1inv.y, ctx);
          _gcry_mpi_ec_add_points (result, &p2, &p1inv, ctx);
        }
    }

  point_free (&p1);
  point_free (&p2);
  point_free (&p1inv);
  mpi_free (h);
  mpi_free (k);
}


/* Return true if POINT is on the curve described by CTX.  */
int
_gcry_mpi_ec_curve_point (gcry_mpi_point_t point, mpi_ec_t ctx)
{
  int res = 0;
  gcry_mpi_t x, y, w;

  x = mpi_new (0);
  y = mpi_new (0);
  w = mpi_new (0);

  if (_gcry_mpi_ec_get_affine (x, y, point, ctx))
    return 0;

  switch (ctx->model)
    {
    case MPI_EC_WEIERSTRASS:
      log_fatal ("%s: %s not yet supported\n",
                 "_gcry_mpi_ec_curve_point", "Weierstrass");
      break;
    case MPI_EC_MONTGOMERY:
      log_fatal ("%s: %s not yet supported\n",
                 "_gcry_mpi_ec_curve_point", "Montgomery");
      break;
    case MPI_EC_TWISTEDEDWARDS:
      {
        /* a · x^2 + y^2 - 1 - b · x^2 · y^2 == 0 */
        ec_pow2 (x, x, ctx);
        ec_pow2 (y, y, ctx);
        ec_mulm (w, ctx->a, x, ctx);
        ec_addm (w, w, y, ctx);
        ec_subm (w, w, mpi_const (MPI_C_ONE), ctx);
        ec_mulm (x, x, y, ctx);
        ec_mulm (x, x, ctx->b, ctx);
        ec_subm (w, w, x, ctx);
        if (!mpi_cmp_ui (w, 0))
          res = 1;
      }
      break;
    }

  gcry_mpi_release (w);
  gcry_mpi_release (x);
  gcry_mpi_release (y);

  return res;
}
