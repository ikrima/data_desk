/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Data Desk

Author  : Ryan Fleury
Updated : 5 December 2019
License : MIT, at end of file.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// NOTE(rjf): Platform-Specific
__pragma(warning(push))
__pragma(warning(disable:4201 4152 4018 4204 4130 4102))
#define BUILD_WIN32 1
#define BUILD_LINUX 0

#if BUILD_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif BUILD_LINUX
#include <dlfcn.h>
#endif

// NOTE(rjf): C Runtime Library
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// NOTE(rjf): Data Desk Code
#define DATA_DESK_USE_OLD_NAMES 0
#include "data_desk.h"
#include "data_desk_debug.c"
#include "data_desk_custom.c"
#include "data_desk_utilities.c"
#include "data_desk_tokenizer.c"
#include "data_desk_parse.c"
#include "data_desk_graph_traverse.c"

static void PrintAndResetParseContextErrors(ParseContext* context)
{
  for (int i = 0; i < context->error_stack_size; ++i) {
    fprintf(stderr, "ERROR (%s:%i): %s\n", context->error_stack[i].file, context->error_stack[i].line, context->error_stack[i].string);
  }
  context->error_stack_size = 0;
}

static DataDeskNode* ParseFile(ParseContext* context, char* file, char* filename)
{
  Tokenizer tokenizer = {0};
  {
    tokenizer.at       = file;
    tokenizer.filename = filename;
    tokenizer.line     = 1;
  }

  DataDeskNode* root = ParseCode(context, &tokenizer);
  PrintAndResetParseContextErrors(context);

  // NOTE(rjf): ParseContextCleanUp shouldn't be called, because often time, code
  // will depend on ASTs persisting between files (which should totally work).
  // So, we won't clean up anything, and let the operating system do it ond
  // program exit. We can still let a ParseContext go out of scope, though,
  // because the memory will stay allocated. Usually, this is called a leak and
  // thus harmful, but because we want memory for all ASTs ever parsed within
  // a given runtime to persist (because we store pointers to them, etc.), we
  // actually don't care; this is /exactly/ what we want. The operating system
  // frees the memory on exit, and for this reason, there is literally no reason
  // to care about AST clean-up at all.
  // ParseContextCleanUp(&context);

  // NOTE(rjf): This is a reason why non-nuanced and non-context-specific programming
  // rules suck.

  return root;
}

static void ProcessParsedGraph(char* filename, DataDeskNode* root, ParseContext* context, DataDeskCustom custom)
{
  PatchGraphSymbols(context, root);
  GenerateGraphNullTerminatedStrings(context, root);
  CallCustomParseCallbacks(context, root, custom, filename);
  PrintAndResetParseContextErrors(context);
}

int datadesk_entry(const char* a_dsFilePath, int a_bLog, DataDeskCustom a_dsClbk)
{
  char* dsFilePath = (char*)a_dsFilePath;
  global_log_enabled = !!a_bLog;

  int expected_number_of_files = 1;

  Log("Data Desk v" DATA_DESK_VERSION_STRING);

  if (a_dsClbk.InitCallback) {
    a_dsClbk.InitCallback(a_dsClbk.clbkCtx);
  }

  ParseContext parse_context = {0};

  int number_of_parsed_files = 0;
  struct {
    DataDeskNode* root;
    char*         filename;
  }* parsed_files = ParseContextAllocateMemory(&parse_context, sizeof(*parsed_files) * expected_number_of_files);

  Assert(parsed_files != 0);



  Log("Processing file at \"%s\".", dsFilePath);
  char* file = LoadEntireFileAndNullTerminate(dsFilePath);
  if (file) {
    DataDeskNode* root                            = ParseFile(&parse_context, file, dsFilePath);
    parsed_files[number_of_parsed_files].root     = root;
    parsed_files[number_of_parsed_files].filename = dsFilePath;
    ++number_of_parsed_files;
  }
  else {
    LogError("ERROR: Could not load \"%s\".", dsFilePath);
  }


  ProcessParsedGraph(parsed_files[0].filename, parsed_files[0].root, &parse_context, a_dsClbk);

  if (a_dsClbk.CleanUpCallback) {
    a_dsClbk.CleanUpCallback(a_dsClbk.clbkCtx);
  }



  return 0;
}

__pragma(warning(pop))


/*
Copyright 2019 Ryan Fleury

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/