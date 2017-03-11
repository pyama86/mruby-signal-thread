/*
** mrb_signalthread.c - SignalThread class
**
** Copyright (c) pyama86 2017
**
** See Copyright Notice in LICENSE
*/

#include "mruby.h"
#include "mruby/data.h"
#include "mrb_signalthread.h"

#define DONE mrb_gc_arena_restore(mrb, 0);

typedef struct {
  char *str;
  int len;
} mrb_signalthread_data;

static const struct mrb_data_type mrb_signalthread_data_type = {
  "mrb_signalthread_data", mrb_free,
};

static mrb_value mrb_signalthread_init(mrb_state *mrb, mrb_value self)
{
  mrb_signalthread_data *data;
  char *str;
  int len;

  data = (mrb_signalthread_data *)DATA_PTR(self);
  if (data) {
    mrb_free(mrb, data);
  }
  DATA_TYPE(self) = &mrb_signalthread_data_type;
  DATA_PTR(self) = NULL;

  mrb_get_args(mrb, "s", &str, &len);
  data = (mrb_signalthread_data *)mrb_malloc(mrb, sizeof(mrb_signalthread_data));
  data->str = str;
  data->len = len;
  DATA_PTR(self) = data;

  return self;
}

static mrb_value mrb_signalthread_hello(mrb_state *mrb, mrb_value self)
{
  mrb_signalthread_data *data = DATA_PTR(self);

  return mrb_str_new(mrb, data->str, data->len);
}

static mrb_value mrb_signalthread_hi(mrb_state *mrb, mrb_value self)
{
  return mrb_str_new_cstr(mrb, "hi!!");
}

void mrb_mruby_signal_thread_gem_init(mrb_state *mrb)
{
  struct RClass *signalthread;
  signalthread = mrb_define_class(mrb, "SignalThread", mrb->object_class);
  mrb_define_method(mrb, signalthread, "initialize", mrb_signalthread_init, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, signalthread, "hello", mrb_signalthread_hello, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, signalthread, "hi", mrb_signalthread_hi, MRB_ARGS_NONE());
  DONE;
}

void mrb_mruby_signal_thread_gem_final(mrb_state *mrb)
{
}

