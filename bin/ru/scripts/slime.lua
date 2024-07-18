require("ru/scripts/funcs")

function init()
	jumpTimer = 0
	enemyDistance = getExecNum("enemyDistance")
	jumpCooldown = getExecNum("jumpCooldown")
	damageCD = getExecNum("damageCooldown")
end

function main()
	local dist = distanceToPlayer(
		getExecNum("posX"),
		getExecNum("posY")
	)
	local onGround = getExecNum("onGround")
	jumpTimer = jumpTimer + getDeltaTime()

	print(getExecNum("attackOnTouch"))
	setExecNum("attackOnTouch", getExecNum("noHurtTimer") < damageCD and 0 or 1)
	
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