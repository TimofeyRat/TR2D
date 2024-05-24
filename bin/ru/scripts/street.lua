function init()
	timer = 0
end

function main()
	timer = timer + getDeltaTime()
	if timer > 5 then
		local x = math.random(100, getNum("lvl current w") - 100)
		exec("spawnEnt GreenElemental "..x.." 2250")
		timer = 0
	end
end