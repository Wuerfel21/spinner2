# encoding: utf-8
# frozen_string_literal: true

RIP_EQUATES = %w[
    file_limit obj_limit info_limit
    debug_data_limit debug_string_limit debug_display_limit
]
TYPEMAP = {
    "dbx" => "uint8_t",
    "bwx" => "union unk16",
    "ddx" => "union unk32",

    "error_msg" => "char *",
    "source" => "char *",
    "source_start" => "int32_t",
    "source_finish" => "int32_t",
    "list" => "char *",
    "doc" => "char *",
    "dat_offsets" => "uint8_t *",
    "obj_offsets" => "uint8_t *",
    "dat_lengths" => "uint32_t",
    "obj_lengths" => "uint32_t",
    "size_obj" => "uint32_t",
    "size_interpreter" => "uint32_t",
    "size_var" => "uint32_t",
    "size_flashloader" => "uint32_t",
    "obj_ptr" => "uint32_t", # Not actually a pointer

    "obj_filenames" => "char",
    "dat_filenames" => "char",

    "P2InitStruct" => "struct Spin2Compiler *",
}

def value_xlate(str)
    RIP_EQUATES.each{|equ|str.gsub!(equ,"SPIN2_"+equ.upcase)}
    str.gsub!(/(\d+)h/,'0x\1')
    return str
end

File.open "p2com.asm",?r do |infile|

    structdef = "".dup
    constdef = "".dup
    funcdef = "".dup

    infile.each_line do |line|

        # Handle special cases
        case line
        when /^(\w+)\s*=\s*([\w*+]+)\s*(?:;.*)?$/
            if RIP_EQUATES.include? $1
                constdef << "#define SPIN2_#{$1.upcase} #{value_xlate($2)}\n"
            end
        when /^(dbx|dwx|ddx)\s+(\w+)\s*(?:,([\w*+]+))?\s*(?:;.*)?$/
            type = TYPEMAP[$2]||TYPEMAP[$1]
            if $3.nil?
                structdef << "#{type} #{$2};\n"
            else
                structdef <<  "#{type} #{$2}[#{value_xlate($3)}];\n"
            end
        when /^\s*public\s+(\w+)\s*$/
            funcdef << "#{TYPEMAP[$1]||'void'} #{$1.upcase}();\n"
            funcdef << "#define #{$1} #{$1.upcase}\n"
        when /^\s*proc\s+P2Compile1\s*$/
            # Reached code
            break
        else
            # Ignore
        end
    end 
    File.write "p2com.h",<<~HEAD
        // Auto-generated header file for Spin2 compiler
        #pragma once
        #include <stdint.h>

        #ifdef __cplusplus
        extern "C" {
        #endif

        #{constdef}

        union unk32 {
            int32_t s;
            uint32_t u;
            float f;
            void *ptr;
        };
        union unk16 {
            int16_t s;
            uint16_t u;
        };

        struct __attribute__((packed)) Spin2Compiler {
        #{structdef.gsub(/^/,'    ')}
        };
        #{funcdef}
        #ifdef __cplusplus
        }
        #endif
    HEAD
end

