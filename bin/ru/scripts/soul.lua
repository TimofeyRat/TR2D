function main()
	if (getExecNum("rotateToPlayer") == 1) then
		local rotation = 0
		if (getNum("camOwner-posX") < getExecNum("posX")) then rotation = -1 end
		if (getNum("camOwner-posX") > getExecNum("posX")) then rotation = 1 end
		setExecNum("rotation", rotation)
	end
end