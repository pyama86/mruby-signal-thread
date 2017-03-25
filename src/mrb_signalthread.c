/*
** mrb_signalthread.c - SignalThread class
**
** Copyright (c) pyama86 2017
**
** See Copyright Notice in LICENSE
*/
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/proc.h>
#include <mruby/range.h>
#include <mruby/string.h>
#include <mruby/value.h>
#include <mruby/variable.h>

#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

// thanks: https://github.com/ksss/mruby-signal/blob/master/src/signal.c
static const struct signals {
  const char *signm;
  int signo;
} siglist[] = {{"EXIT", 0},
#ifdef SIGHUP
               {"HUP", SIGHUP},
#endif
               {"INT", SIGINT},
#ifdef SIGQUIT
               {"QUIT", SIGQUIT},
#endif
#ifdef SIGILL
               {"ILL", SIGILL},
#endif
#ifdef SIGTRAP
               {"TRAP", SIGTRAP},
#endif
#ifdef SIGABRT
               {"ABRT", SIGABRT},
#endif
#ifdef SIGIOT
               {"IOT", SIGIOT},
#endif
#ifdef SIGEMT
               {"EMT", SIGEMT},
#endif
#ifdef SIGFPE
               {"FPE", SIGFPE},
#endif
#ifdef SIGKILL
               {"KILL", SIGKILL},
#endif
#ifdef SIGBUS
               {"BUS", SIGBUS},
#endif
#ifdef SIGSEGV
               {"SEGV", SIGSEGV},
#endif
#ifdef SIGSYS
               {"SYS", SIGSYS},
#endif
#ifdef SIGPIPE
               {"PIPE", SIGPIPE},
#endif
#ifdef SIGALRM
               {"ALRM", SIGALRM},
#endif
#ifdef SIGTERM
               {"TERM", SIGTERM},
#endif
#ifdef SIGURG
               {"URG", SIGURG},
#endif
#ifdef SIGSTOP
               {"STOP", SIGSTOP},
#endif
#ifdef SIGTSTP
               {"TSTP", SIGTSTP},
#endif
#ifdef SIGCONT
               {"CONT", SIGCONT},
#endif
#ifdef SIGCHLD
               {"CHLD", SIGCHLD},
#endif
#ifdef SIGCLD
               {"CLD", SIGCLD},
#else
#ifdef SIGCHLD
               {"CLD", SIGCHLD},
#endif
#endif
#ifdef SIGTTIN
               {"TTIN", SIGTTIN},
#endif
#ifdef SIGTTOU
               {"TTOU", SIGTTOU},
#endif
#ifdef SIGIO
               {"IO", SIGIO},
#endif
#ifdef SIGXCPU
               {"XCPU", SIGXCPU},
#endif
#ifdef SIGXFSZ
               {"XFSZ", SIGXFSZ},
#endif
#ifdef SIGVTALRM
               {"VTALRM", SIGVTALRM},
#endif
#ifdef SIGPROF
               {"PROF", SIGPROF},
#endif
#ifdef SIGWINCH
               {"WINCH", SIGWINCH},
#endif
#ifdef SIGUSR1
               {"USR1", SIGUSR1},
#endif
#ifdef SIGUSR2
               {"USR2", SIGUSR2},
#endif
#ifdef SIGLOST
               {"LOST", SIGLOST},
#endif
#ifdef SIGMSG
               {"MSG", SIGMSG},
#endif
#ifdef SIGPWR
               {"PWR", SIGPWR},
#endif
#ifdef SIGPOLL
               {"POLL", SIGPOLL},
#endif
#ifdef SIGDANGER
               {"DANGER", SIGDANGER},
#endif
#ifdef SIGMIGRATE
               {"MIGRATE", SIGMIGRATE},
#endif
#ifdef SIGPRE
               {"PRE", SIGPRE},
#endif
#ifdef SIGGRANT
               {"GRANT", SIGGRANT},
#endif
#ifdef SIGRETRACT
               {"RETRACT", SIGRETRACT},
#endif
#ifdef SIGSOUND
               {"SOUND", SIGSOUND},
#endif
#ifdef SIGINFO
               {"INFO", SIGINFO},
#endif
               {NULL, 0}};

static const char *signo2signm(mrb_int no)
{
  const struct signals *sigs;

  for (sigs = siglist; sigs->signm; sigs++) {
    if (sigs->signo == no)
      return sigs->signm;
  }
  return 0;
}

static int signm2signo(const char *nm)
{
  const struct signals *sigs;

  for (sigs = siglist; sigs->signm; sigs++) {
    if (strcmp(sigs->signm, nm) == 0)
      return sigs->signo;
  }

#ifdef SIGRTMIN
  /* Handle RT Signal#0 as special for strtol's err spec */
  if (strcmp("RT0", nm) == 0)
    return SIGRTMIN;

  if (strncmp("RT", nm, 2) == 0) {
    int ret = (int)strtol(nm + 2, NULL, 0);
    if (!ret || (SIGRTMIN + ret > SIGRTMAX))
      return 0;
    return SIGRTMIN + ret;
  }
#endif
  return 0;
}

static int trap_signm(mrb_state *mrb, mrb_value vsig)
{
  int sig = -1;
  const char *s;

  switch (mrb_type(vsig)) {
  case MRB_TT_FIXNUM:
    sig = mrb_fixnum(vsig);
    if (sig < 0 || sig >= NSIG) {
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "invalid signal number (%S)", vsig);
    }
    break;
  case MRB_TT_SYMBOL:
    s = mrb_sym2name(mrb, mrb_symbol(vsig));
    if (!s)
      mrb_raise(mrb, E_ARGUMENT_ERROR, "bad signal");
    goto str_signal;
  default:
    vsig = mrb_string_type(mrb, vsig);
    s = RSTRING_PTR(vsig);

  str_signal:
    if (memcmp("SIG", s, 3) == 0)
      s += 3;
    sig = signm2signo(s);
    if (sig == 0 && strcmp(s, "EXIT") != 0)
      mrb_raise(mrb, E_ARGUMENT_ERROR, "unsupported signal");
    break;
  }
  return sig;
}

static mrb_value mrb_signal_thread_mask(mrb_state *mrb, mrb_value self)
{
  int sig;
  mrb_value *argv;
  mrb_int argc;
  sigset_t set;

  mrb_get_args(mrb, "*", &argv, &argc);
  if (argc != 1)
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments (1 for %S)", mrb_fixnum_value(argc));

  sig = trap_signm(mrb, argv[0]);

  sigemptyset(&set);
  sigaddset(&set, sig);

  if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "set mask error");
  }
  return self;
}

static mrb_value mrb_signal_thread_wait(mrb_state *mrb, mrb_value self)
{
  int sig, s;
  mrb_value *argv;
  mrb_int argc;
  sigset_t set, mask;
  mrb_value command, block;

  mrb_get_args(mrb, "*&", &argv, &argc, &block);
  if (argc != 1)
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments (1 for %S)", mrb_fixnum_value(argc));

  if (!mrb_nil_p(block) && MRB_PROC_CFUNC_P(mrb_proc_ptr(block))) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "require defined block");
  }

  sig = trap_signm(mrb, argv[0]);

  sigfillset(&mask);
  sigdelset(&mask, sig);
  if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "set mask error");
  }

  sigemptyset(&set);
  sigaddset(&set, sig);

  for (;;) {
    sigwait(&set, &sig);
    mrb_yield_argv(mrb, block, 0, NULL);
  }
}

void mrb_mruby_signal_thread_gem_init(mrb_state *mrb)
{
  struct RClass *signalthread;
  signalthread = mrb_define_class(mrb, "SignalThread", mrb->object_class);

  mrb_define_class_method(mrb, signalthread, "mask", mrb_signal_thread_mask, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, signalthread, "wait", mrb_signal_thread_wait, MRB_ARGS_REQ(1));
}

void mrb_mruby_signal_thread_gem_final(mrb_state *mrb)
{
}
