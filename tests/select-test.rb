#!/usr/bin/env ruby

pipes = [IO.pipe, IO.pipe, IO.pipe, IO.pipe]

fennel=ENV["FENNEL_LUA"]

child = fork
if child.nil?
  pipes.each do |p|
    p[0].close_on_exec = false
    p[1].close
  end
  redirs = [0,1,2,3].map {|fd| "#{40+fd}<&#{pipes[fd][0].fileno}"}.join(" ")
  cmd = "./src/upscript #{fennel} --add-fennel-path ./scripts/?.fnl tests/select-test-fn.fnl #{redirs}"
  warn cmd
  Kernel.system(cmd)
  exit
end


outs = []
pipes.each do |p|
  p[0].close
  outs << p[1]
end
outs[1].puts "backer"
outs[0].puts "able"
outs[2].puts "charlie"
outs[3].puts "dibbler"
sleep(0.1)
outs[3].puts "chew\0backer"
outs[2].puts "able bodied"
sleep(0.01)
outs[1].puts "charlie says"
outs[0].puts "dibbler pew"
sleep(4)
# the process under test should have exited by now, but if it
# hasn't, send it some more data
Process.wait(child, Process::WNOHANG) or outs[0].puts "tardy"
outs.map(&:close)

begin
  Process.wait()
rescue Errno::ECHILD
  nil
end
