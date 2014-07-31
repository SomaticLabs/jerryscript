/* Copyright 2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef __TARGET_MCU
#include "common-io.h"
#include "actuators.h"
#include "sensors.h"
#include "generated.h"
#endif

#include "globals.h"
#include "interpreter.h"
#include "jerry-libc.h"
#include "lexer.h"
#include "parser.h"
#include "serializer.h"
#include "deserializer.h"
#include "optimizer-passes.h"

#define MAX_STRINGS 100
#define MAX_NUMS 25

static const OPCODE *
parser_run (const char *script_source, size_t script_source_size, bool is_show_opcodes)
{
  const char *strings[MAX_STRINGS];
  int32_t nums[MAX_NUMS];
  uint8_t strings_num, nums_count;
  uint16_t offset;
  const OPCODE *opcodes;

  lexer_init( script_source, script_source_size, is_show_opcodes);

  lexer_run_first_pass();

  strings_num = lexer_get_strings (strings);
  nums_count = lexer_get_nums (nums);
  lexer_adjust_num_ids ();

  offset = serializer_dump_strings (strings, strings_num);
  serializer_dump_nums (nums, nums_count, offset, strings_num);
  
  lexer_free ();

  parser_init ();
  parser_parse_program ();

  opcodes = deserialize_bytecode ();

  optimizer_run_passes ((OPCODE *) opcodes);

#ifdef __HOST
  serializer_print_opcodes ();
#endif

  return opcodes;
}

static void
jerry_run (const char *script_source, size_t script_source_size, bool is_parse_only, 
           bool is_show_opcodes)
{
  const OPCODE *opcodes;

  mem_init();

  serializer_init (is_show_opcodes);

  opcodes = parser_run (script_source, script_source_size, is_show_opcodes);

  if (is_parse_only)
    {
	    return;
    }

  init_int (opcodes);
  
#ifdef __HOST
  run_int ();
#endif
} /* jerry_run */

#ifdef __HOST
static uint8_t source_buffer[ JERRY_SOURCE_BUFFER_SIZE ];

static const char*
read_sources (const char *script_file_names[],
              size_t files_count,
              size_t *out_source_size_p)
{
  size_t i;
  uint8_t *source_buffer_tail = source_buffer;

  for (i = 0; i < files_count; i++)
    {
      const char *script_file_name = script_file_names[i];

      _FILE *file = __fopen (script_file_name, "r");

      if (file == NULL)
        {
          jerry_exit (ERR_IO);
        }

      int fseek_status = __fseek( file, 0, __SEEK_END);

      if ( fseek_status != 0 )
        {
          jerry_exit (ERR_IO);
        }

      long script_len = __ftell( file);

      if ( script_len < 0 )
        {
          jerry_exit (ERR_IO);
        }

      __rewind( file);

      const size_t source_size = (size_t)script_len;
      size_t bytes_read = 0;
     
      while ( bytes_read < source_size )
        {
          bytes_read += __fread( source_buffer_tail, 1, source_size, file);

          if ( __ferror( file) != 0 )
            {
              jerry_exit (ERR_IO);
            }
        }

      __fclose( file);

      source_buffer_tail += source_size;

      *out_source_size_p += source_size;
    }

  return (const char*)source_buffer;
}

int
main (int argc __unused,
      char **argv __unused)
{
  const char *file_names[argc];
  bool parse_only = false, show_opcodes = false;
  bool print_mem_stats = false;
  int i;
  size_t files_counter = 0;

  for (i = 1; i < argc; i++)
    {
      if (!__strcmp ("-v", argv[i]))
        {
          __printf("Build date: \t%s\n", JERRY_BUILD_DATE);
          __printf("Commit hash:\t%s\n", JERRY_COMMIT_HASH);
          __printf("Branch name:\t%s\n", JERRY_BRANCH_NAME);
          __printf("\n");
        }
      if (!__strcmp ("--mem-stats", argv[i]))
        {
          print_mem_stats = true;
        }
      else if (!__strcmp ("--parse-only", argv[i]))
        {
          parse_only = true;
        }
      else if (!__strcmp ("--show-opcodes", argv[i]))
        {
          show_opcodes = true;
        }
      else
        {
          file_names[files_counter++] = argv[i];
        }
    }

  if (files_counter == 0)
    {
      jerry_exit (ERR_NO_FILES);
    }
 
  size_t source_size;
  const char *source_p = read_sources (file_names, files_counter, &source_size);

  jerry_run (source_p, source_size, parse_only, show_opcodes);

  if (print_mem_stats)
    {
      mem_heap_print( false, false, true);
    }
  return 0;
}
#endif

#ifdef __TARGET_MCU
static uint32_t start;
static uint32_t finish_native_ms __unused;
static uint32_t finish_parse_ms __unused;
static uint32_t finish_int_ms __unused;

int
main (void)
{
  initialize_sys_tick ();
  initialize_leds ();
  initialize_timer ();

  led_on (13);

  const char *source_p = generated_source;
  const size_t source_size = sizeof (generated_source);

  set_sys_tick_counter ((uint32_t) - 1);
  start = get_sys_tick_counter ();
  jerry_run (source_p,
             source_size, false, false);
  finish_parse_ms = (start - get_sys_tick_counter ()) / 1000;
  led_on (14);

  set_sys_tick_counter ((uint32_t) - 1);
  start = get_sys_tick_counter ();
  run_int ();
  finish_int_ms = (start - get_sys_tick_counter ()) / 1000;

  led_on (15);
}
#endif
