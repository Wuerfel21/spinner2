# encoding: utf-8
# frozen_string_literal: true

require "rake/clean"

def windows?
    Gem.win_platform?
end

EXE_NAME = "spinner2#{".exe" if windows?}"
BOOTSTRAP_EXE_NAME = "spinner2_bootstrap#{".exe" if windows?}"

GCC_OPTS = "-Wall -g -funsigned-char -fno-pic"

file "p2com.OBJ" => ["p2com.asm","Spin2_interpreter.inc","Spin2_debugger.inc","flash_loader.inc"] do |t|
    sh "#{"wine " unless windows?} ./TASM32.EXE #{t.source} /m /l /z /c"
end
file "p2com_bootstrap.OBJ" => ["p2com_bootstrap.asm"] do |t|
    sh "#{"wine " unless windows?} ./TASM32.EXE #{t.source} /m /l /z /c"
end

file "p2com_bootstrap.asm" => "p2com.asm" do |t|
    puts "Generating bare PASM compiler source for bootstrapping"
    # Just remove all the include directives
    File.write(t.name,File.read(t.source).gsub(/include\s*"\w+\.inc"/,''))
end

rule ".binary" => [".spin2",BOOTSTRAP_EXE_NAME] do |t|
    sh "./#{BOOTSTRAP_EXE_NAME} #{t.source}"
end

rule ".inc" => ".binary" do |t|
    puts "Converting #{t.source} to #{t.name}"
    File.write(t.name,File.binread(t.source).bytes.each_slice(16).map{|sl|"db #{sl.join ?,}\n"}.join)
end

rule ".elf" => ".OBJ" do |t|
    File.delete t.name if File.exist? t.name
    sh "#{"wine " unless windows?}./objconv.exe -felf32 #{"-nu+" if windows?} -nd #{t.source} #{t.name}"
    raise unless File.exist? t.name
end

file "p2com.h" => ["p2com.asm","asm2header.rb"] do |t|
    puts "Generating p2com.h"
    load "asm2header.rb"
end

rule ".o" => [".c","p2com.h"] do |t|
    sh "gcc -c -m32 #{GCC_OPTS} #{t.source}"
end

file EXE_NAME => ["spinner2.o","p2com.elf"] do |t|
    sh "gcc -m32 #{t.sources.join ' '} -no-pie -o #{t.name}"
end

file BOOTSTRAP_EXE_NAME => ["spinner2.o","p2com_bootstrap.elf"] do |t|
    sh "gcc -m32 #{t.sources.join ' '} -no-pie -o #{t.name}"
end

task :default => EXE_NAME

CLEAN.include %W[*.o *.inc *.binary *.OBJ *.elf p2com.h p2com_bootstrap.asm #{BOOTSTRAP_EXE_NAME}]
CLOBBER.include [EXE_NAME]
