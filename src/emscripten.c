#include <mruby.h>
#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <emscripten.h>

//
// callback function
//

typedef struct {
  mrb_state *mrb;
  mrb_value self;
} idb_callback_context;

static void callback_call(idb_callback_context* context, mrb_value arg, const char* callback_name)
{
  mrb_state *mrb = context->mrb;
  mrb_value self = context->self;
  mrb_value callback = mrb_iv_get(mrb,self, mrb_intern_cstr(mrb,callback_name));
  mrb_value argv[1] = { arg };
  mrb_yield_argv(mrb,callback,1,argv);
  mrb_free(context->mrb,context);
}

static void onstore(void* context)
{
  callback_call( context, mrb_true_value(), "@store_callback" );
}

static void onstore_error(void* context)
{
  callback_call( context, mrb_false_value(), "@store_callback" );
}

static void onload(void* context, void* buf, int size)
{
  mrb_value result = mrb_str_new( ((idb_callback_context*)context)->mrb, buf, size);
  callback_call( context, result, "@load_callback" );
}

static void onload_error(void* context)
{
  callback_call( context, mrb_nil_value(), "@load_callback" );
}

static void ondelete(void* context)
{
  callback_call( context, mrb_true_value(), "@delete_callback" );
}

static void ondelete_error(void* context)
{
  callback_call( context, mrb_false_value(), "@delete_callback" );
}

static void onexists(void* context, int exists)
{
  callback_call( context, exists==0 ? mrb_false_value() : mrb_true_value(), "@exists_callback" );
}

static void onexists_error(void* context)
{
  callback_call( context, mrb_nil_value(), "@exists_callback" );
}
//
//
//
static mrb_value mrb_idb_initialize(mrb_state *mrb, mrb_value self)
{
    mrb_value dbname;
    mrb_get_args(mrb, "S", &dbname );
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb,"@dbname"), dbname );
    return self;
}

static mrb_value mrb_idb_store(mrb_state *mrb, mrb_value self)
{
    mrb_value _key,_buffer,callback;
    mrb_get_args(mrb, "SS&", &_key, &_buffer, &callback );

    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb,"@store_callback"), callback );
    mrb_value _dbname = mrb_iv_get(mrb,self, mrb_intern_cstr(mrb,"@dbname"));
    const char *dbname = RSTRING_PTR(_dbname);
    const char *key = RSTRING_PTR(_key);
    const char *buffer = RSTRING_PTR(_buffer);
    int len = RSTRING_LEN(_buffer);

    idb_callback_context *context = mrb_malloc(mrb,sizeof(idb_callback_context));
    context->mrb = mrb;
    context->self = self;

    emscripten_idb_async_store(dbname, key, buffer, len, context, onstore, onstore_error);

    return self;
}

static mrb_value mrb_idb_load(mrb_state *mrb, mrb_value self)
{
    mrb_value _key,callback;
    mrb_get_args(mrb, "S&", &_key, &callback );

    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb,"@load_callback"), callback );
    mrb_value _dbname = mrb_iv_get(mrb,self, mrb_intern_cstr(mrb,"@dbname"));
    const char *dbname = RSTRING_PTR(_dbname);
    const char *key = RSTRING_PTR(_key);

    idb_callback_context *context = mrb_malloc(mrb,sizeof(idb_callback_context));
    context->mrb = mrb;
    context->self = self;

    emscripten_idb_async_load(dbname, key, context, onload, onload_error);

    return self;
}

static mrb_value mrb_idb_delete(mrb_state *mrb, mrb_value self)
{
    mrb_value _key,callback;
    mrb_get_args(mrb, "S&", &_key, &callback );

    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb,"@delete_callback"), callback );
    mrb_value _dbname = mrb_iv_get(mrb,self, mrb_intern_cstr(mrb,"@dbname"));
    const char *dbname = RSTRING_PTR(_dbname);
    const char *key = RSTRING_PTR(_key);

    idb_callback_context *context = mrb_malloc(mrb,sizeof(idb_callback_context));
    context->mrb = mrb;
    context->self = self;

    emscripten_idb_async_delete(dbname, key, context, ondelete, ondelete_error);

    return self;
}

static mrb_value mrb_idb_exists(mrb_state *mrb, mrb_value self)
{
    mrb_value _key,callback;
    mrb_get_args(mrb, "S&", &_key, &callback );

    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb,"@exists_callback"), callback );
    mrb_value _dbname = mrb_iv_get(mrb,self, mrb_intern_cstr(mrb,"@dbname"));
    const char *dbname = RSTRING_PTR(_dbname);
    const char *key = RSTRING_PTR(_key);

    idb_callback_context *context = mrb_malloc(mrb,sizeof(idb_callback_context));
    context->mrb = mrb;
    context->self = self;

    emscripten_idb_async_exists(dbname, key, context, onexists, onexists_error);

    return self;
}

static mrb_value mrb_run_script(mrb_state *mrb, mrb_value self)
{
    mrb_value code;
    mrb_get_args(mrb, "S", &code );
    emscripten_run_script( mrb_string_cstr(mrb,code) );
    return self;
}

static mrb_value mrb_run_script_int(mrb_state *mrb, mrb_value self)
{
    mrb_value code;
    mrb_get_args(mrb, "S", &code );
    int i = emscripten_run_script_int( mrb_string_cstr(mrb,code) );
    return mrb_fixnum_value(i);
}

static mrb_value mrb_run_script_double(mrb_state *mrb, mrb_value self)
{
    mrb_value code;
    mrb_get_args(mrb, "S", &code );
    double d = emscripten_run_script_double( mrb_string_cstr(mrb,code) );
    return mrb_float_value(mrb,d);
}


void mrb_mruby_emscripten_gem_init(mrb_state* mrb)
{
  struct RClass *emscripten, *idb;
  emscripten = mrb_define_class(mrb, "Emscripten", mrb->object_class);
  idb = mrb_define_class_under(mrb, emscripten, "IndexedDB", mrb->object_class);

  mrb_define_method(mrb, idb, "initialize", mrb_idb_initialize, MRB_ARGS_REQ(1)); // dbname
  mrb_define_method(mrb, idb, "store", mrb_idb_store, MRB_ARGS_REQ(3)); // key,value,callback
  mrb_define_method(mrb, idb, "load", mrb_idb_load, MRB_ARGS_REQ(2)); // key,callback
  mrb_define_method(mrb, idb, "delete", mrb_idb_delete, MRB_ARGS_REQ(2)); // key,callback
  mrb_define_method(mrb, idb, "exists", mrb_idb_exists, MRB_ARGS_REQ(2)); // key,callback

  mrb_define_class_method(mrb, emscripten, "run_script", mrb_run_script, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, emscripten, "run_script_int", mrb_run_script_int, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, emscripten, "run_script_double", mrb_run_script_double, MRB_ARGS_REQ(1));
}

void mrb_mruby_emscripten_gem_final(mrb_state* mrb)
{
}
