function tutorial()
	local state = -1
	if not hasItem("weapon:lighter", 0) and
		not checkCollision("camOwner", "trigger-nightstand") and
		getStr("camOwner-weapon") ~= "lighter" then state = 0
	elseif hasItem("weapon:lighter", 0) and getNum("input-main-showInventory") ~= 1 then state = 1
	elseif hasItem("weapon:lighter", 0) and getStr("camOwner-weapon") ~= "lighter" then state = 2 end

	if state ~= -1 then exec("window showHint {Window-str-tutorial"..state.."} 0") end
end

function main()
	tutorial()
	setNum("gotoHouse-active", (getStr("camOwner-weapon") == "lighter") and 1 or 0)
end