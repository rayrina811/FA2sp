step = tonumber(input_box("«Î ‰»Î¬∑æ∂µ„æ‡¿Î"))
for Y=iso_size(),0,-step*2 do
	for y=Y,iso_size(),step do
		x = y - Y
		place_waypoint(x, y)
	end
end
for X=step*2,iso_size(),step*2 do
	for x=X,iso_size(),step do
		y = x - X
		place_waypoint(x, y)
	end
end
