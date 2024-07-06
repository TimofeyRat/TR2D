require("ru/scripts/funcs")

function init()
	jumpTimer = 0
	enemyDistance = getExecNum("enemyDistance")
	jumpCooldown = getExecNum("jumpCooldown")
end

--[[
	Algorithm:
		1. If distance between slime and enemy is greater than getExecNum("enemyDistance"):
			1.1. Reset jump timer
			1.2. End movement
			1.3. Reset 'attacking'
		2. Else if jump timer is greater than getExecNum("jumpCooldown") and slime is on ground:
			2.1. Reset jump timer
			2.2. Set 'dx' according to position of enemy(-1 if enemyX < slimeX and 1 otherwise)
			2.3. Set 'dy' to -1
			2.4. Set 'attacking' to 1
]]

function main()
	local dist = distanceToPlayer(
		getExecNum("posX"),
		getExecNum("posY")
	)
	local onGround = getExecNum("onGround")
	jumpTimer = jumpTimer + getDeltaTime()
	
	if dist > enemyDistance then
		jumpTimer = 0
		setExecNum("dx", 0)
		setExecNum("dy", 0)
		setExecNum("attacking", 0)
		return
	end
	
	if onGround == 1 and getExecNum("wasOnGround") == 0 then setExecNum("dx", 0) end

	if jumpTimer < jumpCooldown then return end

	jumpTimer = 0

	if onGround == 1 then
		local diff = getNum("camOwner-posX") - getExecNum("posX")
		setExecNum("dx", diff / math.abs(diff))
		setExecNum("dy", -1)
	elseif onGround == 0 then setExecNum("dy", 0) end
	

	setExecNum("wasOnGround", onGround)
end