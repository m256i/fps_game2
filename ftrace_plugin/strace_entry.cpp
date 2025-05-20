
#include <unordered_set>
#include <string>
#include <windows.h>
#include <gcc-plugin.h>
#include <plugin-version.h>
#include <tree.h>
#include <gimple.h>
#include <pass_manager.h>
#include <gimple-iterator.h>
#include <context.h>
#include <function.h>
#include <tree-pass.h>
#include <c-family/c-common.h>

__declspec(dllexport) int plugin_is_GPL_compatible;

const pass_data instrument_pass_data = {
  GIMPLE_PASS,                       /* type */
  "instrument_strace",               /* name */
  OPTGROUP_NONE,                     /* opt info */
  TV_NONE,                           /* tv_id */
  0,                                 /* properties_required */
  0,                                 /* properties_provided */
  0,                                 /* properties_destroyed */
  TODO_cleanup_cfg,                  /* todo_flags_start */
  TODO_update_ssa | TODO_cleanup_cfg /* todo_flags_finish */
};

static tree ptr_type;
static tree str_type;
static tree retaddr_decl;
static tree typelist;

static bool initialized = false;

static bool has_attr_no_strace(tree decl) {
  for (tree al = DECL_ATTRIBUTES(decl); al; al = TREE_CHAIN(al)) {
    tree name = TREE_PURPOSE(al);
    if (name && !strcmp(IDENTIFIER_POINTER(name), "no_strace")) return true;
  }
  return false;
}

static std::unordered_set<std::string> skip_set;

struct instrument_pass : gimple_opt_pass {
  instrument_pass(gcc::context *ctxt) :
    gimple_opt_pass(instrument_pass_data, ctxt) {}

  unsigned int execute(function *fun) override {
    if (!initialized) {
      ptr_type     = build_pointer_type(unsigned_char_type_node);
      str_type     = build_pointer_type(char_type_node);
      retaddr_decl = builtin_decl_implicit(BUILT_IN_RETURN_ADDRESS);
      typelist =
        build_function_type_list(void_type_node, ptr_type, str_type, NULL_TREE);
      initialized = true;
      puts(" > temporary trees setup");
    }

    const char *current_func_name = function_name(fun);

    if (!current_func_name) {
      return 0;
    }

    if (strcmp(current_func_name, "__instrument_strace") == 0) {
      printf("  > skipping symbol: <%s>\n", current_func_name);
      return 0;
    }

    if (has_attr_no_strace(fun->decl)) {
      printf("  > skipping symbol: <%s> (by userflag)\n", current_func_name);
      skip_set.insert(current_func_name);
      return 0;
    }

    if (skip_set.find(current_func_name) != skip_set.end()) {
      printf(
        "  > skipping symbol: <%s> (by passed userflag)\n",
        current_func_name
      );
      return 0;
    }

    basic_block          bb;
    gimple_stmt_iterator gsi;

    FOR_EACH_BB_FN(bb, fun) {
      gsi = gsi_start_bb(bb);
      if (!gsi_end_p(gsi)) break;
    }
    if (gsi_end_p(gsi)) {
      puts("empty body skipping");
      return 0;
    }

    printf("  > patching symbol: %s\n", current_func_name);

    tree name_str =
      build_string_literal(strlen(current_func_name) + 1, current_func_name);
    tree fn_instrument = build_fn_decl("__instrument_strace", typelist);
    tree addr_callsite = build_call_expr(retaddr_decl, 1, integer_zero_node);

    gcall *gc = gimple_build_call(fn_instrument, 2, addr_callsite, name_str);
    gsi_safe_insert_before(&gsi, gc);
    return 0;
  }
};

static struct attribute_spec no_strace_attr = {
  .name                   = "no_strace",
  .min_length             = 0,
  .max_length             = 0,
  .type_required          = false,
  .function_type_required = false,
  .affects_type_identity  = false,
  .handler                = nullptr,
};

static void register_my_attributes(void *event_data, void *data) {
  register_attribute(&no_strace_attr);
}

__declspec(dllexport) int plugin_init(
  struct plugin_name_args   *plugin_info,
  struct plugin_gcc_version *version
) {
  if (!plugin_default_version_check(version, &gcc_version)) {
    printf(
      "incompatible gcc (%s) version for plugin gcc version: %s ",
      gcc_version.basever,
      version->basever
    );
    return 1;
  }

  puts("--- running gcc plugin strace_dump --- (written by blue)");

  static instrument_pass ftrace_pass{g};

  register_pass_info pass_info;
  pass_info.pass = &ftrace_pass;

  if (optimize) {
    pass_info.reference_pass_name = "pre";
  } else {
    pass_info.reference_pass_name = "cfg";
  }

  pass_info.ref_pass_instance_number = 1;
  pass_info.pos_op                   = PASS_POS_INSERT_AFTER;

  register_callback(
    plugin_info->base_name,
    PLUGIN_ATTRIBUTES,
    register_my_attributes,
    NULL
  );

  register_callback(
    plugin_info->base_name,
    PLUGIN_PASS_MANAGER_SETUP,
    nullptr,
    &pass_info
  );

  return 0;
}

extern "C" {
BOOL APIENTRY __declspec(dllexport)
     DllMain(HINSTANCE hInst, DWORD reason, LPVOID lpReserved) {
  return TRUE;
}
BOOL WINAPI __declspec(dllexport)
     DllEntryPoint(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
  return DllMain(hinst, reason, reserved);
}
}