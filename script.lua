-- test.lua
function add(a, b)
  return a + b
end

print("hello lua ==============")

-- local count = 0
-- local isLoop = true;
-- while isLoop do 
--   count = count + 1
--   if count > 100000 then
--     isLoop = false
--     print("test loop")
--   end
--   print("test loop")
-- end


function update(dt)
  -- Game logic here, e.g., move entities, check collisions
  print("Lua update called with dt: " .. dt)
end