function init()
	timer = 0
end

function clamp(x, min, max)
	return math.max(min, math.min(max, x))
end

function main()
	timer = timer + getDeltaTime()
	if timer > 5 then
		local dist = math.random(-1000, 1000)
		local x = clamp(getNum("ent Nasake posX") + dist, 100, getNum("lvl current w") - 100)
		exec("spawnEnt GreenElemental "..x.." 2260")
		timer = 0
	end
end