@static_unload

class A:
	static var x: int = 1

	static var y: int = 2:
		set(_value):
			print("The setter is NOT called on initialization.")

	# GH-77331
	func test_inheritance() -> void:
		x = 999
		print(x)

class B extends A:
	pass

func test():
	prints(A.x, B.x)
	A.x = 2
	prints(A.x, B.x)
	B.x = 3
	prints(A.x, B.x)

	var b := B.new()
	b.test_inheritance()
