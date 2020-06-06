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

#define DONE mrb_gc_arena_restore(mrb, 0);

typedef struct {
  int argc;
  mrb_value *argv;
  struct RProc *proc;
  pthread_t thread;
  mrb_state *mrb_caller;
  mrb_state *mrb;
  mrb_value result;
  mrb_bool alive;
} mrb_thread_context;

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

#ifdef SIGRTMAX
#define MRB_SIGNAL_LIMIT_NO SIGRTMAX + 1
#else
#define MRB_SIGNAL_LIMIT_NO NSIG
#endif

static int trap_signm(mrb_state *mrb, mrb_value vsig)
{
  int sig = -1;
  const char *s;

  switch (mrb_type(vsig)) {
  case MRB_TT_FIXNUM:
    sig = mrb_fixnum(vsig);
    if (sig < 0 || sig >= MRB_SIGNAL_LIMIT_NO)
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "invalid signal number (%S)", vsig);
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

  if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0)
    mrb_raise(mrb, E_RUNTIME_ERROR, "set mask error");

  return self;
}

static mrb_value mrb_signal_thread_wait(mrb_state *mrb, mrb_value self)
{
  int sig;
  mrb_value *argv;
  mrb_int argc;
  sigset_t set, mask;
  mrb_value block = mrb_nil_value();

  mrb_get_args(mrb, "*&", &argv, &argc, &block);
  if (argc != 1)
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments (1 for %S)", mrb_fixnum_value(argc));

  if (!mrb_nil_p(block) && MRB_PROC_CFUNC_P(mrb_proc_ptr(block)))
    mrb_raise(mrb, E_RUNTIME_ERROR, "require defined block");

  sig = trap_signm(mrb, argv[0]);

  sigfillset(&mask);
  sigdelset(&mask, sig);
  if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0)
    mrb_raise(mrb, E_RUNTIME_ERROR, "set mask error");

  sigemptyset(&set);
  sigaddset(&set, sig);

  if (mrb_nil_p(block)) {
    /* just wait if no block given */
    sigwait(&set, &sig);
    return mrb_nil_value();
  } else {
    for (;;) {
      sigwait(&set, &sig);
      mrb_yield_argv(mrb, block, 0, NULL);
    }
    /* never return */
    mrb_raise(mrb, E_RUNTIME_ERROR, "[BUG] Wait loop seems broken");
  }
}

/* SigInfo functions */

#define MRB_DEFINE_SIGINFO_MEMBER(name, mrb_convert_fun, member)                                                       \
  static mrb_value mrb_siginfo_get_##name(mrb_state *mrb, mrb_value self)                                              \
  {                                                                                                                    \
    siginfo_t *si = DATA_PTR(self);                                                                                    \
    if (!si) {                                                                                                         \
      mrb_raise(mrb, E_RUNTIME_ERROR, "[BUG]");                                                                        \
    }                                                                                                                  \
    return mrb_convert_fun(si->member);                                                                                \
  }

MRB_DEFINE_SIGINFO_MEMBER(pid, mrb_fixnum_value, si_pid);
MRB_DEFINE_SIGINFO_MEMBER(uid, mrb_fixnum_value, si_uid);
#ifdef si_syscall
  MRB_DEFINE_SIGINFO_MEMBER(syscall, mrb_fixnum_value, si_syscall);
#endif

static mrb_data_type mrb_siginfo_type = { "siginfo", mrb_free };

static void mrb_siginfo_register_data(mrb_state *mrb, mrb_value obj, siginfo_t *si)
{
  siginfo_t *si2 = mrb_malloc(mrb, sizeof(siginfo_t));
  memcpy(si2, si, sizeof(siginfo_t));
  mrb_data_init(obj, si2, &mrb_siginfo_type);
}

/* Return to SignalThread functions */

static mrb_value mrb_signal_thread_waitinfo(mrb_state *mrb, mrb_value self)
{
  int sig, argc;
  mrb_value signo, block = mrb_nil_value(), mrb_si;
  mrb_value argv[1];
  sigset_t set, mask;
  siginfo_t siginfo;
  memset(&siginfo, 0, sizeof(siginfo_t));

  if ((argc = mrb_get_args(mrb, "o&", &signo, &block)) <= 0)
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments (1..2 for %S)", argc);

  if (!mrb_nil_p(block) && MRB_PROC_CFUNC_P(mrb_proc_ptr(block)))
    mrb_raise(mrb, E_RUNTIME_ERROR, "require block defined in ruby code");

  sig = trap_signm(mrb, signo);

  sigfillset(&mask);
  sigdelset(&mask, sig);
  if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0)
    mrb_raise(mrb, E_RUNTIME_ERROR, "set mask error");

  sigemptyset(&set);
  sigaddset(&set, sig);

  if (mrb_nil_p(block)) {
    /* just wait if no block given */
    sigwaitinfo(&set, &siginfo);

    mrb_si = mrb_obj_new(mrb, mrb_class_get(mrb, "SigInfo"), 0, NULL);
    mrb_siginfo_register_data(mrb, mrb_si, &siginfo);
    return mrb_si;
  } else {
    for (;;) {
      sigwaitinfo(&set, &siginfo);

      mrb_si = mrb_obj_new(mrb, mrb_class_get(mrb, "SigInfo"), 0, NULL);
      mrb_siginfo_register_data(mrb, mrb_si, &siginfo);
      argv[0] = mrb_si;
      mrb_yield_argv(mrb, block, 1, argv);
    }
    /* never return */
    mrb_raise(mrb, E_RUNTIME_ERROR, "[BUG] Wait loop seems broken");
  }
}

static mrb_thread_context* mrb_signal_thread_context(mrb_state *mrb, mrb_value self)
{
  if (strcmp(DATA_TYPE(self)->struct_name, "mrb_thread_context") != 0) {
    mrb_raisef(mrb, E_TYPE_ERROR, "wrong argument type %S (expected mrb_thread_context)",
               mrb_str_new_cstr(mrb, DATA_TYPE(self)->struct_name));
  }

  return DATA_PTR(self);
}

static mrb_value mrb_signal_thread_kill(mrb_state *mrb, mrb_value self)
{
  int sig;
  mrb_value *argv;
  mrb_int argc;
  mrb_thread_context *context = NULL;

  mrb_get_args(mrb, "*", &argv, &argc);
  if (argc != 1)
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong number of arguments (1 for %S)", mrb_fixnum_value(argc));

  sig = trap_signm(mrb, argv[0]);

  context = mrb_signal_thread_context(mrb, self);
  if (context->mrb == NULL)
    return mrb_nil_value();

  pthread_kill(context->thread, sig);
  return context->result;
}

static mrb_value mrb_signal_thread_thread_id(mrb_state *mrb, mrb_value self)
{
  mrb_thread_context *context = mrb_signal_thread_context(mrb, self);

  if (context->mrb == NULL) {
    return mrb_nil_value();
  }

  return mrb_float_value(mrb, (mrb_float)context->thread);
}

static mrb_value mrb_signal_thread_kill_by_thread_id(mrb_state *mrb, mrb_value self)
{
  mrb_int sig;
  mrb_float thread_id;
  mrb_value sig_obj;

  mrb_get_args(mrb, "fo", &thread_id, &sig_obj);

  sig = trap_signm(mrb, sig_obj);

  return mrb_fixnum_value(pthread_kill((pthread_t)thread_id, sig));
}

static mrb_value mrb_signal_thread_is_failed(mrb_state *mrb, mrb_value self)
{
  mrb_thread_context *context = mrb_signal_thread_context(mrb, self);
  if (context->mrb == NULL) {
    return mrb_nil_value();
  }

  return mrb_bool_value((mrb_bool)!!context->mrb->exc);
}

mrb_value mrb_thread_migrate_value(mrb_state *mrb, mrb_value v, mrb_state *mrb2);

static mrb_value mrb_signal_thread_exception(mrb_state *mrb, mrb_value self)
{
  mrb_thread_context *context = mrb_signal_thread_context(mrb, self);

  if (context->mrb == NULL || !context->mrb->exc) {
    return mrb_nil_value();
  }

  return mrb_thread_migrate_value(context->mrb, mrb_obj_value(context->mrb->exc), mrb);
}

static mrb_value mrb_signal_thread_cancel(mrb_state *mrb, mrb_value self)
{
  mrb_thread_context *context = mrb_signal_thread_context(mrb, self);
  sigset_t set;
  if (context->mrb == NULL) {
    return mrb_false_value();
  }

  if (context->alive) {
    /* reset sigmask */
    sigemptyset(&set);
    if (pthread_sigmask(SIG_SETMASK, &set, NULL) != 0)
      mrb_raise(mrb, E_RUNTIME_ERROR, "set mask error");

    if (pthread_cancel(context->thread) != 0) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "pthread_cancel failed");
    }
    if (pthread_join(context->thread, NULL) != 0) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "pthread canceled, but join failed");
    }
    context->alive = FALSE;
  }
  context->result = mrb_symbol_value(mrb_intern_lit(mrb, "canceled"));
  mrb_close(context->mrb);
  context->mrb = NULL;

  return context->result;
}

#ifdef __APPLE__
static int sigqueue(pid_t pid, int sig, const union sigval value)
{
  (void)value;
  return kill(pid, sig);
}
#endif

static mrb_value mrb_signal_thread_queue(mrb_state *mrb, mrb_value self)
{
  int sig;
  mrb_int pid;
  mrb_value sigobj;
  union sigval sv;
  sv.sival_int = 0; /* Dummy */

  mrb_get_args(mrb, "io", &pid, &sigobj);

  sig = trap_signm(mrb, sigobj);
  if (sigqueue((pid_t)pid, sig, sv) == -1)
    mrb_raise(mrb, E_RUNTIME_ERROR, "sigqueue failed");

  return mrb_fixnum_value(sig);
}

void mrb_mruby_signal_thread_gem_init(mrb_state *mrb)
{
  struct RClass *_class_thread = mrb_class_get(mrb, "Thread");
  struct RClass *signalthread, *siginfo;

  signalthread = mrb_define_class(mrb, "SignalThread", _class_thread);

  mrb_define_class_method(mrb, signalthread, "mask", mrb_signal_thread_mask, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, signalthread, "wait", mrb_signal_thread_wait, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, signalthread, "waitinfo", mrb_signal_thread_waitinfo, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, signalthread, "kill_by_thread_id", mrb_signal_thread_kill_by_thread_id, MRB_ARGS_REQ(2));

  mrb_define_method(mrb, signalthread, "_kill", mrb_signal_thread_kill, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, signalthread, "thread_id", mrb_signal_thread_thread_id, MRB_ARGS_NONE());
  mrb_define_method(mrb, signalthread, "failed?", mrb_signal_thread_is_failed, MRB_ARGS_NONE());
  mrb_define_method(mrb, signalthread, "exception", mrb_signal_thread_exception, MRB_ARGS_NONE());
  mrb_define_method(mrb, signalthread, "cancel", mrb_signal_thread_cancel, MRB_ARGS_NONE());

  mrb_define_class_method(mrb, signalthread, "queue", mrb_signal_thread_queue, MRB_ARGS_REQ(2));

  siginfo = mrb_define_class(mrb, "SigInfo", mrb->object_class);
  MRB_SET_INSTANCE_TT(siginfo, MRB_TT_DATA);
  mrb_define_method(mrb, siginfo, "uid", mrb_siginfo_get_uid, MRB_ARGS_NONE());
  mrb_define_method(mrb, siginfo, "pid", mrb_siginfo_get_pid, MRB_ARGS_NONE());
#ifdef si_syscall
  mrb_define_method(mrb, siginfo, "syscall", mrb_siginfo_get_syscall, MRB_ARGS_NONE());
#endif

  DONE;
}

void mrb_mruby_signal_thread_gem_final(mrb_state *mrb)
{
}
