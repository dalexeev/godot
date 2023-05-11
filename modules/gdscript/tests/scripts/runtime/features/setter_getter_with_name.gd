var a: set = set_normal_untyped, get = get_normal_untyped
var b: int: set = set_normal_typed, get = get_normal_typed
var c: set = set_named_untyped, get = get_named_untyped
var d: int: set = set_named_typed, get = get_named_typed

func test():
	print(a)
	print(b)
	print(c)
	print(d)
	print(get("a"))
	print(get("b"))
	print(get("c"))
	print(get("d"))
	a = 1
	b = 1
	c = 1
	d = 1
	set("a", 2)
	set("b", 2)
	set("c", 2)
	set("d", 2)

func set_normal_untyped(value, optional = true):
	prints("set_normal_untyped", value, optional)

func get_normal_untyped(optional = true):
	prints("get_normal_untyped", optional)
	return 0

func set_normal_typed(value: int, optional: bool = true) -> void:
	prints("set_normal_typed", value, optional)

func get_normal_typed(optional = true) -> int:
	prints("get_normal_typed", optional)
	return 0

func set_named_untyped(name, value, optional = true):
	prints("set_named_untyped", name, value, optional)

func get_named_untyped(name, optional = true):
	prints("get_named_untyped", name, optional)
	return 0

func set_named_typed(name: StringName, value: int, optional: bool = true) -> void:
	prints("set_named_typed", name, value, optional)

func get_named_typed(name: StringName, optional: bool = true) -> int:
	prints("get_named_typed", name, optional)
	return 0
