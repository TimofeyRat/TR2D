function init()
	-- body
end

function tutorial()
	if not hasVar("lvl-tutorial01") or getNum("lvl-tutorial01") == 0 then
		if getNum("lvl-tutorial0") == 0 then exec("window showHint {Window-str-tutorial0} 0") end
		if getNum("camOwner-interacted") >= 1 then
			setNum("lvl-tutorial0", 1)
			setNum("lvl-tutorial01", 1)
			setNum("lvl-tutorial1", 1)
		end
	end
	if getNum("lvl-tutorial1") == 1 then
		exec("window showHint {Window-str-tutorial1} 0")
		if getNum("input-main-showInventory") == 1 then
			setNum("lvl-tutorial1", 0)
			setNum("lvl-tutorial2", 1)
		end
	end
	if getNum("lvl-tutorial2") == 1 then
		exec("window showHint {Window-str-tutorial2} 0")
		if getStr("camOwner-weapon") == "lighter" then
			setNum("lvl-tutorial2", 0)
		end
	end
end

function main()
	tutorial()
	setNum("gotoHouse-active", (getStr("camOwner-weapon") == "lighter") and 1 or 0)
end