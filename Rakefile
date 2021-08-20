# encoding: utf-8
# frozen_string_literal: true

require "rake/clean"

DOSBOX_COMMAND = 'C:\DOSBox-X\dosbox-x'
GCC_OPTS = "-Wall -g -funsigned-char"

file "P2COM.OBJ" => ["p2com.asm","Spin2_interpreter.inc","Spin2_debugger.inc","flash_loader.inc"] do |t|
    # Remove the -silent to see why it's not working when it isn't ;/
    sh %Q{#{DOSBOX_COMMAND} -silent -fastlaunch -nopromptfolder -c "MOUNT C #{Dir.pwd} " -c "C:\\BUILD16.BAT"}
end

rule ".binary" => ".spin2" do |t|
    sh "flexspin -2 -b #{t.source}"
end

rule ".inc" => ".binary" do |t|
    puts "Converting #{t.source} to #{t.name}"
    File.write(t.name,File.binread(t.source).bytes.each_slice(16).map{|sl|"db #{sl.join ?,}\n"}.join)
end

file "p2com.elf" => "P2COM.OBJ" do |t|
    File.delete t.name if File.exist? t.name
    sh "./objconv -felf32 -nu+ -nd #{t.source} #{t.name}"
    raise unless File.exist? t.name
end

file "p2com.h" => ["p2com.asm","asm2header.rb"] do |t|
    puts "Generating p2com.h"
    load "asm2header.rb"
end

rule ".o" => [".c","p2com.h"] do |t|
    sh "gcc -c -m32 -static #{GCC_OPTS} #{t.source}"
end

file "spinner2.exe" => ["spinner2.o","p2com.elf"] do |t|
    sh "gcc -m32 -static #{t.sources.join ' '} -o #{t.name}"
end

task :default => "spinner2.exe"

CLEAN.include %w[*.o *.inc *.OBJ p2com.h]
CLOBBER.include %w[spinner2.exe]
