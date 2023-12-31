/*
 * Copyright 2016 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "wabt/config.h"

#include "wabt/apply-names.h"
#include "wabt/common.h"
#include "wabt/error-formatter.h"
#include "wabt/feature.h"
#include "wabt/generate-names.h"
#include "wabt/ir.h"
#include "wabt/option-parser.h"
#include "wabt/stream.h"
#include "wabt/wast-parser.h"
#include "wabt/wat-writer.h"

using namespace wabt;

static const char* s_infile;
static const char* s_outfile;
static bool s_debug_parsing;
static bool s_fold_exprs;
static bool s_generate_names;
static bool s_inline_import;
static bool s_inline_export;
static Features s_features;

static const char s_description[] =
    R"(  read a file in the wasm s-expression format and format it.

examples:
  # write output to stdout
  $ wat-desugar test.wat

  # write output to test2.wat
  $ wat-desugar test.wat -o test2.wat

  # generate names for indexed variables
  $ wat-desugar --generate-names test.wat
)";

static void ParseOptions(int argc, char** argv) {
  OptionParser parser("wat-desugar", s_description);

  parser.AddOption('o', "output", "FILE", "Output file for the formatted file",
                   [](const char* argument) { s_outfile = argument; });
  parser.AddOption("debug-parser", "Turn on debugging the parser of wat files",
                   []() { s_debug_parsing = true; });
  parser.AddOption('f', "fold-exprs", "Write folded expressions where possible",
                   []() { s_fold_exprs = true; });
  parser.AddOption("inline-exports", "Write all exports inline",
                   []() { s_inline_export = true; });
  parser.AddOption("inline-imports", "Write all imports inline",
                   []() { s_inline_import = true; });
  s_features.AddOptions(&parser);
  parser.AddOption(
      "generate-names",
      "Give auto-generated names to non-named functions, types, etc.",
      []() { s_generate_names = true; });

  parser.AddArgument("filename", OptionParser::ArgumentCount::One,
                     [](const char* argument) { s_infile = argument; });
  parser.Parse(argc, argv);
}

int ProgramMain(int argc, char** argv) {
  InitStdio();
  ParseOptions(argc, argv);

  std::vector<uint8_t> file_data;
  Result result = ReadFile(s_infile, &file_data);
  if (Failed(result)) {
    WABT_FATAL("unable to read %s\n", s_infile);
  }

  Errors errors;
  std::unique_ptr<WastLexer> lexer(WastLexer::CreateBufferLexer(
      s_infile, file_data.data(), file_data.size(), &errors));

  std::unique_ptr<Script> script;
  WastParseOptions parse_wast_options(s_features);
  result = ParseWastScript(lexer.get(), &script, &errors, &parse_wast_options);
  auto line_finder = lexer->MakeLineFinder();
  FormatErrorsToFile(errors, Location::Type::Text);

  if (Succeeded(result)) {
    Module* module = script->GetFirstModule();
    if (!module) {
      WABT_FATAL("no module in file.\n");
    }

    if (s_generate_names) {
      result = GenerateNames(module);
    }

    if (Succeeded(result)) {
      result = ApplyNames(module);
    }

    if (Succeeded(result)) {
      WriteWatOptions wat_options(s_features);
      wat_options.fold_exprs = s_fold_exprs;
      wat_options.inline_import = s_inline_import;
      wat_options.inline_export = s_inline_export;
      FileStream stream(s_outfile ? FileStream(s_outfile) : FileStream(stdout));
      result = WriteWat(&stream, module, wat_options);
    }
  }

  return result != Result::Ok;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}
