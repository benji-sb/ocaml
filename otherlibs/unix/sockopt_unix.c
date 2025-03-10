/**************************************************************************/
/*                                                                        */
/*                                 OCaml                                  */
/*                                                                        */
/*             Xavier Leroy, projet Cristal, INRIA Rocquencourt           */
/*                                                                        */
/*   Copyright 1996 Institut National de Recherche en Informatique et     */
/*     en Automatique.                                                    */
/*                                                                        */
/*   All rights reserved.  This file is distributed under the terms of    */
/*   the GNU Lesser General Public License version 2.1, with the          */
/*   special exception on linking described in the file LICENSE.          */
/*                                                                        */
/**************************************************************************/

#include <caml/mlvalues.h>
#include <caml/memory.h>
#include <caml/alloc.h>
#include <caml/fail.h>
#include "caml/unixsupport.h"

#ifdef HAS_SOCKETS

#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "caml/socketaddr.h"

#ifndef SO_DEBUG
#define SO_DEBUG (-1)
#endif
#ifndef SO_BROADCAST
#define SO_BROADCAST (-1)
#endif
#ifndef SO_REUSEADDR
#define SO_REUSEADDR (-1)
#endif
#ifndef SO_REUSEPORT
#define SO_REUSEPORT (-1)
#endif
#ifndef SO_KEEPALIVE
#define SO_KEEPALIVE (-1)
#endif
#ifndef SO_DONTROUTE
#define SO_DONTROUTE (-1)
#endif
#ifndef SO_OOBINLINE
#define SO_OOBINLINE (-1)
#endif
#ifndef SO_ACCEPTCONN
#define SO_ACCEPTCONN (-1)
#endif
#ifndef SO_SNDBUF
#define SO_SNDBUF (-1)
#endif
#ifndef SO_RCVBUF
#define SO_RCVBUF (-1)
#endif
#ifndef SO_ERROR
#define SO_ERROR (-1)
#endif
#ifndef SO_TYPE
#define SO_TYPE (-1)
#endif
#ifndef SO_RCVLOWAT
#define SO_RCVLOWAT (-1)
#endif
#ifndef SO_SNDLOWAT
#define SO_SNDLOWAT (-1)
#endif
#ifndef SO_LINGER
#define SO_LINGER (-1)
#endif
#ifndef SO_RCVTIMEO
#define SO_RCVTIMEO (-1)
#endif
#ifndef SO_SNDTIMEO
#define SO_SNDTIMEO (-1)
#endif
#ifndef TCP_NODELAY
#define TCP_NODELAY (-1)
#endif
#ifndef SO_ERROR
#define SO_ERROR (-1)
#endif
#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6 (-1)
#endif
#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY (-1)
#endif

enum option_type {
  TYPE_BOOL = 0,
  TYPE_INT = 1,
  TYPE_LINGER = 2,
  TYPE_TIMEVAL = 3,
  TYPE_UNIX_ERROR = 4
};

struct socket_option {
  int level;
  int option;
};

/* Table of options, indexed by type */

static const struct socket_option sockopt_bool[] = {
  { SOL_SOCKET, SO_DEBUG },
  { SOL_SOCKET, SO_BROADCAST },
  { SOL_SOCKET, SO_REUSEADDR },
  { SOL_SOCKET, SO_KEEPALIVE },
  { SOL_SOCKET, SO_DONTROUTE },
  { SOL_SOCKET, SO_OOBINLINE },
  { SOL_SOCKET, SO_ACCEPTCONN },
  { IPPROTO_TCP, TCP_NODELAY },
  { IPPROTO_IPV6, IPV6_V6ONLY},
  { SOL_SOCKET, SO_REUSEPORT }
};

static const struct socket_option sockopt_int[] = {
  { SOL_SOCKET, SO_SNDBUF },
  { SOL_SOCKET, SO_RCVBUF },
  { SOL_SOCKET, SO_ERROR },
  { SOL_SOCKET, SO_TYPE },
  { SOL_SOCKET, SO_RCVLOWAT },
  { SOL_SOCKET, SO_SNDLOWAT } };

static const struct socket_option sockopt_linger[] = {
  { SOL_SOCKET, SO_LINGER }
};

static const struct socket_option sockopt_timeval[] = {
  { SOL_SOCKET, SO_RCVTIMEO },
  { SOL_SOCKET, SO_SNDTIMEO }
};

static const struct socket_option sockopt_unix_error[] = {
  { SOL_SOCKET, SO_ERROR }
};

static const struct socket_option * sockopt_table[] = {
  sockopt_bool,
  sockopt_int,
  sockopt_linger,
  sockopt_timeval,
  sockopt_unix_error
};

static const char * getsockopt_fun_name[] = {
  "getsockopt",
  "getsockopt_int",
  "getsockopt_optint",
  "getsockopt_float",
  "getsockopt_error"
};

static const char * setsockopt_fun_name[] = {
  "setsockopt",
  "setsockopt_int",
  "setsockopt_optint",
  "setsockopt_float",
  "setsockopt_error"
};

union option_value {
  int i;
  struct linger lg;
  struct timeval tv;
};

CAMLexport value caml_unix_getsockopt_aux(const char * name,
                                     enum option_type ty, int level, int option,
                                     value socket)
{
  CAMLparam0();
  CAMLlocal1(err);
  value res;
  union option_value optval;
  socklen_param_type optsize;


  switch (ty) {
  case TYPE_BOOL:
  case TYPE_INT:
  case TYPE_UNIX_ERROR:
    optsize = sizeof(optval.i); break;
  case TYPE_LINGER:
    optsize = sizeof(optval.lg); break;
  case TYPE_TIMEVAL:
    optsize = sizeof(optval.tv); break;
  default:
    caml_unix_error(EINVAL, name, Nothing);
  }

  if (getsockopt(Int_val(socket), level, option,
                 (void *) &optval, &optsize) == -1)
    caml_uerror(name, Nothing);

  switch (ty) {
  case TYPE_BOOL:
    res = Val_bool(optval.i); break;
  case TYPE_INT:
    res = Val_int(optval.i); break;
  case TYPE_LINGER:
    if (optval.lg.l_onoff == 0) {
      res = Val_none;
    } else {
      res = caml_alloc_some(Val_int(optval.lg.l_linger));
    }
    break;
  case TYPE_TIMEVAL:
    res = caml_copy_double((double) optval.tv.tv_sec
                           + (double) optval.tv.tv_usec / 1e6);
    break;
  case TYPE_UNIX_ERROR:
    if (optval.i == 0) {
      res = Val_none;
    } else {
      err = caml_unix_error_of_code(optval.i);
      res = caml_alloc_some(err);
    }
    break;
  default:
    caml_unix_error(EINVAL, name, Nothing);
  }
  CAMLreturn(res);
}

CAMLexport value caml_unix_setsockopt_aux(const char * name,
                                     enum option_type ty, int level, int option,
                                     value socket, value val)
{
  union option_value optval;
  socklen_param_type optsize;
  double f;

  switch (ty) {
  case TYPE_BOOL:
  case TYPE_INT:
    optsize = sizeof(optval.i);
    optval.i = Int_val(val);
    break;
  case TYPE_LINGER:
    optsize = sizeof(optval.lg);
    optval.lg.l_onoff = Is_some(val);
    if (optval.lg.l_onoff)
      optval.lg.l_linger = Int_val(Some_val(val));
    break;
  case TYPE_TIMEVAL:
    f = Double_val(val);
    optsize = sizeof(optval.tv);
    optval.tv.tv_sec = (int) f;
    optval.tv.tv_usec = (int) (1e6 * (f - optval.tv.tv_sec));
    break;
  case TYPE_UNIX_ERROR:
  default:
    caml_unix_error(EINVAL, name, Nothing);
  }

  if (setsockopt(Int_val(socket), level, option,
                 (void *) &optval, optsize) == -1)
    caml_uerror(name, Nothing);

  return Val_unit;
}

CAMLprim value caml_unix_getsockopt(value vty, value vsocket, value voption)
{
  enum option_type ty = Int_val(vty);
  const struct socket_option * opt = &(sockopt_table[ty][Int_val(voption)]);
  return caml_unix_getsockopt_aux(getsockopt_fun_name[ty],
                             ty,
                             opt->level,
                             opt->option,
                             vsocket);
}

CAMLprim value caml_unix_setsockopt(value vty, value vsocket, value voption,
                               value val)
{
  enum option_type ty = Int_val(vty);
  const struct socket_option * opt = &(sockopt_table[ty][Int_val(voption)]);
  return caml_unix_setsockopt_aux(setsockopt_fun_name[ty],
                             ty,
                             opt->level,
                             opt->option,
                             vsocket,
                             val);
}

#else

CAMLprim value caml_unix_getsockopt(value vty, value socket, value option)
{ caml_invalid_argument("getsockopt not implemented"); }

CAMLprim value caml_unix_setsockopt(value vty, value socket, value option,
                                    value val)
{ caml_invalid_argument("setsockopt not implemented"); }

#endif
