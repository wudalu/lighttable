
function convert_table_to_lt(tbl)
	local ret = lighttable.create()

	for k, v in pairs(tbl) do
		if type(v) == type({}) then
			--recursive
			ret[k] = convert_table_to_lt(v)
		else
			ret[k] = v
		end
	end
	
	--print(string.format("convert_table_to_lt exetime=%d", ETime - STime))
	return ret
end

function test_convert()

	--[[
	a = {}
	a[1] = "wudalu"
	a.str = 10000

	b = {["name"]="b", keycount=2}
	a["tbl"] = b
	local lt = convert_table_to_lt(a)
	]]

	--[[
	local ttt = {}
	ttt.a = 1
	ttt[1] = "fafafafa"
	ttt.b = "wuzexiang"
	ddd = {[1]=1000, ["name"]="name"}
	ttt.d = ddd
	local ct = lighttable.convert(ttt)
	lighttable.serialize(ct)
	--]]

	local a = dofile("anni_cele2012.lua")

	local STime = os.time()
	local lt
	for i=1,100 do
		lt = lighttable.convert(a.UserData)
		--lt = convert_table_to_lt(a.UserData)
	end

	local ETime = os.time()
	print(string.format("convert time=%d", ETime - STime))
	--lighttable.serialize(lt)
	print(lighttable.size(lt))
	lighttable.delete(lt)
end

function fun()
	--local t=lighttable.init()
	--[[
	local a=lighttable.create()
	a[1] = "fffffff"
	--print(a[1])
	--a["fa"] = 1
	--print(a["fa"])

	--lighttable.serialize(a)
	--lighttable.serialize(b)

	b = lighttable.create()
	b.name = "wuzexiang"
	b.id = 122
	a["tbl"] = b
	lighttable.serialize(a)
	--print(a["tbl"].name)
	
	a[2] = "200"
	a[3] = 300
	a[4] = "400"

	a[9] = 10000

	for i=1,10 do
		local key = "a" .. tostring(i)
		a[key] = i
	end

	print("=================================")

	for k, v in lighttable.pairs(a) do
		print(k,v)
	end

	print(lighttable.size(a))
	]]

	local ltt = lighttable.create()
	local ttt = {}
	local testdata = dofile("anni_cele2012.lua")
	local sec,msec = lighttable.real_time()
	for k,v in pairs(testdata.UserData) do
		ltt[k] = 1	
		--ltt[tostring(k)] = 1	
		--ttt[tostring(k)] = 1
		--ttt[tostring(k)] = 1
	end

	--[[
	for i=1,1000 do
		ltt[i] = 1
	end
	]]

	--lighttable.serialize(ltt)

	for k, v in lighttable.pairs(ltt) do
		print(k)
	end

	--[[
	for i=1,1000000 do
		ltt[i] = 1
		--ttt[i] = 1
	end
	]]

	--[[
	local sec,msec = lighttable.real_time()
	for k,v in lighttable.pairs(ltt) do
		print(v)
	end
	]]

	local esec, emsec =lighttable.real_time()
	print(string.format("luatable exe time = %d",(esec * 1000 + emsec / 1000) - (sec *1000 + msec/1000)))
	print(lighttable.size(ltt))
	lighttable.delete(ltt)
	--lighttable.serialize(a)
end

