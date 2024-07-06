function clamp(x, min, max)
	return math.max(min, math.min(max, x))
end

function distance(x1, y1, x2, y2)
	local dx = x2 - x1
	local dy = y2 - y1
	return math.sqrt(dx * dx + dy * dy)
end

function distanceToPlayer(x, y)
	return distance(
		x, y,
		getNum("camOwner-posX"),
		getNum("camOwner-posY")
	)
end