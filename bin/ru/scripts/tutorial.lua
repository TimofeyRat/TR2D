function init()
	-- body
end

function main()
	if getNum("lvl-tutorial1") == 1 then
		exec("window showHint {Window-str-tutorial1} 0")
		if getNum("input-main-inventory") == 1 then
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