function init()
	timer = 0
end

function main()
	timer = timer + getDeltaTime()
	if timer > 5 then
		local dist = math.random(-1000, 1000)
		local x = getNum("ent Nasake posX") + dist
		exec("spawnEnt GreenElemental "..x.." 2260")
		timer = 0
	end
end