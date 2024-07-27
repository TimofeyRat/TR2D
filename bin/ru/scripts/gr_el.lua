require("ru/scripts/funcs")

function main()
	setExecNum("attacking", 0)
	setExecNum("dx", 0)
	setExecNum("speed", 1)
	local dist = distanceToPlayer(getExecNum("posX"), getExecNum("posY"))
	if (dist < 750) then
		dist = getNum("camOwner-posX") - getExecNum("posX")
		setExecNum("attacking", 1)
		rotation = dist / math.abs(dist)
		setExecNum("dx", rotation)
		setExecNum("rotation", rotation)
	end
end