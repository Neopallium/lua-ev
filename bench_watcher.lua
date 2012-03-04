#!/usr/bin/env lua

local ev = require"ev"
local loop = ev.Loop.default

local socket = require"socket"
local time = socket.gettime
local clock = os.clock
local quiet = false
local disable_gc = true

local N=10000000

local i=1
while i < #arg do
	local p=arg[i]
	if p == '-gc' then
		disable_gc = false
	elseif p == '-N' then
		i = i + 1
		N=tonumber(arg[i])
	end
	i = i + 1
end
print("N=",N)
if disable_gc then
	print"GC is disabled so we can track memory usage better"
	print""
end

local function printf(fmt, ...)
	local res
	if not quiet then
		fmt = fmt or ''
		res = print(string.format(fmt, ...))
		io.stdout:flush()
	end
	return res
end

local function full_gc()
	-- make sure all free-able memory is freed
	collectgarbage"collect"
	collectgarbage"collect"
	collectgarbage"collect"
end

local function bench(name, N, func, ...)
	local start1,start2
	printf('run bench: %s', name)
	start1 = clock()
	start2 = time()
	func(N, ...)
	local diff1 = (clock() - start1)
	local diff2 = (time() - start2)
	printf("total time: %10.6f (%10.6f) seconds", diff1, diff2)
	return diff1, diff2
end

local function mper(mem, N)
	local per = mem / N
	if per < 1 then per = 0 end
	return per
end

local function run_watcher_speedtest(N, name, create)
	local start_mem, end_mem

	local pending = 0
	local function cb()
		pending = pending - 1
		if pending <= 0 then loop:unloop() end
	end

	local watcher = create(cb)
	watcher:start(loop)

	local function run(N)
		pending = N
		loop:loop()
	end

	full_gc()
	start_mem = (collectgarbage"count" * 1024)
	--print(name, 'start memory size: ', start_mem)
	if disable_gc then collectgarbage"stop" end
	local diff1, diff2 = bench(name, N, run)
	end_mem = (collectgarbage"count" * 1024)
	local total = N
	printf("units/sec: %10.6f (%10.6f) units/sec", total/diff1, total/diff2)
	--print(name, 'end   memory size: ', end_mem)
	local mem = (end_mem - start_mem)
	print(name.. ': total memory used: '.. mem.. ' bytes,  used per loop: '..
		mper(mem,total).. ' bytes')
	print()

	watcher:stop(loop)
	collectgarbage"restart"
	full_gc()
end

local function run_watcher_stop_start_speedtest(N, name, create)
	local start_mem, end_mem

	local watcher1, watcher2
	local pending = 0
	local function cb()
		pending = pending - 1
		if pending <= 0 then loop:unloop() end
	end

	-- create watcher 1
	watcher1 = create(function()
		watcher1:stop(loop)
		watcher2:start(loop)
		cb()
	end)

	-- create watcher 2
	watcher2 = create(function()
		watcher2:stop(loop)
		watcher1:start(loop)
		cb()
	end)

	-- start only watcher 1
	watcher1:start(loop)

	local function run(N)
		pending = N
		loop:loop()
	end

	full_gc()
	start_mem = (collectgarbage"count" * 1024)
	--print(name, 'start memory size: ', start_mem)
	if disable_gc then collectgarbage"stop" end
	local diff1, diff2 = bench(name, N, run)
	end_mem = (collectgarbage"count" * 1024)
	local total = N
	printf("units/sec: %10.6f (%10.6f) units/sec", total/diff1, total/diff2)
	--print(name, 'end   memory size: ', end_mem)
	local mem = (end_mem - start_mem)
	print(name.. ': total memory used: '.. mem.. ' bytes,  used per loop: '..
		mper(mem,total).. ' bytes')
	print()

	watcher1:stop(loop)
	watcher2:stop(loop)
	collectgarbage"restart"
	full_gc()
end

local function per_watcher_overhead(N, name, create, cb)
	local start_mem, end_mem
	local watchers = {}
 
	-- pre-grow table
	for i=1,N do
		watchers[i] = true -- add place-holder values.
	end
	full_gc()
	start_mem = (collectgarbage"count" * 1024)
	--print('overhead: start memory size: ', start_mem)
	for i=1,N do
		watchers[i] = create(cb)
	end
	full_gc()
	end_mem = (collectgarbage"count" * 1024)
	--print('overhead: end   memory size: ', end_mem)
	print(name.. ': total memory used: '.. mper((end_mem - start_mem), N).. ' bytes per watcher')
	print()
   
	watchers = nil
	full_gc()
end

local tests = {
idle = {
	create = function(cb)
		return ev.Idle.new(cb, 0, ev.READ)
	end,
},
}

local function null_cb()
end

print('overhead test')
for name,test in pairs(tests) do
	per_watcher_overhead(N/100, name, test.create, null_cb)
end

---[[
print()
print('speed test watcher callback overhead')
for name,test in pairs(tests) do
	run_watcher_speedtest(N, name, test.create)
end
--]]

print()
print('speed test watcher start/stop overhead')
for name,test in pairs(tests) do
	run_watcher_stop_start_speedtest(N, name, test.create)
end

