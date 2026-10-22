-- SANDBOX: checks from inside a cart that nothing outside the cart API is reachable.
-- Finishes "win" on the first tick when every check passes; otherwise logs each failure
-- and finishes "lose".

local failures = {}

local function expect(ok, what)
    if not ok then
        table.insert(failures, what)
    end
end

local function fails(fn, ...)
    return not pcall(fn, ...)
end

-- Host libraries and loaders are absent.
expect(io == nil, "io is reachable")
expect(os == nil, "os is reachable")
expect(debug == nil, "debug is reachable")
expect(require == nil, "require is reachable")
expect(loadstring == nil and load == nil, "a code loader is reachable")
expect(dofile == nil and loadfile == nil, "a file loader is reachable")
expect(print == nil, "print is reachable")
expect(gcinfo == nil and collectgarbage == nil, "GC introspection is reachable")
expect(vector == nil, "the vector library is reachable")

-- The RNG and libm functions are gone from math, including their compiled fast paths.
for _, name in { "random", "randomseed", "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
                 "sinh", "cosh", "tanh", "exp", "log", "log10", "pow" } do
    expect(math[name] == nil, "math." .. name .. " is reachable")
end
expect(fails(function() return math.random() end), "math.random() runs")
expect(fails(function() return math.sin(1) end), "math.sin(1) runs through a fast path")
expect(fails(function() return math.pow(2, 3) end), "math.pow(2, 3) runs through a fast path")
expect(fails(function() return vector.create(1, 2, 3) end), "vector.create runs through a fast path")
expect(fails(function() return os.clock() end), "os.clock() runs")

-- Exact math stays.
expect(math.floor(2.5) == 2 and math.sqrt(16) == 4 and math.abs(-3) == 3, "exact math is missing")
expect(math.clamp(5, 0, 3) == 3 and math.max(1, 2) == 2, "exact math is missing")

-- The API, libraries and string metatable are frozen.
expect(fails(function() mother.log = nil end), "the mother table is writable")
expect(fails(function() math.floor = nil end), "the math table is writable")
expect(fails(function() string.rep = nil end), "the string table is writable")
expect(fails(function() getmetatable("").__index = {} end), "the string metatable is writable")
expect(fails(function() _G.cls = nil end), "the global API table is writable")

-- import() stays inside the cart folder.
expect(fails(import, "../hello/main"), "import escaped with ..")
expect(fails(import, "C:/Windows/win"), "import took an absolute path")
expect(fails(import, "hello/box"), "import took a subfolder path")
expect(fails(import, "missing"), "import found a missing module")
expect(fails(import, "main"), "import loaded main.luau again")

-- Reserved input buttons and 3D names.
expect(fails(btn, "x") and fails(btn, "y") and fails(btnp, "x"), "X/Y are readable by the cart")
expect(fails(tri) and fails(mesh) and fails(cam3d), "a 3D stub did not raise")

-- Drawing only happens inside cart.draw().
expect(fails(cls, 0), "cls() works outside cart.draw()")

-- The heap is capped at 16 MB.
expect(fails(string.rep, "x", 32 * 1024 * 1024), "a 32 MB string fit in the heap")

function cart.init() end

function cart.update()
    if #failures == 0 then
        mother.finish("win")
        return
    end
    for _, what in failures do
        mother.log("FAIL: " .. what)
    end
    mother.finish("lose")
end

function cart.draw() end

function cart.hash()
    return #failures
end
