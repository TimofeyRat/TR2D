function init()
	-- body
end

function clamp(x, min, max)
	return math.max(min, math.min(max, x))
end

function distance(x1, y1, x2, y2)
	local dx = x2 - x1
	local dy = y2 - y1
	return math.sqrt(dx * dx + dy * dy)
end

function main()
	setExecNum("attacking", 0)
	setExecNum("dx", 0)
	setExecNum("speed", 1)
	local dist = distance(getExecNum("posX"), getExecNum("posY"), getNum("ent Nasake posX"), getNum("ent Nasake posY"))
	if (dist == clamp(dist, -750, 750)) then
		dist = getNum("ent-Nasake-posX") - getExecNum("posX")
		setExecNum("attacking", 1)
		rotation = dist / math.abs(dist)
		setExecNum("dx", rotation)
		setExecNum("rotation", rotation)
	end
end